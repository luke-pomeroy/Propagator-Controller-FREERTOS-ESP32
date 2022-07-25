// Pin Definitions for ESP32
#define HEATRLY 12
#define LIGHTRLY1 32
#define LIGHTRLY2 33
#define SWITCHPIN 22
#define SDAPIN 25
#define SCLPIN 26
#define ONE_WIRE_BUS 18
#define ZEROCROSS 27
#define ACD 14

// WiFi settings
#define WIFI_NETWORK "ssid wifi network"
#define WIFI_PASSWORD "wifi password"
#define WIFI_TIMEOUT_MS 20000 // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt
#define HOSTNAME "ESP32-FREERTOS"
#define SERVER_NAME "https://urlof/serverendpoint"
#define DEVICE_ID "My device number 1"
#define LIGHT_ON_TIME 9
#define LIGHT_OFF_TIME 19
#define LIGHT_ON_TARGET_T 25.00
#define LIGHT_OFF_TARGET_T 10.00

#define SENSOR_READ_FREQ_MS 10000 // Read sensors every 10 seconds
