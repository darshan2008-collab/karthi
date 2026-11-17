#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// WI-FI & IP CONFIGURATION
// ==========================================
// ESP32 Wi-Fi Hotspot details (Connect laptop/phone to this):
const char* AP_SSID     = "ESP32_LANDSLIDE_NODE";  // ESP Device Name
const char* AP_PASSWORD = "password123";           // Wi-Fi Password

WebServer server(80);
String espIP = "192.168.4.1";

// ========== OLED CONFIGURATION ==========

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(
  U8G2_R0,
  19,
  18,
  U8X8_PIN_NONE
);

// ========== MPU6050 ==========

TwoWire MPU_Wire = TwoWire(0);
Adafruit_MPU6050 mpu;

bool mpuReady = false;

// ========== SENSOR PINS ==========

#define RAIN_PIN 34
#define SOIL_PIN 35
#define LED_PIN 26

// ========== THRESHOLDS ==========

#define RAIN_THRESHOLD 2000
#define SOIL_THRESHOLD 2000
#define TILT_THRESHOLD 15.0

// ========== VARIABLES ==========

int rainValue = 0;
int soilValue = 0;

float temperature = 0;
float tilt = 0;

float ax = 0;
float ay = 0;
float az = 0;

float gx = 0;
float gy = 0;
float gz = 0;

bool warning = false;

unsigned long previousMillis = 0;

const unsigned long interval = 500;

// ========== HTTP API HANDLER FOR IP DASHBOARD ==========
void handleApiData() {
  // Convert 12-bit ADC (4095 dry -> 1000 wet) to percentage 0-100%
  int soilPercent = constrain(map(soilValue, 4095, 1000, 0, 100), 0, 100);
  int rainPercent = constrain(map(rainValue, 4095, 1000, 0, 100), 0, 100);

  String json = "{";
  json += "\"soil\":" + String(soilPercent) + ",";
  json += "\"soil_raw\":" + String(soilValue) + ",";
  json += "\"rain\":" + String(rainPercent) + ",";
  json += "\"rain_raw\":" + String(rainValue) + ",";
  json += "\"pitch\":" + String(tilt, 1) + ",";
  json += "\"roll\":0.0,";
  json += "\"temp\":" + String(temperature, 1) + ",";
  json += "\"status\":\"" + String(warning ? "WARNING" : "NORMAL") + "\",";
  json += "\"ip\":\"" + espIP + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  server.send(200, "application/json", json);
}

void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
  server.send(204);
}

// ========== SETUP ==========

void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);

  // OLED START
  display.begin();

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);

  display.drawStr(10, 25, "LANDSLIDE");
  display.drawStr(10, 40, "MONITORING SYSTEM");

  display.sendBuffer();

  delay(1000);

  // MPU6050 START
  MPU_Wire.begin(21, 22);

  if (mpu.begin(0x68, &MPU_Wire)) {

    mpuReady = true;

    mpu.setAccelerometerRange(
      MPU6050_RANGE_8_G
    );

    mpu.setGyroRange(
      MPU6050_RANGE_500_DEG
    );

    Serial.println("MPU6050 CONNECTED");

  }
  else {

    mpuReady = false;

    Serial.println("MPU6050 NOT DETECTED");

  }

  // START ESP32 WI-FI ACCESS POINT (FOR IP CONNECTION)
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  espIP = WiFi.softAPIP().toString();

  // START WEB SERVER FOR DASHBOARD
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/data", HTTP_OPTIONS, handleOptions);
  server.on("/", HTTP_GET, handleApiData);
  server.begin();

  Serial.println("======================================");
  Serial.print("ESP32 Wi-Fi SSID : "); Serial.println(AP_SSID);
  Serial.print("Password         : "); Serial.println(AP_PASSWORD);
  Serial.print("Connect IP       : "); Serial.println(espIP);
  Serial.println("======================================");

  display.clearBuffer();
  display.drawStr(5, 20, "WIFI HOTSPOT:");
  display.drawStr(5, 35, AP_SSID);
  display.drawStr(5, 50, espIP.c_str());
  display.sendBuffer();

  delay(1200);

}

// ========== MAIN LOOP ==========

void loop() {

  // HANDLE INCOMING DASHBOARD IP REQUESTS
  server.handleClient();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {

    previousMillis = currentMillis;

    // READ RAIN SENSOR
    rainValue = analogRead(RAIN_PIN);

    // READ SOIL SENSOR
    soilValue = analogRead(SOIL_PIN);

    // READ MPU6050
    if (mpuReady) {

      sensors_event_t a, g, temp;

      mpu.getEvent(&a, &g, &temp);

      ax = a.acceleration.x;
      ay = a.acceleration.y;
      az = a.acceleration.z;

      gx = g.gyro.x;
      gy = g.gyro.y;
      gz = g.gyro.z;

      temperature = temp.temperature;

      tilt = atan2(ay, az) * 180.0 / PI;

    }

    // ALERT CONDITIONS
    bool rainAlert =
      rainValue < RAIN_THRESHOLD;

    bool soilAlert =
      soilValue < SOIL_THRESHOLD;

    bool tiltAlert =
      mpuReady &&
      (fabs(tilt) > TILT_THRESHOLD);

    warning =
      rainAlert || soilAlert || tiltAlert;

    // LED CONTROL
    digitalWrite(
      LED_PIN,
      warning ? HIGH : LOW
    );

    // SERIAL MONITOR
    Serial.println("----------------------");

    Serial.print("Rain: ");
    Serial.println(rainValue);

    Serial.print("Soil: ");
    Serial.println(soilValue);

    Serial.print("MPU6050: ");
    Serial.println(mpuReady ? "CONNECTED" : "ERROR");

    if (mpuReady) {

      Serial.print("Temperature: ");
      Serial.println(temperature);

      Serial.print("Tilt: ");
      Serial.println(tilt);

      Serial.print("Acceleration X: ");
      Serial.println(ax);

      Serial.print("Acceleration Y: ");
      Serial.println(ay);

      Serial.print("Acceleration Z: ");
      Serial.println(az);

      Serial.print("Gyroscope X: ");
      Serial.println(gx);

      Serial.print("Gyroscope Y: ");
      Serial.println(gy);

      Serial.print("Gyroscope Z: ");
      Serial.println(gz);

    }

    Serial.print("STATUS: ");
    Serial.println(warning ? "WARNING" : "NORMAL");

    // OLED DISPLAY
    display.clearBuffer();

    display.setFont(u8g2_font_6x12_tf);

    display.drawStr(0, 10, "LANDSLIDE MONITOR");

    display.drawHLine(0, 13, 128);

    char line[30];

    snprintf(
      line,
      sizeof(line),
      "Rain  : %d",
      rainValue
    );

    display.drawStr(0, 25, line);

    snprintf(
      line,
      sizeof(line),
      "Soil  : %d",
      soilValue
    );

    display.drawStr(0, 37, line);

    if (mpuReady) {

      snprintf(
        line,
        sizeof(line),
        "Temp  : %.1f C",
        temperature
      );

      display.drawStr(0, 49, line);

      snprintf(
        line,
        sizeof(line),
        "Tilt  : %.1f deg",
        tilt
      );

      display.drawStr(0, 61, line);

    }
    else {

      display.drawStr(0, 49, "MPU6050 ERROR");

      display.drawStr(0, 61, "CHECK CONNECTION");

    }

    display.sendBuffer();

  }

}
