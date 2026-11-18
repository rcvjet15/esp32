/*
 * MAX98357A Web Player Test — ESP32-S3 (SD card)
 *
 * Serves a web page on the ESP32 to audition the fireplace sounds:
 *   - dropdown to pick a sound (scanned from the SD card at boot)
 *   - volume slider (0-100%)
 *   - Start / Stop buttons
 *
 * Sounds are MP3s on a FAT32 microSD card, read over SPI and decoded with
 * ESP8266Audio (libmad) -> I2S -> MAX98357A. When a clip finishes it loops, so
 * you can leave one running as ambience. No LittleFS/partition juggling — just
 * copy MP3s to the SD card's root and insert it.
 *
 * Wiring (MAX98357A -> ESP32-S3) — same as the other test scripts:
 *   BCLK -> GPIO5
 *   LRC  -> GPIO4
 *   DIN  -> GPIO18
 *   VIN  -> 3.3V (NOT 5V — 5V makes the amp ignore 3.3V logic)
 *   GND  -> GND
 *   SD   -> 3.3V (enable)
 *   GAIN -> 3.3V for +15 dB (GND = +6 dB; floating = +3 dB, the module's
 *           100 kΩ pull-down gives the +9 dB default)
 *
 * Wiring (microSD TF module -> ESP32-S3) — SPI, no clash with the I2S pins:
 *   CS   -> GPIO10
 *   SCK  -> GPIO12
 *   MOSI -> GPIO11
 *   MISO -> GPIO13
 *   3v3  -> 3.3V (small "3v3" module — do NOT feed it 5V)
 *   GND  -> GND
 *   (only the LARGER module labeled VCC has an onboard regulator and takes 5V)
 *
 * Compile/upload:
 *   arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc .
 *   arduino-cli upload  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc -p /dev/cu.usbmodem* .
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <esp_system.h>

#include "AudioFileSourceSD.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#define I2S_BCLK_PIN  5
#define I2S_LRC_PIN   4
#define I2S_DOUT_PIN  18

#define SD_CS    10
#define SD_SCK   12
#define SD_MISO  13
#define SD_MOSI  11

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
AudioFileSource *file = nullptr;    // raw SD source, created per play
AudioFileSourceBuffer *buffered = nullptr;  // read-ahead buffer wrapping `file`

bool playing = false;
String currentSound = "";
float volume = 0.8f;  // 0.0 .. 1.0

void stopPlayback() {
  if (gen != nullptr) {
    gen->stop();
    delete gen;
    gen = nullptr;
  }
  if (buffered != nullptr) {
    delete buffered;
    buffered = nullptr;
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
  if (!SD.exists(path)) {
    Serial.printf("ERROR: %s not found on SD\n", path.c_str());
    return false;
  }

  file = new AudioFileSourceSD(path.c_str());
  buffered = new AudioFileSourceBuffer(file, 16384);
  gen = new AudioGeneratorMP3();
  if (!gen->begin(buffered, &out)) {
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
  File root = SD.open("/");
  if (root) {
    File f = root.openNextFile();
    while (f) {
      String n = String(f.name());
      if (n.startsWith("/")) n = n.substring(1);
      if (n.endsWith(".mp3")) {
        // 128 kbps CBR MP3 ~= 16000 bytes/sec -> derive duration from size.
        uint32_t sec = (uint32_t)(f.size() / 16000);
        if (!first) json += ",";
        json += "{\"n\":\"" + n + "\",\"s\":" + String(sec) + "}";
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
const fmt = (s) => s < 60 ? (s + 's') : (Math.floor(s/60) + 'm ' + (s%60) + 's');

function playSound(f) {
  setStatus('starting ' + f + '...');
  fetch('/play?f=' + encodeURIComponent(f))
    .then(r => r.json())
    .then(j => setStatus(j.ok ? ('playing ' + f) : ('error: ' + j.error)));
}

function loadList() {
  fetch('/list').then(r => r.json()).then(items => {
    const sel = $('sound');
    sel.innerHTML = '';
    items.forEach(it => {
      const o = document.createElement('option');
      o.value = it.n;
      o.textContent = it.n.replace(/\.mp3$/, '') + ' (' + fmt(it.s) + ')';
      sel.appendChild(o);
    });
    setStatus(items.length ? (items.length + ' sounds') : 'no sounds found');
  }).catch(() => setStatus('failed to load sound list'));
}

$('sound').onchange = () => { const f = $('sound').value; if (f) playSound(f); };
$('start').onclick = () => { const f = $('sound').value; if (f) playSound(f); };

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
  Serial.println("\nMAX98357A web player test (SD card)");
  Serial.printf("Reset reason: %d\n", esp_reset_reason());

  // Audio output: I2S -> MAX98357A (same pins/wiring as the other test scripts)
  out.SetPinout(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
  out.SetOutputModeMono(true);   // mono stream to the single speaker
  out.SetGain(volume);
  Serial.printf("Pins: BCLK=%d LRC=%d DIN=%d\n", I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);

  // SD card with the MP3s (retry a few times — an SD card can be wedged after
  // a brownout/power glitch and needs a moment to recover).
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  bool sd_ok = false;
  for (int attempt = 1; attempt <= 5; attempt++) {
    if (SD.begin(SD_CS)) {
      sd_ok = true;
      break;
    }
    Serial.printf("SD init attempt %d/5 failed, retrying...\n", attempt);
    delay(1000);
  }
  if (!sd_ok) {
    Serial.println("FAILED: SD card init (check wiring: CS=10 SCK=12 MOSI=11 MISO=13)");
    while (1) delay(1000);
  }
  Serial.printf("SD card OK: %llu total / %llu used bytes\n",
                SD.totalBytes(), SD.usedBytes());

  // Let the SD card settle before continuing.
  delay(500);

  // Wi-Fi (STA, fall back to AP). Start it BEFORE server.begin() — the socket
  // in server.begin() needs lwIP's core mutex, which is NULL until Wi-Fi
  // initializes the network stack, and locking NULL panics the chip.
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);

  // Web routes (after Wi-Fi so the socket can be created)
  server.on("/", handleRoot);
  server.on("/list", sendJsonList);
  server.on("/play", handlePlay);
  server.on("/stop", handleStop);
  server.on("/volume", handleVolume);
  server.on("/status", handleStatus);
  server.begin();

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
