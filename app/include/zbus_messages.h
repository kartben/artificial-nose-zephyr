#ifndef __APP_ZBUS_MESSAGES_H__
#define __APP_ZBUS_MESSAGES_H__

struct inference_result_msg {
	char label[16];
	float confidence;
	float anomaly_score;
};

#endif /* __APP_ZBUS_MESSAGES_H__ */