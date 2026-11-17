"""
Landslide Monitoring System - Python Backend (Flask + PySerial)
மண் சரிவு முன்னெச்சரிக்கை அமைப்பு - பைதான் சேவையகம்
"""

import os
import time
import json
import threading
import serial
import serial.tools.list_ports
from flask import Flask, send_from_directory, jsonify, Response, request

app = Flask(__name__, static_folder='.', static_url_path='')

# Global state for latest sensor data
latest_data = {
    "soil": 0,
    "soil_raw": 0,
    "rain": 0,
    "rain_raw": 1023,
    "pitch": 0.0,
    "roll": 0.0,
    "risk": "NORMAL",
    "port": "None",
    "connected": False
}

serial_lock = threading.Lock()
current_serial_connection = None

def get_available_ports():
    ports = serial.tools.list_ports.comports()
    return [{"port": p.device, "desc": p.description} for p in ports]

def serial_reader_thread():
    global current_serial_connection, latest_data
    while True:
        try:
            if current_serial_connection is None or not current_serial_connection.is_open:
                # Try auto-detecting Arduino/ESP32 COM port
                ports = serial.tools.list_ports.comports()
                target_port = None
                for p in ports:
                    # Look for common USB-Serial chips (CH340, CP2102, FTDI, Arduino)
                    desc = p.description.lower()
                    if any(x in desc for x in ["ch340", "cp210", "ftdi", "arduino", "usb-serial", "com"]):
                        target_port = p.device
                        break
                
                if target_port:
                    try:
                        print(f"[*] Trying to connect to {target_port} at 115200 baud...")
                        ser = serial.Serial(target_port, 115200, timeout=2)
                        time.sleep(2) # Wait for Arduino reset
                        with serial_lock:
                            current_serial_connection = ser
                            latest_data["port"] = target_port
                            latest_data["connected"] = True
                        print(f"[+] Successfully connected to {target_port}")
                    except Exception as err:
                        # Try 9600 fallback
                        try:
                            ser = serial.Serial(target_port, 9600, timeout=2)
                            time.sleep(2)
                            with serial_lock:
                                current_serial_connection = ser
                                latest_data["port"] = target_port
                                latest_data["connected"] = True
                            print(f"[+] Successfully connected to {target_port} at 9600 baud")
                        except Exception as err2:
                            pass

            if current_serial_connection and current_serial_connection.is_open:
                line = current_serial_connection.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    if line.startswith('{') and line.endswith('}'):
                        try:
                            parsed = json.loads(line)
                            with serial_lock:
                                if "soil" in parsed:
                                    s = parsed["soil"]
                                    latest_data["soil"] = max(0, min(100, int(((4095 - s) / 3000) * 100))) if s > 100 else s
                                    latest_data["soil_raw"] = parsed.get("soil_raw", s)
                                if "rain" in parsed:
                                    r = parsed["rain"]
                                    latest_data["rain"] = max(0, min(100, int(((4095 - r) / 3000) * 100))) if r > 100 else r
                                    latest_data["rain_raw"] = parsed.get("rain_raw", r)
                                if "pitch" in parsed: latest_data["pitch"] = float(parsed["pitch"])
                                if "tilt" in parsed: latest_data["pitch"] = float(parsed["tilt"])
                                if "roll" in parsed: latest_data["roll"] = float(parsed["roll"])
                                if "temp" in parsed: latest_data["temperature"] = float(parsed["temp"])
                                if "status" in parsed: latest_data["risk"] = parsed["status"]
                                latest_data["connected"] = True
                        except Exception as json_err:
                            pass
            else:
                time.sleep(2)
        except Exception as e:
            with serial_lock:
                latest_data["connected"] = False
                if current_serial_connection:
                    try:
                        current_serial_connection.close()
                    except:
                        pass
                    current_serial_connection = None
            time.sleep(2)

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

@app.route('/api/esp_proxy')
def esp_proxy():
    import urllib.request
    ip = request.args.get('ip', '192.168.4.1')
    try:
        url = f"http://{ip}/api/data"
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=2) as response:
            data = response.read().decode('utf-8')
            return Response(data, mimetype="application/json")
    except Exception as e:
        return jsonify({"error": str(e), "connected": False}), 502

@app.route('/api/data')
def get_sensor_data():
    with serial_lock:
        return jsonify(latest_data)

@app.route('/api/stream')
def sse_stream():
    def event_stream():
        while True:
            with serial_lock:
                data_str = json.dumps(latest_data)
            yield f"data: {data_str}\n\n"
            time.sleep(1)
    return Response(event_stream(), mimetype="text/event-stream")

if __name__ == '__main__':
    # Start background serial listener thread
    t = threading.Thread(target=serial_reader_thread, daemon=True)
    t.start()

    print("=" * 60)
    print(" Landslide Monitoring System Dashboard Started!")
    print(" மண் சரிவு கண்காணிப்பு டேஷ்போர்டு துவங்கப்பட்டது!")
    print(" Browser URL: http://localhost:5000")
    print("=" * 60)
    app.run(host='0.0.0.0', port=5000, debug=False)
