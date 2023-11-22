#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>

#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include "zbus_messages.h"
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>

#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <errno.h>

#define SERVER_ADDR          CONFIG_APP_MQTT_BROKER_HOSTNAME
#define SERVER_PORT          CONFIG_APP_MQTT_BROKER_PORT
#define APP_SLEEP_MSECS      8000
#define APP_MQTT_BUFFER_SIZE 200

/* Buffers for MQTT client. */
static uint8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* Socket Poll */
static struct zsock_pollfd fds[1];
static int nfds;

static bool mqtt_connected;

static struct k_work_delayable pub_message;
#if defined(CONFIG_NET_DHCPV4)
static struct k_work_delayable check_network_conn;

/* Network Management events */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

static struct net_mgmt_event_callback l4_mgmt_cb;
#endif

#if defined(CONFIG_DNS_RESOLVER)
static struct zsock_addrinfo hints;
static struct zsock_addrinfo *haddr;
#endif

static K_SEM_DEFINE(mqtt_start, 0, 1);

static void mqtt_event_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);

static void prepare_fds(struct mqtt_client *client)
{
	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static int wait(int timeout)
{
	int rc = -EINVAL;

	if (nfds <= 0) {
		return rc;
	}

	rc = zsock_poll(fds, nfds, timeout);
	if (rc < 0) {
		LOG_ERR("poll error: %d", errno);
		return -errno;
	}

	return rc;
}

static void broker_init(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);

#if defined(CONFIG_DNS_RESOLVER)
	net_ipaddr_copy(&broker4->sin_addr, &net_sin(haddr->ai_addr)->sin_addr);
#else
	zsock_inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#endif
}

bool get_device_id(char *id, int id_max_len)
{
	uint8_t hwinfo_id[32];
	ssize_t length;

	length = hwinfo_get_device_id(hwinfo_id, 32);
	if (length <= 0) {
		return false;
	}

	memset(id, 0, id_max_len);
	length = bin2hex(hwinfo_id, (size_t)length, id, id_max_len);

	return length > 0;
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	char hr_addr[NET_IPV4_ADDR_LEN];
	char *hr_family = "IPv4";

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_event_handler;

	char client_id[32];
	if (!get_device_id(client_id, 32)) {
		strcpy(client_id, "ai-nose-123"); // hardcoded ID if hwinfo fails
	}

	client->client_id.utf8 = (uint8_t *)client_id;
	client->client_id.size = strlen(client_id);

	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
}

static void mqtt_event_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	struct mqtt_puback_param puback;
	uint8_t data[33];
	int len;
	int bytes_read;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		mqtt_connected = true;
		LOG_DBG("MQTT client connected!");
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_DBG("MQTT client disconnected %d", evt->result);

		mqtt_connected = false;
		clear_fds();
		break;

	case MQTT_EVT_PUBLISH:
		len = evt->param.publish.message.payload.len;

		LOG_INF("MQTT publish received %d, %d bytes", evt->result, len);
		LOG_INF(" id: %d, qos: %d", evt->param.publish.message_id,
			evt->param.publish.message.topic.qos);

		while (len) {
			bytes_read = mqtt_read_publish_payload(
				&client_ctx, data,
				len >= sizeof(data) - 1 ? sizeof(data) - 1 : len);
			if (bytes_read < 0 && bytes_read != -EAGAIN) {
				LOG_ERR("failure to read payload");
				break;
			}

			data[bytes_read] = '\0';
			LOG_INF("   payload: %s", data);
			len -= bytes_read;
		}

		puback.message_id = evt->param.publish.message_id;
		mqtt_publish_qos1_ack(&client_ctx, &puback);
		break;

	default:
		LOG_DBG("Unhandled MQTT event %d", evt->type);
		break;
	}
}

static int publish(struct mqtt_client *client, const char *topic, const char *payload,
		   enum mqtt_qos qos)
{
	uint8_t len = strlen(topic);
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)topic;
	param.message.topic.topic.size = len;
	param.message.payload.data = payload;
	param.message.payload.len = strlen(payload);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

static void poll_mqtt(void)
{
	int rc;

	while (mqtt_connected) {
		rc = wait(SYS_FOREVER_MS);
		if (rc > 0) {
			mqtt_input(&client_ctx);
		}
	}
}

static int try_to_connect(struct mqtt_client *client)
{
	uint8_t retries = 3U;
	int rc;

	LOG_DBG("attempting to connect...");

	while (retries--) {
		client_init(client);

		rc = mqtt_connect(client);
		if (rc) {
			LOG_ERR("mqtt_connect failed %d", rc);
			continue;
		}

		prepare_fds(client);

		rc = wait(APP_SLEEP_MSECS);
		if (rc < 0) {
			mqtt_abort(client);
			return rc;
		}

		mqtt_input(client);

		if (mqtt_connected) {
			// k_work_reschedule(&pub_message, K_SECONDS(timeout_for_publish()));
			return 0;
		}

		mqtt_abort(client);

		wait(10 * MSEC_PER_SEC);
	}

	return -EINVAL;
}

