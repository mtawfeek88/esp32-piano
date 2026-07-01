# 🎹 ESP32 Piano

A web-controlled piano that turns your ESP32 into a Bluetooth speaker instrument. Play melodies from your phone or laptop using a touch-friendly keyboard interface.

---

## 📋 Features

- 🎵 **12-key piano** with touch and mouse support
- 🔊 **Bluetooth A2DP** streaming to any Bluetooth speaker (JBL, Sony, Anker, etc.)
- 📱 **Mobile-friendly** web interface with smooth sliding
- 🔗 **Real-time control** via WebSocket
- 📡 **WiFi connectivity** (no cloud dependency)

---

## 🛠️ Hardware Requirements

| Component | Recommended Model |
|-----------|-------------------|
| **ESP32** | ESP32-WROOM, ESP32-DevKitC, or any ESP32 with Bluetooth Classic |
| **Bluetooth Speaker** | JBL Go 4, Anker Soundcore, Sony XB13, etc. |
| **Power** | Micro-USB cable + power source |

---

## 💻 Software Requirements

| Tool | Version |
|------|---------|
| Arduino IDE | 1.8.19+ |
| ESP32 Board Package | 2.0.17+ |

### 📦 Required Libraries

Install these from Arduino IDE → **Sketch → Include Library → Manage Libraries...**

| Library        | Author         | Purpose                    |
|----------------|----------------|----------------------------|
| **WebSockets** | Markus Sattler | WebSocket communication    |
| **ESP32-A2DP** | pschatzmann    | Bluetooth audio streaming  |

---

## 🚀 Setup Guide

### 1. Clone or Download



git clone https://github.com/YOUR_USERNAME/esp32-piano.git
cd esp32-piano

2. Open the Sketch

    Open esp32_piano.ino in Arduino IDE

3. Configure WiFi and Speaker

    Find these lines in the code:
      const char* ssid = "YOUR_WIFI_NAME";
      const char* password = "YOUR_WIFI_PASSWORD";
      const char* speakerName = "JBL Go 4";
    Replace with your WiFi credentials and speaker name

4. Install Required Libraries

   # Via Arduino IDE Library Manager:
   # 1. WebSockets by Markus Sattler
   # 2. ESP32-A2DP by pschatzmann
5. Select Board & Upload

    Tools → Board → ESP32 Dev Module

    Tools → Port → [Your COM Port]

    Click Upload

6. Find the IP Address

    Open Serial Monitor (115200 baud)

    Look for:
   ✅ Connected! IP: 192.168.x.x
   🌐 Open: http://192.168.x.x

7. Play!

    Connect your phone/laptop to the same WiFi network

    Open your browser and go to: http://[ESP32_IP]

    Tap the keys and enjoy!

How to Play
Action	Effect
Tap a key	Play a note
Slide across keys	Play a glissando (smooth sliding)
Lift finger	Stop all notes
Click + drag (mouse)	Same as touch

┌─────────────────────────────────────────────────────────────┐
│                    LOCAL NETWORK (WiFi)                    │
│                                                             │
│  ┌─────────────┐                                           │
│  │   Phone     │──┐                                        │
│  │  (Browser)  │  │     ┌─────────────┐                    │
│  └─────────────┘  ├────► │    ESP32    │                    │
│                   │     │  (WebServer)│                    │
│  ┌─────────────┐  │     └──────┬──────┘                    │
│  │   Laptop    │──┘            │                           │
│  │  (Browser)  │               │                           │
│  └─────────────┘               │                           │
│                                 │                           │
│                           Bluetooth A2DP                   │
│                                 │                           │
│                                 ▼                           │
│                          ┌─────────────┐                    │
│                          │   JBL Go 4  │                    │
│                          │  (Speaker)  │                    │
│                          └─────────────┘                    │
└─────────────────────────────────────────────────────────────┘

Troubleshooting
Speaker not connecting?

    Put your speaker in pairing mode (flashing light)

    Make sure the speaker name in the code matches exactly

    Restart the ESP32 after pairing the speaker to your phone once (resets cache)

Page not loading?

    Ensure your phone/laptop is on the same WiFi network

    Try http://[ESP32_IP]:80 (explicit port)

    Check firewall settings (Windows: set network to "Private")

Audio glitching?

    Reduce the sampleRate in get_sound_data() (22050 is stable)

    Ensure only one device is connected to the speaker

Credits

    ESP32-A2DP Library – pschatzmann

    WebSockets Library – Markus Sattler

    Built with ❤️ using Arduino IDE
    
