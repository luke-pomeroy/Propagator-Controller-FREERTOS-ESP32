#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>
#include <RBDdimmer.h>//https://github.com/RobotDynOfficial/RBDDimmer

//Include Conf env variables
#include ".\Conf.h"

//Fan dimmer settings
int MIN_POWER  = 0;
int MAX_POWER  = 100;
int POWER_STEP  = 1;
int power  = 0;

//Pins for AC Dimmer Module
// ZC (Zero Cross)
//PSM (PWM/acd)
const int zeroCrossPin  = ZEROCROSS;
const int acdPin  = ACD;

dimmerLamp acd(acdPin,zeroCrossPin);

Adafruit_AHTX0 aht;

const long utcOffsetInSeconds = 0;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

long wifiStartAttemptTime;
float targetTemp(NAN);
float cHumidity(NAN);
float cTemp(NAN);
bool targetLight(false);
bool invertLight(false);
bool timeSet(false);
bool tempSet(false);
bool ahtConnected(false);
                    
void syncTime(void * parameter){
  for(;;){
    if(WiFi.status() != WL_CONNECTED){ //Don't continue if WiFi is not connected
      vTaskDelay(10000 / portTICK_PERIOD_MS); //check Wi-Fi is connected every 10 seconds
      continue;
    }
    timeSet = true;
    vTaskDelay(3.6e+6 / portTICK_PERIOD_MS); // 3.6e+6 = 1 hour
    timeClient.update();
    String formattedDate = timeClient.getFormattedDate();
    Serial.println(formattedDate);
  }
}

void keepWiFiAlive(void * parameter){
    for(;;){
        
        if(WiFi.status() == WL_CONNECTED){
            vTaskDelay(10000 / portTICK_PERIOD_MS); //check Wi-Fi is connected every 10 seconds
            wifiStartAttemptTime = millis();
            continue;
        }
        if(WiFi.status() != WL_IDLE_STATUS && WiFi.status() != WL_CONNECTED){
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);   
            WiFi.setHostname(HOSTNAME);
            WiFi.setAutoReconnect(true);
            WiFi.persistent(true);
        }
        
        // Keep looping while we're not connected and haven't reached the timeout
        if(WiFi.status() != WL_CONNECTED && millis() - wifiStartAttemptTime < WIFI_TIMEOUT_MS){
            Serial.println("Connecting to WiFi...");
            Serial.println(WiFi.status());
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            
            if(WiFi.status() != WL_CONNECTED){
              continue;
            }
        }

        // When we couldn't make a WiFi connection (or the timeout expired)
        // Disconnect Wi-Fi then sleep for a while and then retry.
        if(WiFi.status() != WL_CONNECTED){
            WiFi.disconnect();
            Serial.println("Could not connect to WiFi.");
            vTaskDelay(WIFI_RECOVER_TIME_MS / portTICK_PERIOD_MS);
            Serial.println("Retrying connection to Wi-Fi");
            wifiStartAttemptTime = millis();
            continue;
        }
        
        timeClient.update(); // update the time now that we're connected to WiFi
        String formattedDate = timeClient.getFormattedDate();
        Serial.println(formattedDate);
        
        // print the SSID of the network you're attached to:
        Serial.print("Conntected to Wi-Fi! SSID: ");
        Serial.println(WiFi.SSID());
      
        // print your WiFi shield's IP address:
        IPAddress ip = WiFi.localIP();
        Serial.print("IP Address: ");
        Serial.println(ip);

        // print the Hostname of the device:
        Serial.print("Hostname: ");
        Serial.println(WiFi.getHostname());
      
        // print the received signal strength:
        long rssi = WiFi.RSSI();
        Serial.print("signal strength (RSSI):");
        Serial.print(rssi);
        Serial.println(" dBm");
    }
}

