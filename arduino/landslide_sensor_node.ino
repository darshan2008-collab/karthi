/*
  ====================================================================================
  LANDSLIDE EARLY WARNING SYSTEM - SENSOR NODE
  மண் சரிவு முன்னெச்சரிக்கை அமைப்பு - Arduino / ESP32 Code
  ====================================================================================
  Sensors Connected:
  1. Soil Moisture Sensor    -> Analog Pin A0
  2. Rain Water Sensor       -> Analog Pin A1
  3. MPU6050 Tilt Sensor     -> I2C (SDA: A4 / GPIO21, SCL: A5 / GPIO22, VCC: 5V/3.3V, GND: GND)

  Baud Rate: 115200 bps (Dashboard-ல் உள்ள Baud Rate உடன் பொருந்த வேண்டும்)
  ====================================================================================
*/

#include <Wire.h>

// Sensor Pin Definitions
const int SOIL_PIN = A0;
const int RAIN_PIN = A1;

// MPU6050 I2C Address (default is 0x68)
const int MPU_ADDR = 0x68;

// Accelerometer raw values
int16_t ax, ay, az;
int16_t gx, gy, gz;

void setup() {
  // Initialize Serial Monitor over USB
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  // Initialize I2C communication
  Wire.begin();
  
  // Wake up MPU-6050 (default is sleep mode)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  delay(500);
}

void loop() {
  // -------------------------------------------------------------
  // 1. READ SOIL MOISTURE SENSOR (A0)
  // -------------------------------------------------------------
  // Typical capacitive/resistive sensor: Dry ~ 1023, Wet in water ~ 250-400
  int soilRaw = analogRead(SOIL_PIN);
  // Map raw analog reading to percentage (0% = dry, 100% = wet)
  int soilPercent = map(soilRaw, 1023, 300, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  // -------------------------------------------------------------
  // 2. READ RAIN WATER SENSOR (A1)
  // -------------------------------------------------------------
  // Rain board sensor: Dry ~ 1023, Wet droplets ~ 300-600
  int rainRaw = analogRead(RAIN_PIN);
  int rainPercent = map(rainRaw, 1023, 250, 0, 100);
  rainPercent = constrain(rainPercent, 0, 100);

  // -------------------------------------------------------------
  // 3. READ MPU6050 ACCELEROMETER FOR TILT (PITCH & ROLL)
  // -------------------------------------------------------------
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true); // request a total of 6 registers

  float pitch = 0.0;
  float roll = 0.0;

  if (Wire.available() >= 6) {
    ax = Wire.read() << 8 | Wire.read(); // X-axis acceleration
    ay = Wire.read() << 8 | Wire.read(); // Y-axis acceleration
    az = Wire.read() << 8 | Wire.read(); // Z-axis acceleration

    // Convert raw accelerometer reading to G-force (scale +/- 2g = 16384 LSB/g)
    float ax_g = (float)ax / 16384.0;
    float ay_g = (float)ay / 16384.0;
    float az_g = (float)az / 16384.0;

    // Calculate Pitch and Roll angles in degrees
    pitch = atan2(ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 180.0 / PI;
    roll  = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180.0 / PI;
  }

  // -------------------------------------------------------------
  // 4. DETERMINE LANDSLIDE RISK STATUS
  // -------------------------------------------------------------
  float totalTilt = sqrt(pitch * pitch + roll * roll);
  String risk = "NORMAL";

  if (soilPercent >= 75 && (rainPercent >= 60 || totalTilt >= 25.0)) {
    risk = "DANGER";
  } else if (totalTilt >= 30.0) {
    risk = "DANGER";
  } else if (soilPercent >= 65 || rainPercent >= 45 || totalTilt >= 15.0) {
    risk = "WARNING";
  }

  // -------------------------------------------------------------
  // 5. SEND JSON PACKET OVER USB SERIAL TO DASHBOARD
  // -------------------------------------------------------------
  Serial.print("{\"soil\":");
  Serial.print(soilPercent);
  Serial.print(",\"soil_raw\":");
  Serial.print(soilRaw);
  Serial.print(",\"rain\":");
  Serial.print(rainPercent);
  Serial.print(",\"rain_raw\":");
  Serial.print(rainRaw);
  Serial.print(",\"pitch\":");
  Serial.print(pitch, 1);
  Serial.print(",\"roll\":");
  Serial.print(roll, 1);
  Serial.print(",\"risk\":\"");
  Serial.print(risk);
  Serial.println("\"}");

  // Transmit every 1 second (1000 ms)
  delay(1000);
}
