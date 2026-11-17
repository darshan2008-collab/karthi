# Landslide Early Warning & Monitoring Dashboard
## மண் சரிவு முன்னெச்சரிக்கை மற்றும் நிகழ்நேர கண்காணிப்பு பலகை

இந்த சிஸ்டம் Soil Moisture (மண் ஈரப்பதம்), Rain Water (மழை அளவு), மற்றும் MPU6050 (சாய்வு மற்றும் நில அதிர்வு) ஆகிய சென்சார்களின் அளவீடுகளை USB வழியாக கணினியுடன் இணைத்து, ஒரு அதிநவீன dashboard-ல் நிகழ்நேரத்தில் (real-time) கண்காணிக்க உதவுகிறது.

---

### 1. Hardware Components & Wiring (சென்சார் இணைப்புகள்)

| Sensor (சென்சார்) | Arduino Uno Pin | ESP32 Pin | விளக்கம் (Details) |
| :--- | :--- | :--- | :--- |
| **Soil Moisture Sensor** | **A0** | **GPIO 34 / 36** | VCC -> 5V/3.3V, GND -> GND, AO -> A0 |
| **Rain Water Sensor** | **A1** | **GPIO 35 / 39** | VCC -> 5V/3.3V, GND -> GND, AO -> A1 |
| **MPU6050 (SDA)** | **A4** | **GPIO 21** | I2C Serial Data Pin |
| **MPU6050 (SCL)** | **A5** | **GPIO 22** | I2C Serial Clock Pin |
| **MPU6050 (VCC & GND)**| **5V / 3.3V, GND** | **3.3V, GND** | Power Supply |

---

### 2. Quick Start (எப்படி இயக்குவது?)

#### வழிமுறை 1: Browser-ல் நேரடியாக USB இணைப்பது (No Python Needed!)
1. Google Chrome அல்லது Microsoft Edge பிரவுசரில் `index.html` கோப்பை திறக்கவும் (அல்லது `run_dashboard.bat`-ஐ கிளிக் செய்யவும்).
2. திரையின் மேற்பகுதியில் உள்ள **"Connect USB Device"** என்ற பட்டனை கிளிக் செய்யவும்.
3. உங்கள் Arduino / ESP32 இணைக்கப்பட்டுள்ள **COM Port**-ஐ தேர்வு செய்து **Connect** கொடுக்கவும்.
4. சென்சாரின் நேரடி அளவுகள், வரைபடங்கள் (Charts) மற்றும் Landslide Alert உடனடியாக திரையில் தெரியும்.

#### வழிமுறை 2: பைதான் சர்வர் வழியாக இயக்குவது (Python Backend)
```bash
python app.py
```
பிரவுசரில் தானாகவே `http://localhost:5000` திறக்கப்படும். பைதான் சர்வர் தானாகவே COM போர்ட்டை கண்டறிந்து தகவல்களை ஸ்ட்ரீம் செய்யும்.

---

### 3. Landslide Alert Logic (எச்சரிக்கை விதிகள்)

1. **Safe (பாதுகாப்பானது - பச்சை நிறம்):**
   - மண் ஈரப்பதம் < 65%
   - மழை அளவு < 50%
   - நிலத்தின் சாய்வு கோணம் (Slope Tilt) < 15°

2. **Warning (கவனம் - மஞ்சள் நிறம்):**
   - மண் ஈரப்பதம் > 65% அல்லது மிதமான மழை அல்லது சாய்வு 15° - 25°

3. **Critical / Landslide Danger (மண் சரிவு அபாயம் - ஒளிரும் சிவப்பு நிறம் & அலாரம் சத்தம்):**
   - நீர் நிரம்பிய மண் (Moisture > 75%) + கனமழை (Rain > 60%) + திடீர் நில சாய்வு/அதிர்வு (> 25°).
   - இந்த நிலையில் திரையில் அலர்ட் ஒளிர்ந்து Audio Buzzer ஒலிக்கும்.

---

### 4. Arduino Code
- Arduino கோப்பு: `arduino/landslide_sensor_node.ino`
- Arduino IDE-ல் திறந்து உங்கள் போர்டில் Upload செய்யவும்.
- Baud Rate: **115200** (Dashboard-லும் 115200 வைத்துள்ளதை உறுதி செய்யவும்).