void readSensors(void * parameter){
       
    for(;;){
        if(!timeSet){
            vTaskDelay(5000 / portTICK_PERIOD_MS); //check if time is set every 5 seconds if not set already
            continue;
        }
        invertLight = (digitalRead(SWITCHPIN)==LOW);
        
        if (timeClient.getHours() >= LIGHT_OFF_TIME && timeClient.getHours() <= LIGHT_ON_TIME ) {
          if(invertLight){
            targetLight = true;
            Serial.println("Light manually turned on");
          } else{
            targetLight = false;
            Serial.println("Light is set to FALSE");
          }
          targetTemp = LIGHT_OFF_TARGET_T;
        } else {
            if(invertLight){
              targetLight = false;
              Serial.println("Light manually turned off");
            } else {
              Serial.println("Light is set to TRUE");
              targetLight = true;
            }
          targetTemp = LIGHT_ON_TARGET_T;
        }
        Serial.print("Target temp is ");
        Serial.println(targetTemp);
        Serial.print("Target light is ");
        Serial.println(targetLight);
        Serial.print("Invert light status switch is ");
        Serial.println(invertLight);

        // Temp is controlled by aht, only read sensors and set relays if aht is connected
        if(ahtConnected){
          // Print the AHT Sensor data
          sensors_event_t humidity, temp;
          aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
          Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
          Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
          cHumidity = humidity.relative_humidity;
          cTemp = temp.temperature;
          tempSet = true;
          // Only turn on/off relays if time has been set on boot
          if(timeSet){
             if (tempSet){
                //Compare values with target, and act based on variances
                if (temp.temperature >= targetTemp && digitalRead(HEATRLY)){
                  Serial.println("Heat Relay OFF");
                  digitalWrite(HEATRLY, LOW);
                } else if (temp.temperature < targetTemp && !digitalRead(HEATRLY)){
                  Serial.println("Heat Relay ON");
                  digitalWrite(HEATRLY, HIGH);
                }
              }
              if (targetLight && !digitalRead(LIGHTRLY1) && !digitalRead(LIGHTRLY2)){
                Serial.println("Heat Relays ON");
                digitalWrite(LIGHTRLY1, HIGH);
                digitalWrite(LIGHTRLY2, HIGH);
              } else if (!targetLight && digitalRead(LIGHTRLY1) && digitalRead(LIGHTRLY2)){
                  Serial.println("Heat Relays OFF");
                  digitalWrite(LIGHTRLY1, LOW);
                  digitalWrite(LIGHTRLY2, LOW);
              }
           }
        }
        
        Serial.print("Light Relay 1 Status: ");
        Serial.println(digitalRead(LIGHTRLY1));
        Serial.print("Light Relay 2 Status: ");
        Serial.println(digitalRead(LIGHTRLY2));
        Serial.print("Heat Relay Status: ");
        Serial.println(digitalRead(HEATRLY));
        Serial.print("Fan power: ");
        Serial.print(acd.getPower());
        Serial.println("%");
        Serial.print("Temp set: ");
        Serial.println(tempSet);
        Serial.print("AHT Status: ");
        Serial.println(ahtConnected);
  
        vTaskDelay(SENSOR_READ_FREQ_MS / portTICK_PERIOD_MS);
    }
}

void sendReading(void * parameter) {

  for(;;){
    if(WiFi.status() != WL_CONNECTED || !timeSet || !tempSet){ //Don't continue if WiFi is not connected or time is not set
      vTaskDelay(5000 / portTICK_PERIOD_MS); //check Wi-Fi is connected every 5 seconds
      continue;
    }
    StaticJsonDocument<300> JSONbuffer;
    JsonObject JSONencoder = JSONbuffer.to<JsonObject>();
    char JSONmessageBuffer[300];
    HTTPClient http;
    const char* serverName = SERVER_NAME;
    const char* deviceId = DEVICE_ID;
    String formattedDate = timeClient.getFormattedDate();
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    JSONencoder["device_id"] = deviceId;
    JSONencoder["temperature"] = cTemp;
    JSONencoder["humidity"] = cHumidity;
    JSONencoder["user"] = "logger";
    JSONencoder["pass"] = "NbFjs23!DF19";
    JSONencoder["timestamp"] = formattedDate;
    serializeJson(JSONbuffer, JSONmessageBuffer);
    Serial.println("Pretty JSON message from buffer: ");
    Serial.println(JSONmessageBuffer);
    int httpResponseCode = http.POST(JSONmessageBuffer);
    
    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.println("Wi-Fi Status: ");
    Serial.print(WiFi.status());
    
    // if response code is less than 0 eg -1, end hhtp request and disconnect Wi-Fi so connection can be re-established, then we try again
    if(httpResponseCode < 0){
       Serial.println("sendReading error: HTTP Response -1, disconnecting from Wi-Fi so connection can be re-established...");
       http.end();
       WiFi.disconnect();
       continue;
    }
    
    Serial.println("\nHTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
    vTaskDelay(300000 / portTICK_PERIOD_MS); // 300000 = 5 mins until next reading sent
  }
}

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect.
  }
  
  pinMode(LIGHTRLY1, OUTPUT);
  pinMode(LIGHTRLY2, OUTPUT);
  pinMode(HEATRLY, OUTPUT);
  pinMode(SWITCHPIN, INPUT_PULLUP);

  digitalWrite(LIGHTRLY1, LOW);
  digitalWrite(LIGHTRLY2, LOW);
  digitalWrite(HEATRLY, LOW); 

  wifiStartAttemptTime = millis();
  
  acd.begin(NORMAL_MODE, ON);
  acd.setPower(50);
  delay(1000);
  acd.setPower(72);
  Wire.begin(SDAPIN, SCLPIN); // SDA and SCL
    
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
  } else {
    Serial.println("AHT10 or AHT20 found");
    ahtConnected = true;
  }
  
  xTaskCreatePinnedToCore(
  keepWiFiAlive,    // Task function
  "keepWiFiAlive",  // Task name
  5000,             // Stack size (bytes)
  NULL,             // Parameter
  3,                // Task priority
  NULL,             // Task handle
  0
  );
  xTaskCreatePinnedToCore(
  syncTime,    // Task function
  "syncTime",  // Task name
  5000,             // Stack size (bytes)
  NULL,             // Parameter
  2,                // Task priority
  NULL,             // Task handle
  0
  );
  xTaskCreatePinnedToCore(
  sendReading,    // Task function
  "sendReading",  // Task name
  5000,             // Stack size (bytes)
  NULL,             // Parameter
  1,                // Task priority
  NULL,             // Task handle
  0
  );
  xTaskCreatePinnedToCore(
  readSensors,    // Task function
  "readSensors",  // Task name
  5000,             // Stack size (bytes)
  NULL,             // Parameter
  1,                // Task priority
  NULL,             // Task handle
  1
  );

}

void loop() {
  // put your main code here, to run repeatedly:
}
