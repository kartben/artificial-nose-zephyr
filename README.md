# Artificial Nose - Zephyr version

This repository contains a complete rewrite of https://github.com/kartben/artificial-nose using
Zephyr RTOS.

The overall functionality is the same (it even uses the exact same TensorFlow model as the previous
version), but the code has been rewritten from scratch to take advantage of Zephyr's features, in
particular:

- Use multiple threads, each one handling a specific task (ex. sensor reading, model inference,
  Graphical User Interface, etc.)
- Use zbus to communicate between threads
- Use LVGL to create the graphical user interface, using native widgets such as charts and meters.

## Author <!-- omit in toc -->

👤 **Benjamin Cabé**

- Website: [https://blog.benjamin-cabe.com](https://blog.benjamin-cabe.com)
- Twitter: [@kartben](https://twitter.com/kartben)
- Github: [@kartben](https://github.com/kartben)
- LinkedIn: [@benjamincabe](https://linkedin.com/in/benjamincabe)

## 🤝 Contributing <!-- omit in toc -->

Contributions, issues and feature requests are welcome!

Feel free to check [issues page](https://github.com/kartben/artificial-nose-zephyr/issues).

## Show your support <!-- omit in toc -->

Give a ⭐️ if this project helped you!

## 📝 License <!-- omit in toc -->

Copyright &copy; 2020-2023 [Benjamin Cabé](https://github.com/kartben).

This project is [MIT](/LICENSE) licensed.
