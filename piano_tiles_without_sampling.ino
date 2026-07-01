#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "BluetoothA2DPSource.h"
#include <math.h>

// -------- CONFIG --------
const char* ssid = "H155-381_12BF";
const char* password = "hFBhN4Bm2E8";
const char* speakerName = "JBL Go 4";



// -------- GLOBALS --------
WebServer server(80);
WebSocketsServer ws(81);
BluetoothA2DPSource a2dp_source;

bool notesActive[12] = {false};
const int NOTE_FREQS[12] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
float phase[12] = {0};
bool bluetoothConnected = false;

// -------- HTML PAGE (index_html) --------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 Piano</title>
<style>
body{background:#1a1a2e;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;font-family:Arial;overflow:hidden}
.piano{display:flex;position:relative;height:200px;width:90vw;max-width:600px}
.key{border:1px solid #333;cursor:pointer;touch-action:none}
.key.white{flex:1;background:#fff;border-radius:0 0 8px 8px;z-index:1}
.key.white.active{background:#c0d8e0}
.key.black{position:absolute;width:28px;height:60%;background:#111;border-radius:0 0 4px 4px;z-index:10}
.key.black.active{background:#444}
.key[data-note="1"]{left:35px}.key[data-note="3"]{left:78px}
.key[data-note="6"]{left:165px}.key[data-note="8"]{left:208px}.key[data-note="10"]{left:251px}
.status{color:#aaa;margin-top:30px;font-size:14px}
.status.connected{color:#4ade80}.status.disconnected{color:#f87171}
</style>
</head>
<body>
<div style="text-align:center;width:100%">
<h2 style="color:#eee">ESP32 Piano</h2>
<div class="piano" id="piano">
<div class="key white" data-note="0"></div>
<div class="key black" data-note="1"></div>
<div class="key white" data-note="2"></div>
<div class="key black" data-note="3"></div>
<div class="key white" data-note="4"></div>
<div class="key white" data-note="5"></div>
<div class="key black" data-note="6"></div>
<div class="key white" data-note="7"></div>
<div class="key black" data-note="8"></div>
<div class="key white" data-note="9"></div>
<div class="key black" data-note="10"></div>
<div class="key white" data-note="11"></div>
</div>
<div class="status" id="status">Connecting...</div>
</div>
<script>
const ws = new WebSocket('ws://' + location.hostname + ':81/');
const NOTES = [262,277,294,311,330,349,370,392,415,440,466,494];
let activeKeys = new Set();

function noteOn(i) {
    if (activeKeys.has(i)) return;
    activeKeys.add(i);
    ws.send(JSON.stringify({type: 'note_on', note: i}));
    document.querySelector(`.key[data-note="${i}"]`).classList.add('active');
}

function noteOff(i) {
    if (!activeKeys.has(i)) return;
    activeKeys.delete(i);
    ws.send(JSON.stringify({type: 'note_off', note: i}));
    document.querySelector(`.key[data-note="${i}"]`).classList.remove('active');
}

function clearAllKeys() {
    activeKeys.forEach(i => {
        ws.send(JSON.stringify({type: 'note_off', note: i}));
        document.querySelector(`.key[data-note="${i}"]`).classList.remove('active');
    });
    activeKeys.clear();
}

// Get the note under a touch/click point
function getNoteFromPoint(x, y) {
    const elements = document.elementsFromPoint(x, y);
    for (let el of elements) {
        if (el.classList && el.classList.contains('key')) {
            const note = parseInt(el.dataset.note);
            if (!isNaN(note)) return note;
        }
    }
    return -1;
}

// ---- MOUSE EVENTS ----
let mouseDown = false;
document.querySelectorAll('.key').forEach(el => {
    const note = parseInt(el.dataset.note);
    
    el.addEventListener('mousedown', (e) => {
        e.preventDefault();
        mouseDown = true;
        noteOn(note);
    });
    
    el.addEventListener('mouseup', () => {
        mouseDown = false;
        clearAllKeys();
    });
    
    el.addEventListener('mouseleave', () => {
        if (mouseDown) noteOff(note);
    });
});

// ---- TOUCH EVENTS (Full sliding support) ----
const piano = document.getElementById('piano');

piano.addEventListener('touchstart', (e) => {
    e.preventDefault();
    const touch = e.touches[0];
    const note = getNoteFromPoint(touch.clientX, touch.clientY);
    if (note >= 0) noteOn(note);
}, { passive: false });

piano.addEventListener('touchmove', (e) => {
    e.preventDefault();
    const touch = e.touches[0];
    const note = getNoteFromPoint(touch.clientX, touch.clientY);
    if (note >= 0) {
        // If we're on a new note, turn off all others and play this one
        if (!activeKeys.has(note)) {
            clearAllKeys();
            noteOn(note);
        }
    }
}, { passive: false });

piano.addEventListener('touchend', (e) => {
    e.preventDefault();
    clearAllKeys();
}, { passive: false });

piano.addEventListener('touchcancel', (e) => {
    e.preventDefault();
    clearAllKeys();
}, { passive: false });

ws.onopen = () => {
    document.getElementById('status').textContent = 'Connected';
    document.getElementById('status').className = 'status connected';
};
ws.onclose = () => {
    document.getElementById('status').textContent = 'Disconnected';
    document.getElementById('status').className = 'status disconnected';
};
</script>
</body>
</html>
)rawliteral";

// -------- BLUETOOTH CONNECTION CALLBACK --------
void connection_state_changed(esp_a2d_connection_state_t state, void* ptr) {
    if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
        Serial.println("⚠️ Bluetooth speaker disconnected!");
        bluetoothConnected = false;
    } else if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        Serial.println("✅ Bluetooth speaker connected!");
        bluetoothConnected = true;
    }
}

// -------- AUDIO CALLBACK --------
int32_t get_sound_data(Frame* frame, int32_t frameCount) {
    memset(frame, 0, frameCount * sizeof(Frame));
    
    bool anyNoteActive = false;
    for (int n = 0; n < 12; n++) {
        if (notesActive[n]) { anyNoteActive = true; break; }
    }
    
    if (!anyNoteActive) {
        return frameCount;
    }
    
    const float sampleRate = 22050.0;
    const float volume = 0.3;
    
    for (int n = 0; n < 12; n++) {
        if (!notesActive[n]) continue;
        
        float freq = NOTE_FREQS[n];
        float inc = 2.0 * M_PI * freq / sampleRate;
        
        for (int i = 0; i < frameCount; i++) {
            phase[n] += inc;
            if (phase[n] > 2.0 * M_PI) phase[n] -= 2.0 * M_PI;
            int16_t sample = (int16_t)(32767 * volume * sin(phase[n]));
            frame[i].channel1 += sample;
            frame[i].channel2 += sample;
        }
    }
    
    return frameCount;
}

// -------- WEBSOCKET HANDLER --------
void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        int noteIndex = -1;
        
        if (msg.indexOf("\"note\"") > 0) {
            int start = msg.indexOf("\"note\"") + 7;
            int end = msg.indexOf(",", start);
            if (end == -1) end = msg.indexOf("}", start);
            noteIndex = msg.substring(start, end).toInt();
        }
        
        if (noteIndex >= 0 && noteIndex < 12) {
            if (msg.indexOf("note_on") > 0) {
                notesActive[noteIndex] = true;
            } else if (msg.indexOf("note_off") > 0) {
                notesActive[noteIndex] = false;
                phase[noteIndex] = 0;
            }
        }
    }
}

// -------- SETUP --------
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n🎹 ESP32 Piano Starting...");

    // WiFi STA
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("📡 Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());

    // WebServer
    server.on("/", []() { server.send(200, "text/html", index_html); });
    server.begin();

    // WebSocket
    ws.begin();
    ws.onEvent(wsEvent);

    // Bluetooth
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));
    a2dp_source.set_on_connection_state_changed(connection_state_changed);
    a2dp_source.start(speakerName, get_sound_data);

    Serial.println("\n🌐 Open: http://" + WiFi.localIP().toString());
}

// -------- LOOP --------
void loop() {
    server.handleClient();
    ws.loop();
    
    // Reconnection logic without .stop()
    static unsigned long lastRestart = 0;
    if (!bluetoothConnected && millis() - lastRestart > 30000) {
        lastRestart = millis();
        Serial.println("🔄 Restarting Bluetooth...");
        // Just restart the A2DP source by calling start again
        a2dp_source.start(speakerName, get_sound_data);
    }
}