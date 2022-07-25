# ESP32 Dual Core FREERTOS Propagator Controller

This is a Propagator Controller and uses FREERTOS to take advantage of both cores of the ESP32, and the ability to carry out two tasks simultaneously.
One core maintains the Wi-Fi connection and time, while the other core reads the temperature sensor and activates relays accordingly.

Overview of main features:
- Reads data from a waterproof DS18B20 temperature sensor and maintains a constant target temperature via a solid state relay.
- Control propagator light, turning on and off at times set in Conf.h.
- Invert propagator light using a switch, to turn lighting on outside of times set.
- Maintain an active Wi-Fi connection and ensure time is set.
- Submit temperature readings via HTTPS to a cloud server ([Cactusapp](https://github.com/whyspark/cactusapp)).
