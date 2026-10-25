/*
 * MAX98357A Web Player Test — ESP32-S3
 *
 * Serves a web page on the ESP32 to audition the fireplace sounds:
 *   - dropdown to pick a sound (scanned from LittleFS at boot)
 *   - volume slider (0-100%)
 *   - Start / Stop buttons
 *
 * Sounds are the mono 16 kHz / 64 kbps MP3s from ../sounds/, flashed into a
 * LittleFS partition (see partitions.csv + upload_fs.sh). Playback decodes MP3
 * with ESP8266Audio (libmad) and outputs over I2S to the MAX98357A. When a clip
 * finishes it loops, so you can leave one running as ambience.
 *
 * Wiring (MAX98357A -> ESP32-S3) — same as the other test scripts:
 *   BCLK -> GPIO5
 *   LRC  -> GPIO4
 *   DIN  -> GPIO18
 *   VIN  -> 3.3V (NOT 5V — 5V makes the amp ignore 3.3V logic)
 *   GND  -> GND
 *   SD   -> 3.3V (enable)
 *   GAIN -> floating (default +9 dB)
 *
 * Compile/upload (needs a 3MB app partition; partitions.csv provides the FS):
 *   arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB .
 *   arduino-cli upload  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB -p /dev/cu.usbmodem* .
 *   ./upload_fs.sh   # build + flash the LittleFS image from ../../sounds/
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "AudioFileSourceFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#define I2S_BCLK_PIN  5
#define I2S_LRC_PIN   4
#define I2S_DOUT_PIN  18

// ---- Wi-Fi -----------------------------------------------------------------
// STA first; if it can't connect within the timeout, fall back to an AP so you
// can still reach the page (connect to "fireplace-test" and open 192.168.4.1).
const char *WIFI_SSID     = "HT_910549";
const char *WIFI_PASS     = "43919240156286395692";
const char *AP_SSID       = "fireplace-test";
const unsigned long WIFI_TIMEOUT_MS = 15000;

WebServer server(80);

// ---- audio -----------------------------------------------------------------
AudioOutputI2S out;                 // one output, kept for the lifetime
AudioGeneratorMP3 *gen = nullptr;   // created per play
AudioFileSourceFS *file = nullptr;  // created per play

bool playing = false;
String currentSound = "";
float volume = 0.8f;  // 0.0 .. 1.0

void stopPlayback() {
  if (gen != nullptr) {
    gen->stop();
    delete gen;
    gen = nullptr;
  }
  if (file != nullptr) {
    file->close();
    delete file;
    file = nullptr;
  }
  playing = false;
}

bool startPlayback(const String &name) {
  stopPlayback();

  String path = "/" + name;
  if (!LittleFS.exists(path)) {
    Serial.printf("ERROR: %s not found in LittleFS\n", path.c_str());
    return false;
  }

  file = new AudioFileSourceFS(LittleFS, path.c_str());
  gen = new AudioGeneratorMP3();
  if (!gen->begin(file, &out)) {
    Serial.println("ERROR: AudioGeneratorMP3 begin failed");
    stopPlayback();
    return false;
  }

  out.SetGain(volume);
  playing = true;
  currentSound = name;
  Serial.printf("playing %s\n", name.c_str());
  return true;
}

// ---- web handlers ----------------------------------------------------------

void sendJsonList() {
  String json = "[";
  bool first = true;
  File root = LittleFS.open("/");
  if (root) {
    File f = root.openNextFile();
    while (f) {
      String n = String(f.name());
      if (n.startsWith("/")) n = n.substring(1);
      if (n.endsWith(".mp3")) {
        if (!first) json += ",";
        json += "\"" + n + "\"";
        first = false;
      }
      f = root.openNextFile();
    }
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handlePlay() {
  String f = server.arg("f");
  if (f.length() == 0 || !f.endsWith(".mp3")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad filename\"}");
    return;
  }
  bool ok = startPlayback(f);
  server.send(ok ? 200 : 404, "application/json",
              ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"not found\"}");
}

void handleStop() {
  stopPlayback();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleVolume() {
  if (server.hasArg("v")) {
    int v = server.arg("v").toInt();
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    volume = v / 100.0f;
    out.SetGain(volume);
  }
  server.send(200, "application/json",
              "{\"ok\":true,\"volume\":" + String((int)(volume * 100)) + "}");
}

void handleStatus() {
  server.send(200, "application/json",
              "{\"playing\":" + String(playing ? "true" : "false") +
              ",\"current\":\"" + currentSound + "\"" +
              ",\"volume\":" + String((int)(volume * 100)) + "}");
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Fireplace Sound Player</title>
<style>
  body { font-family: -apple-system, system-ui, sans-serif; background:#1a120c;
         color:#f3e6d6; max-width:420px; margin:40px auto; padding:0 20px; }
  h1 { font-size:22px; }
  label { display:block; margin:16px 0 4px; font-size:13px; opacity:.7; }
  select, button { width:100%; box-sizing:border-box; font-size:17px; padding:12px;
         border-radius:8px; border:none; }
  select { background:#2a2018; color:#f3e6d6; }
  button { margin-top:16px; font-weight:600; cursor:pointer; }
  #start { background:#e0872a; color:#1a120c; }
  #stop  { background:#3a2c20; color:#f3e6d6; }
  input[type=range] { width:100%; accent-color:#e0872a; }
  #status { margin-top:20px; font-size:14px; opacity:.8; min-height:1.2em; }
</style>
</head>
<body>
<h1>Fireplace Sound Player</h1>

<label for="sound">Sound</label>
<select id="sound"></select>

<label for="vol">Volume</label>
<input type="range" id="vol" min="0" max="100" value="80">

<button id="start">Start</button>
<button id="stop">Stop</button>

<div id="status"></div>

<script>
const $ = (id) => document.getElementById(id);
const setStatus = (t) => { $('status').textContent = t; };

function loadList() {
  fetch('/list').then(r => r.json()).then(names => {
    const sel = $('sound');
    sel.innerHTML = '';
    names.forEach(n => {
      const o = document.createElement('option');
      o.value = n; o.textContent = n;
      sel.appendChild(o);
    });
    setStatus(names.length ? (names.length + ' sounds') : 'no sounds found');
  }).catch(() => setStatus('failed to load sound list'));
}

$('start').onclick = () => {
  const f = $('sound').value;
  if (!f) return;
  setStatus('starting ' + f + '...');
  fetch('/play?f=' + encodeURIComponent(f))
    .then(r => r.json())
    .then(j => setStatus(j.ok ? ('playing ' + f) : ('error: ' + j.error)));
};

$('stop').onclick = () => {
  fetch('/stop').then(() => setStatus('stopped'));
};

$('vol').oninput = () => {
  const v = $('vol').value;
  fetch('/volume?v=' + v);
};

function pollStatus() {
  fetch('/status').then(r => r.json()).then(j => {
    if (j.playing) setStatus('playing ' + j.current);
    else if (!$('status').textContent.startsWith('error')) { /* keep transient msgs */ }
  }).catch(() => {});
}
loadList();
setInterval(pollStatus, 1500);
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// ---- setup -----------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nMAX98357A web player test");

  // Audio output: I2S -> MAX98357A (same pins/wiring as the other test scripts)
  out.SetPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
  out.SetOutputModeMono(true);   // mono stream to the single speaker
  out.SetGain(volume);
  Serial.printf("Pins: BCLK=%d LRC=%d DIN=%d\n", I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);

  // Filesystem with the MP3s
  if (!LittleFS.begin(true)) {
    Serial.println("FAILED: LittleFS mount (did you run upload_fs.sh?)");
    while (1) delay(1000);
  }
  Serial.printf("LittleFS: %u / %u bytes used\n",
                LittleFS.usedBytes(), LittleFS.totalBytes());

  // Web routes
  server.on("/", handleRoot);
  server.on("/list", sendJsonList);
  server.on("/play", handlePlay);
  server.on("/stop", handleStop);
  server.on("/volume", handleVolume);
  server.on("/status", handleStatus);
  server.begin();

  // Wi-Fi (STA, fall back to AP)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWiFi OK, page: http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi failed, starting AP");
    WiFi.softAP(AP_SSID);
    Serial.printf("Connect to '%s' and open http://%s\n", AP_SSID,
                  WiFi.softAPIP().toString().c_str());
  }
}

// ---- loop ------------------------------------------------------------------

void loop() {
  server.handleClient();

  if (playing && gen != nullptr) {
    if (gen->isRunning()) {
      if (!gen->loop()) {
        Serial.println("playback error, stopping");
        stopPlayback();
      }
    } else {
      // Clip finished -> loop it for continuous ambience.
      String n = currentSound;
      stopPlayback();
      startPlayback(n);
    }
  }

  delay(1);
}
