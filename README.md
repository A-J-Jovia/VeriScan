# VeriScan: Deep-Tech Multispectral Pill Authentication

**Domain:** IoT, Edge AI, Optical Physics, Healthcare Security  

## 📖 Executive Summary

VeriScan is a non-destructive hardware and software system designed to detect counterfeit pharmaceutical pills in under 5 seconds. By utilizing a **45/0 optical geometry** and an **18-channel multispectral optical sensor (AS7343)**, the system captures the unique chemical reflectance fingerprint of a pill.

Unlike basic colorimeters, VeriScan utilizes **Differential Optical Reading** (subtracting thermal/ambient dark current at the Edge) and **Row-wise L1 Normalization** (in the Cloud) to eliminate spatial placement variance and isolate the pure chemical signature for Machine Learning classification.

---

## 🏗️ System Architecture

VeriScan is a distributed IoT system divided into three distinct operational domains:

### 1. The Edge (Hardware & Firmware)

* **Microcontroller:** ESP32 DevKit V1 (C++)
* **Sensor:** Adafruit AS7343 (I2C)
* **Chassis:** Custom 5mm matte black PVC sunboard enclosure designed for absolute light isolation and strict 45-degree LED illumination.
* **Firmware Protocol:** Calculates differential (Light - Dark) readings and transmits a packed 36-byte binary struct (Little-Endian) over BLE to avoid MTU bottlenecks and heap fragmentation.

### 2. The Gateway (Mobile App)

* **Framework:** Flutter / Dart
* **Function:** Connects via BLE (MTU negotiated > 128 bytes), unpacks the 36-byte binary payload into an 18-channel integer array, and acts as the HTTP bridge to the inference engine.

### 3. The Brain (Backend & AI)

* **Server:** Python / FastAPI
* **Engine:** Scikit-Learn `Pipeline` (`Normalizer(norm='l1')` -> `RandomForestClassifier`)
* **Pipeline Logic:** Drops saturated hardware channels (CH6, CH12, CH18), normalizes the remaining 15-channel spectrum to extract the chemical ratio, and returns a strict "Real" or "Fake" verdict.

---

## 📂 Repository Structure

```text
VeriScan/
├── firmware/                 # C++ Edge Code (PlatformIO / Arduino IDE)
│   └── src/main.cpp          # BLE & AS7343 Differential Reading Logic
├── backend/                  # Python API and ML Pipeline
│   ├── api.py                # FastAPI Inference Server
│   ├── model.py              # ML Training Script (Run to generate .pkl)
│   ├── requirements.txt      # Python dependencies
│   └── veriscan_pipeline.pkl # Compiled Scikit-Learn Pipeline
├── mobile_app/               # Flutter Gateway (BLE to HTTP bridge)
│   └── lib/                  # Dart source code
└── README.md
```

## 🚀 Quick Start & Reproduction

### Phase 1: Backend Setup

1. Navigate to the backend directory: `cd backend/`

2. Install dependencies: `pip install -r requirements.txt`

3. Generate the ML Pipeline: `python model.py` (Requires a valid `scans.csv` dataset in the folder)

4. Boot the Inference API: `uvicorn api:app --host 0.0.0.0 --port 8000`

5. Verify API is alive at: <http://localhost:8000/docs>

### Phase 2: Hardware Flashing

 1. Open `firmware/src/main.cpp` in your IDE.

 2. Ensure you have the `Adafruit_AS7341` (or AS7343 compatible) library installed.

 3. Flash to the ESP32.

 4. *Note: Ensure your IR LEDs are properly resisted to prevent CH18 saturation before gathering production data.*

 ### Phase 3: Mobile Gateway

 1. Navigate to `mobile_app/` and run flutter pub get.

 2. Update the `api_service.dart` file with the local IP address or Cloud URL of your FastAPI backend.

 3. Compile and run on a physical mobile device (BLE does not work on emulators): `flutter run` 

## ⚠️ Known Hardware Constraints (Pending V3)

* Saturated NIR Band: Depending on LED drive current, the Near-IR band (CH18) may hit ADC maximums (59998). Current backend logic strictly drops this channel to maintain ML integrity until LED dimming/resistor tuning is applied.