#if defined(CONFIG_DNS_RESOLVER)
static int get_mqtt_broker_addrinfo(void)
{
	int retries = 3;
	int rc = -EINVAL;

	while (retries--) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = 0;

		rc = zsock_getaddrinfo(SERVER_ADDR, "1883", &hints, &haddr);
		if (rc == 0) {
			LOG_INF("DNS resolved for %s:%d", SERVER_ADDR, 1883);

			return 0;
		}

		LOG_ERR("DNS not resolved for %s:%d, retrying", SERVER_ADDR, 1883);
	}

	return rc;
}
#endif

static void connect_to_mqtt_broker(void)
{
	int rc = -EINVAL;

#if defined(CONFIG_NET_DHCPV4)
	while (true) {
		k_sem_take(&mqtt_start, K_FOREVER);
#endif
#if defined(CONFIG_DNS_RESOLVER)
		rc = get_mqtt_broker_addrinfo();
		if (rc) {
			return;
		}
#endif
		rc = try_to_connect(&client_ctx);
		if (rc) {
			return;
		}

		poll_mqtt();
#if defined(CONFIG_NET_DHCPV4)
	}
#endif
}

/* DHCP tries to renew the address after interface is down and up.
 * If DHCPv4 address renewal is success, then it doesn't generate
 * any event. We have to monitor this way.
 * If DHCPv4 attempts exceeds maximum number, it will delete iface
 * address and attempts for new request. In this case we can rely
 * on IPV4_ADDR_ADD event.
 */
#if defined(CONFIG_NET_DHCPV4)
static void check_network_connection(struct k_work *work)
{
	struct net_if *iface;

	if (mqtt_connected) {
		return;
	}

	iface = net_if_get_default();
	if (!iface) {
		goto end;
	}

	if (iface->config.dhcpv4.state == NET_DHCPV4_BOUND) {
		k_sem_give(&mqtt_start);
		return;
	}

	LOG_INF("waiting for DHCP to acquire addr");

end:
	k_work_reschedule(&check_network_conn, K_SECONDS(3));
}
#endif

#if defined(CONFIG_NET_DHCPV4)
static void abort_mqtt_connection(void)
{
	if (mqtt_connected) {
		mqtt_connected = false;
		mqtt_abort(&client_ctx);
		k_work_cancel_delayable(&pub_message);
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			     struct net_if *iface)
{
	if ((mgmt_event & L4_EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		/* Wait for DHCP to be back in BOUND state */
		k_work_reschedule(&check_network_conn, K_SECONDS(3));

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		abort_mqtt_connection();
		k_work_cancel_delayable(&check_network_conn);

		return;
	}
}
#endif

ZBUS_CHAN_DECLARE(inference_result_chan);

/**
 * Zbus callback for inference results
 *
 * Called every time a new message is published to the inference result channel
 */
static void inference_cb(const struct zbus_channel *chan)
{
	int rc;
	struct inference_result_msg msg;
	zbus_chan_read(chan, &msg, K_MSEC(200));

	if (mqtt_connected) {
		rc = publish(&client_ctx, "smell/label", msg.label, MQTT_QOS_0_AT_MOST_ONCE);
		if (rc) {
			LOG_ERR("mqtt_publish ERROR");
		}

		char confidence[10];
		sprintf(confidence, "%0.2f", msg.confidence);
		rc = publish(&client_ctx, "smell/confidence", confidence, MQTT_QOS_0_AT_MOST_ONCE);
		if (rc) {
			LOG_ERR("mqtt_publish ERROR");
		}
	}
}

ZBUS_LISTENER_DEFINE(mqtt_publisher_listener, inference_cb);

/**
 * Entry point for the MQTT publication thread.
 *
 * This thread is responsible for publishing the inference results to the MQTT
 * broker.
 * It subscribes to the inference result Zbus channel and publishes the results
 * to the MQTT broker.
 */
void mqtt_pub_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	/* Add zbus observer to be notified of new inference results */
	zbus_chan_add_obs(&inference_result_chan, &mqtt_publisher_listener, K_MSEC(200));

#if defined(CONFIG_NET_DHCPV4)
	k_work_init_delayable(&check_network_conn, check_network_connection);

	net_mgmt_init_event_callback(&l4_mgmt_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_mgmt_cb);
#endif

	connect_to_mqtt_broker();
}

#ifdef __cplusplus
}
#endif
