#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mbedtls/aes.h>
#include "Audio.h"

#if __has_include("config.h")
    #include "config.h"
#else
    #include "config.example.h"
#endif

static const uint32_t IDLE_POLL_INTERVAL_MS = 2000;
static const uint32_t IDLE_STATUS_INTERVAL_MS = 3000;
static const uint32_t WIFI_RETRY_MS = 10000;
static const uint32_t CONTROL_HTTP_TIMEOUT_MS = 1500;
static const byte DNS_PORT = 53;
static const IPAddress PROVISIONING_IP(192, 168, 4, 1);
static const char AUXIOT_HTTP_USER_AGENT[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) AuxIoT/1.0 Safari/537.36";

Audio audio;
Preferences prefs;
WiFiClient plainClient;
WiFiClientSecure secureClient;
WebServer provisioningServer(80);
DNSServer dnsServer;

uint32_t lastCommandId = 0;
uint32_t lastPollMs = 0;
uint32_t lastStatusMs = 0;
uint32_t lastWifiTryMs = 0;
uint32_t lastSerialStatusMs = 0;

String wifiSsid;
String wifiPass;
String serialInput = "";
String currentAction = "boot";
String currentTitle = "";
String currentUrl = "";
String lastError = "";
String infinityCookie = AUXIOT_INFINITYFREE_COOKIE;
String customAudioHeaders = "";
uint8_t currentVolume = AUXIOT_DEFAULT_VOLUME;
bool provisioningActive = false;
bool restartAfterProvisioning = false;
uint32_t restartAtMs = 0;

struct QueuedCommand {
    bool ready = false;
    uint32_t commandId = 0;
    String action = "stop";
    String url = "";
    String title = "";
    int volume = AUXIOT_DEFAULT_VOLUME;
};

SemaphoreHandle_t commandMutex = nullptr;
QueuedCommand queuedCommand;

String stateUrl() {
    return String(AUXIOT_BASE_URL) + "/api.php?action=state&i=1";
}

String statusUrl() {
    return String(AUXIOT_BASE_URL) + "/api.php?action=device_status&i=1";
}

void applyAudioHttpHeaders() {
    customAudioHeaders = "";
    if (infinityCookie.length() > 0) {
        customAudioHeaders += "Cookie: " + infinityCookie + "\r\n";
    }
    audio.setCustomHTTPHeader(customAudioHeaders.c_str());
}

String resolveUrl(const String &url) {
    if (url.startsWith("http://aiotamp.rf.gd/uploads/")) {
        return "https://" + url.substring(7);
    }
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return url;
    }
    if (url.startsWith("/uploads/")) {
        return String(AUXIOT_STREAM_BASE_URL) + url;
    }
    if (url.startsWith("/")) {
        return String(AUXIOT_BASE_URL) + url;
    }
    return "";
}

bool beginHttp(HTTPClient &http, const String &url) {
    if (url.startsWith("https://")) {
        return http.begin(secureClient, url);
    }
    return http.begin(plainClient, url);
}

void addApiHeaders(HTTPClient &http) {
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Pragma", "no-cache");
    if (infinityCookie.length() > 0) {
        http.addHeader("Cookie", infinityCookie);
    }
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

void hexToBytes(const String &hex, uint8_t *out, size_t outLen) {
    for (size_t i = 0; i < outLen; i++) {
        out[i] = (hexNibble(hex[i * 2]) << 4) | hexNibble(hex[i * 2 + 1]);
    }
}

String bytesToHex(const uint8_t *bytes, size_t len) {
    const char *digits = "0123456789abcdef";
    String hex = "";
    hex.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        hex += digits[(bytes[i] >> 4) & 0x0F];
        hex += digits[bytes[i] & 0x0F];
    }
    return hex;
}

String nthToNumbersHex(const String &html, int wantedIndex) {
    int from = 0;
    for (int i = 0; i <= wantedIndex; i++) {
        int start = html.indexOf("toNumbers(\"", from);
        if (start < 0) return "";
        start += 11;
        int end = html.indexOf("\"", start);
        if (end < 0) return "";
        if (i == wantedIndex) {
            return html.substring(start, end);
        }
        from = end + 1;
    }
    return "";
}

bool updateInfinityCookieFromChallenge(const String &html) {
    if (html.indexOf("slowAES.decrypt") < 0 || html.indexOf("__test") < 0) {
        return false;
    }

    String keyHex = nthToNumbersHex(html, 0);
    String ivHex = nthToNumbersHex(html, 1);
    String cipherHex = nthToNumbersHex(html, 2);
    if (keyHex.length() != 32 || ivHex.length() != 32 || cipherHex.length() != 32) {
        lastError = "Challenge parse failed";
        return false;
    }

    uint8_t key[16];
    uint8_t iv[16];
    uint8_t input[16];
    uint8_t output[16];
    hexToBytes(keyHex, key, 16);
    hexToBytes(ivHex, iv, 16);
    hexToBytes(cipherHex, input, 16);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int keyResult = mbedtls_aes_setkey_dec(&aes, key, 128);
    int cryptResult = keyResult == 0 ? mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 16, iv, input, output) : keyResult;
    mbedtls_aes_free(&aes);

    if (cryptResult != 0) {
        lastError = "Challenge AES failed";
        return false;
    }

    infinityCookie = "__test=" + bytesToHex(output, 16);
    applyAudioHttpHeaders();
    Serial.print("[HTTP] Updated InfinityFree cookie: ");
    Serial.println(infinityCookie);
    return true;
}

void loadWifiCredentials() {
    prefs.begin("auxiot", true);
    wifiSsid = prefs.getString("ssid", AUXIOT_FALLBACK_WIFI_SSID);
    wifiPass = prefs.getString("pass", AUXIOT_FALLBACK_WIFI_PASS);
    prefs.end();

    if (wifiSsid == "YOUR_WIFI_NAME") {
        wifiSsid = "";
        wifiPass = "";
    }
}

void saveWifiCredentials(const String &ssid, const String &pass) {
    prefs.begin("auxiot", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
    wifiSsid = ssid;
    wifiPass = pass;
}

void enterProvisioningHint() {
    Serial.println();
    Serial.println("AuxIoT provisioning mode");
    Serial.print("Connect to WiFi SSID: ");
    Serial.println(AUXIOT_PROVISIONING_SSID);
    Serial.print("Password: ");
    Serial.println(AUXIOT_PROVISIONING_PASS);
    Serial.println("Open: http://192.168.4.1");
    Serial.println("Send: WIFI your-ssid your-password");
    Serial.println();
}

String htmlEscape(const String &value) {
    String escaped = "";
    escaped.reserve(value.length() + 8);
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (c == '&') escaped += F("&amp;");
        else if (c == '<') escaped += F("&lt;");
        else if (c == '>') escaped += F("&gt;");
        else if (c == '"') escaped += F("&quot;");
        else if (c == '\'') escaped += F("&#39;");
        else escaped += c;
    }
    return escaped;
}

String encryptionLabel(wifi_auth_mode_t type) {
    if (type == WIFI_AUTH_OPEN) return "Open";
    if (type == WIFI_AUTH_WEP) return "WEP";
    if (type == WIFI_AUTH_WPA_PSK) return "WPA";
    if (type == WIFI_AUTH_WPA2_PSK) return "WPA2";
    if (type == WIFI_AUTH_WPA_WPA2_PSK) return "WPA/WPA2";
    if (type == WIFI_AUTH_WPA2_ENTERPRISE) return "Enterprise";
    if (type == WIFI_AUTH_WPA3_PSK) return "WPA3";
    if (type == WIFI_AUTH_WPA2_WPA3_PSK) return "WPA2/WPA3";
    return "Secured";
}

String scanNetworkOptions() {
    String options = F("<option value=''>Manual / hidden network</option>");
    int count = WiFi.scanNetworks(false, true);
    if (count <= 0) {
        options += F("<option value='' disabled>No nearby networks found</option>");
        WiFi.scanDelete();
        return options;
    }

    for (int i = 0; i < count; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) {
            continue;
        }

        bool duplicate = false;
        for (int j = 0; j < i; j++) {
            if (WiFi.SSID(j) == ssid) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }

        options += F("<option value=\"");
        options += htmlEscape(ssid);
        options += F("\">");
        options += htmlEscape(ssid);
        options += F(" · ");
        options += String(WiFi.RSSI(i));
        options += F(" dBm · ");
        options += encryptionLabel(WiFi.encryptionType(i));
        options += F("</option>");
    }
    WiFi.scanDelete();
    return options;
}

String provisioningPage(const String &message = "") {
    String page = F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<title>AuxIoT WiFi Setup</title><style>");
    page += F(":root{color-scheme:dark;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#080a0f;color:#f6f7fb}");
    page += F("body{min-height:100vh;margin:0;display:grid;place-items:center;padding:22px;background:radial-gradient(circle at 20% 0,rgba(121,226,199,.22),transparent 26rem),#080a0f}");
    page += F("main{width:min(420px,100%);border:1px solid rgba(255,255,255,.12);border-radius:28px;padding:24px;background:rgba(18,22,31,.78);box-shadow:0 24px 80px rgba(0,0,0,.38);backdrop-filter:blur(22px)}");
    page += F(".logo{display:grid;place-items:center;width:52px;height:52px;border-radius:16px;background:linear-gradient(135deg,#79e2c7,#ffcf6e);color:#07100f;margin-bottom:16px}");
    page += F("h1{margin:0 0 6px;font-size:2rem;letter-spacing:0}p{margin:0 0 18px;color:#a8b0c0}label{display:grid;gap:7px;margin:14px 0;color:#a8b0c0}");
    page += F("input,select{height:48px;border:1px solid rgba(255,255,255,.14);border-radius:16px;padding:0 14px;background:rgba(255,255,255,.08);color:#fff;font:inherit;width:100%;box-sizing:border-box}");
    page += F("select option{background:#111721;color:#fff}.row{display:flex;gap:10px;align-items:center}.row a,.mini{display:grid;place-items:center;height:48px;min-width:52px;border:1px solid rgba(255,255,255,.14);border-radius:16px;color:#79e2c7;text-decoration:none;background:rgba(255,255,255,.07)}");
    page += F(".pass{display:flex;gap:10px}.pass input{min-width:0}.mini{width:auto;padding:0 12px;color:#f6f7fb}.qr{display:none;margin-top:12px}.qr.on{display:block}video{width:100%;border-radius:18px;background:#02040a}");
    page += F("button{width:100%;height:50px;border:0;border-radius:18px;margin-top:8px;background:#79e2c7;color:#07100f;font-weight:800;font:inherit}");
    page += F(".msg{border:1px solid rgba(121,226,199,.28);border-radius:16px;padding:12px;margin:0 0 16px;color:#dffaf3;background:rgba(121,226,199,.12)}.hint{font-size:.86rem;line-height:1.45;color:#8994a8;margin-top:-4px}");
    page += F("small{display:block;margin-top:14px;color:#778194}</style></head><body><main>");
    page += F("<div class='logo'><svg width='30' height='30' viewBox='0 0 24 24' fill='currentColor'><path d='M9 17.5a3 3 0 1 1-2-2.83V6.8L18 4v10.7a3 3 0 1 1-2-2.83V7.2l-7 1.76z'/></svg></div>");
    page += F("<h1>AuxIoT</h1><p>Connect your ESP32-S3 to your home WiFi.</p>");
    if (message.length() > 0) {
        page += "<div class='msg'>" + message + "</div>";
    }
    page += F("<form method='post' action='/save'><label>Nearby networks<div class='row'><select id='networkSelect'><option value='' disabled selected>Scanning nearby WiFi...</option>");
    page += scanNetworkOptions();
    page += F("</select><a href='/' title='Scan again'>↻</a></div></label>");
    page += F("<p class='hint'>Select your WiFi, or type the name manually below for hidden networks and scan exceptions.</p>");
    page += F("<label>WiFi name<input id='ssidInput' name='ssid' required maxlength='64' autocomplete='off' placeholder='Manual or selected WiFi name'></label>");
    page += F("<label>WiFi password<div class='pass'><input id='passInput' name='pass' type='password' maxlength='64'><button class='mini' id='showPass' type='button'>Show</button></div></label>");
    page += F("<button class='mini' id='scanQr' type='button'>Scan WiFi QR code</button><div class='qr' id='qrBox'><video id='qrVideo' playsinline></video><p class='hint' id='qrHint'>Point camera at a WiFi QR code.</p></div>");
    page += F("<button type='submit'>Save and restart</button></form>");
    page += F("<small>Setup network: ");
    page += AUXIOT_PROVISIONING_SSID;
    page += F(" / ");
    page += AUXIOT_PROVISIONING_PASS;
    page += F("</small><script>const s=document.getElementById('networkSelect'),i=document.getElementById('ssidInput'),p=document.getElementById('passInput'),sh=document.getElementById('showPass'),qr=document.getElementById('scanQr'),box=document.getElementById('qrBox'),vid=document.getElementById('qrVideo'),hint=document.getElementById('qrHint');s.addEventListener('change',()=>{if(s.value)i.value=s.value;i.focus();});sh.onclick=()=>{p.type=p.type==='password'?'text':'password';sh.textContent=p.type==='password'?'Show':'Hide';};function wifi(v){let m=v.match(/WIFI:(.*);;/i);if(!m)return false;let x=m[1],r={},a='',k='';for(let c of x){if(c==='\\\\'){a+='\\\\';continue}if(c===':'&&!k){k=a;a='';continue}if(c===';'){r[k]=a;k='';a='';continue}a+=c}if(r.S)i.value=r.S;if(r.P)p.value=r.P;return !!r.S}qr.onclick=async()=>{box.classList.add('on');try{if(!('BarcodeDetector'in window))throw Error('QR scanning is not supported by this browser.');let st=await navigator.mediaDevices.getUserMedia({video:{facingMode:'environment'}});vid.srcObject=st;await vid.play();let bd=new BarcodeDetector({formats:['qr_code']});let t=setInterval(async()=>{let codes=await bd.detect(vid);if(codes[0]&&wifi(codes[0].rawValue)){hint.textContent='WiFi QR loaded.';clearInterval(t);st.getTracks().forEach(x=>x.stop());}},700)}catch(e){hint.textContent=e.message||'Camera unavailable. Type WiFi manually.'}}</script></main></body></html>");
    return page;
}

void startProvisioningMode(const String &reason) {
    if (provisioningActive) {
        return;
    }

    audio.stopSong();
    provisioningActive = true;
    currentAction = "provisioning";
    lastError = reason;

    WiFi.disconnect(true, false);
    delay(200);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(PROVISIONING_IP, PROVISIONING_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AUXIOT_PROVISIONING_SSID, AUXIOT_PROVISIONING_PASS);

    dnsServer.start(DNS_PORT, "*", PROVISIONING_IP);
    provisioningServer.on("/", HTTP_GET, []() {
        provisioningServer.send(200, "text/html", provisioningPage());
    });
    provisioningServer.on("/save", HTTP_POST, []() {
        String ssid = provisioningServer.arg("ssid");
        String pass = provisioningServer.arg("pass");
        ssid.trim();
        pass.trim();
        if (ssid.length() == 0) {
            provisioningServer.send(422, "text/html", provisioningPage("WiFi name is required."));
            return;
        }

        saveWifiCredentials(ssid, pass);
        provisioningServer.send(200, "text/html", provisioningPage("Saved. AuxIoT is restarting now."));
        restartAfterProvisioning = true;
        restartAtMs = millis() + 1400;
    });
    provisioningServer.onNotFound([]() {
        provisioningServer.sendHeader("Location", "http://192.168.4.1/", true);
        provisioningServer.send(302, "text/plain", "");
    });
    provisioningServer.begin();

    enterProvisioningHint();
    Serial.print("[PROVISIONING] AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void connectWifiIfNeeded() {
    if (provisioningActive) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (wifiSsid.length() == 0) {
        currentAction = "provisioning";
        lastError = "No WiFi credentials";
        startProvisioningMode(lastError);
        return;
    }

    if (millis() - lastWifiTryMs < WIFI_RETRY_MS) {
        return;
    }

    lastWifiTryMs = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
}

void connectWifiBlocking() {
    if (wifiSsid.length() == 0) {
        startProvisioningMode("No WiFi credentials");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

    Serial.print("Connecting WiFi");
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI: ");
        Serial.println(WiFi.RSSI());
        lastError = "";
    } else {
        Serial.println("\nWiFi connect timed out");
        lastError = "WiFi connect timed out";
        startProvisioningMode(lastError);
    }
}

void stopSong() {
    Serial.println("[AUDIO] Stop");
    audio.stopSong();
    currentAction = "stop";
    currentTitle = "";
    currentUrl = "";
}

void playStream(const String &url, const String &title = "") {
    currentUrl = url;
    currentTitle = title;

    Serial.println("\n========== NEW STREAM ==========");
    Serial.print("URL: ");
    Serial.println(currentUrl);

    audio.stopSong();
    delay(500);

    Serial.println("[AUDIO] Connecting...");
    applyAudioHttpHeaders();
    bool ok = audio.connecttohost(currentUrl.c_str());

    Serial.print("[AUDIO] Result: ");
    Serial.println(ok ? "OK" : "FAILED");
    Serial.println("================================\n");

    if (ok) {
        currentAction = "play";
        lastError = "";
    } else {
        currentAction = "stop";
        lastError = "Unable to open stream";
    }
}

void printFullStatus() {
    Serial.println("\n========== STATUS ==========");
    Serial.print("Device: ");
    Serial.println(AUXIOT_DEVICE_ID);
    Serial.print("Action: ");
    Serial.println(currentAction);
    Serial.print("URL: ");
    Serial.println(currentUrl);
    Serial.print("Title: ");
    Serial.println(currentTitle);
    Serial.print("Volume: ");
    Serial.println(currentVolume);
    Serial.print("Command ID: ");
    Serial.println(lastCommandId);
    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("PSRAM: ");
    Serial.println(psramFound() ? "YES" : "NO");
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.print("IP: ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("-");
    }
    Serial.print("RSSI: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
    Serial.print("Last error: ");
    Serial.println(lastError);
    Serial.println("============================\n");
}

void handleCommand(String cmd) {
    cmd.trim();

    Serial.print("\n[SERIAL] ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("stop")) {
        stopSong();
        return;
    }

    if (cmd.equalsIgnoreCase("status")) {
        printFullStatus();
        return;
    }

    if (cmd.startsWith("vol ")) {
        int v = cmd.substring(4).toInt();
        currentVolume = constrain(v, 0, 21);
        audio.setVolume(currentVolume);
        currentAction = "volume";
        Serial.print("[AUDIO] Volume = ");
        Serial.println(currentVolume);
        return;
    }

    if (cmd.startsWith("WIFI ")) {
        int firstSpace = cmd.indexOf(' ');
        int secondSpace = cmd.indexOf(' ', firstSpace + 1);
        if (secondSpace < 0) {
            Serial.println("[ERROR] Use: WIFI your-ssid your-password");
            return;
        }

        String ssid = cmd.substring(firstSpace + 1, secondSpace);
        String pass = cmd.substring(secondSpace + 1);
        ssid.trim();
        pass.trim();
        if (ssid.length() == 0) {
            Serial.println("[ERROR] SSID cannot be empty");
            return;
        }

        saveWifiCredentials(ssid, pass);
        Serial.println("[WIFI] Credentials saved. Reconnecting...");
        WiFi.disconnect(true);
        delay(250);
        connectWifiBlocking();
        return;
    }

    if (cmd.startsWith("http://") || cmd.startsWith("https://")) {
        playStream(cmd, "Serial stream");
        return;
    }

    Serial.println("[ERROR] Unknown command");
}

void readSerial() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            serialInput.trim();
            if (serialInput.length() > 0) {
                handleCommand(serialInput);
                serialInput = "";
            }
        } else {
            serialInput += c;
        }
    }
}

bool fetchJson(const String &url, JsonDocument &doc, bool allowChallengeRetry = true) {
    connectWifiIfNeeded();
    if (WiFi.status() != WL_CONNECTED) {
        lastError = "WiFi offline";
        return false;
    }

    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(CONTROL_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    bool begun = beginHttp(http, url);
    if (!begun) {
        lastError = "HTTP begin failed";
        return false;
    }
    http.setUserAgent(AUXIOT_HTTP_USER_AGENT);
    addApiHeaders(http);

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        lastError = "GET failed: " + String(code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        if (allowChallengeRetry && updateInfinityCookieFromChallenge(payload)) {
            Serial.println("[HTTP] Retrying JSON request with challenge cookie");
            return fetchJson(url, doc, false);
        }
        lastError = "JSON parse failed";
        Serial.print("[HTTP] Non-JSON response: ");
        Serial.println(payload.substring(0, 180));
        return false;
    }

    lastError = "";
    return true;
}

bool postJson(const String &url, const String &body, bool allowChallengeRetry = true) {
    connectWifiIfNeeded();
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(audio.isRunning() ? CONTROL_HTTP_TIMEOUT_MS : 8000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    bool begun = beginHttp(http, url);
    if (!begun) {
        lastError = "POST begin failed";
        return false;
    }
    http.setUserAgent(AUXIOT_HTTP_USER_AGENT);
    http.addHeader("Content-Type", "application/json");
    addApiHeaders(http);
    int code = http.POST(body);
    String payload = http.getString();
    if (allowChallengeRetry && updateInfinityCookieFromChallenge(payload)) {
        http.end();
        Serial.println("[HTTP] Retrying POST with challenge cookie");
        return postJson(url, body, false);
    }
    if (code < 200 || code >= 300) {
        Serial.print("[HTTP] POST failed ");
        Serial.print(code);
        Serial.print(": ");
        Serial.println(payload.substring(0, 180));
    }
    http.end();
    return code >= 200 && code < 300;
}

bool executeCommandFields(uint32_t commandId, const String &action, const String &url, const String &title, int volume) {
    if (commandId == 0 || commandId == lastCommandId) {
        return false;
    }

    lastCommandId = commandId;
    currentVolume = constrain(volume, 0, 21);
    audio.setVolume(currentVolume);

    if (action == "play") {
        String resolved = resolveUrl(url);
        if (resolved.length() == 0) {
            lastError = "Invalid stream URL";
            stopSong();
            return true;
        }

        playStream(resolved, title);
        return true;
    }

    if (action == "pause" || action == "stop") {
        stopSong();
        return true;
    }

    if (action == "volume") {
        currentAction = "volume";
        return true;
    }

    if (action == "soft_reset") {
        stopSong();
        delay(150);
        ESP.restart();
    }

    if (action == "wifi_setup" || action == "factory_reset") {
        stopSong();
        prefs.begin("auxiot", false);
        prefs.clear();
        prefs.end();
        WiFi.disconnect(true, true);
        delay(250);
        ESP.restart();
    }

    return true;
}

bool queueCommand(JsonObject state) {
    uint32_t commandId = state["command_id"] | 0;
    if (commandId == 0 || commandId == lastCommandId || commandMutex == nullptr) {
        return false;
    }

    if (xSemaphoreTake(commandMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return false;
    }

    bool changed = !queuedCommand.ready || queuedCommand.commandId != commandId;
    if (changed) {
        queuedCommand.ready = true;
        queuedCommand.commandId = commandId;
        queuedCommand.action = String(state["action"] | "stop");
        queuedCommand.url = String(state["url"] | "");
        queuedCommand.title = String(state["title"] | "");
        queuedCommand.volume = state["volume"] | currentVolume;
    }

    xSemaphoreGive(commandMutex);
    return changed;
}

void applyQueuedCommand() {
    if (commandMutex == nullptr) {
        return;
    }

    QueuedCommand cmd;
    if (xSemaphoreTake(commandMutex, 0) != pdTRUE) {
        return;
    }

    if (queuedCommand.ready) {
        cmd = queuedCommand;
        queuedCommand.ready = false;
    }

    xSemaphoreGive(commandMutex);

    if (cmd.ready) {
        executeCommandFields(cmd.commandId, cmd.action, cmd.url, cmd.title, cmd.volume);
    }
}

void pollState() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<2048> doc;
#endif
    if (!fetchJson(stateUrl(), doc)) {
        return;
    }

    if (!(doc["ok"] | false)) {
        lastError = "State API error";
        return;
    }

    queueCommand(doc["state"].as<JsonObject>());
}

void sendStatus() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<1536> doc;
#endif
    doc["device_id"] = AUXIOT_DEVICE_ID;
    doc["ip_address"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
    doc["wifi_status"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["psram_status"] = psramFound() ? "available" : "missing";
    doc["command_id"] = lastCommandId;
    doc["action"] = currentAction;
    doc["track_title"] = currentTitle;
    doc["stream_url"] = currentUrl;
    doc["volume"] = currentVolume;
    doc["playback_state"] = audio.isRunning() ? "playing" : "stopped";
    doc["last_error"] = lastError;
    doc["firmware_version"] = AUXIOT_FIRMWARE_VERSION;
    doc["uptime_ms"] = millis();

    String body;
    serializeJson(doc, body);
    postJson(statusUrl(), body);
}

void controlPollTask(void *parameter) {
    (void) parameter;
    for (;;) {
        if (!provisioningActive && WiFi.status() == WL_CONNECTED) {
            pollState();
        }
        uint32_t delayMs = audio.isRunning() ? 6000 : IDLE_POLL_INTERVAL_MS;
        vTaskDelay(pdMS_TO_TICKS(delayMs));
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=================================");
    Serial.println("AuxIoT ESP32-S3 + PCM5102A");
    Serial.println("=================================");
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("PSRAM: ");
    Serial.println(psramFound() ? "YES" : "NO");

    secureClient.setInsecure();
    commandMutex = xSemaphoreCreateMutex();
    loadWifiCredentials();
    applyAudioHttpHeaders();
    connectWifiBlocking();

    audio.setPinout(AUXIOT_I2S_BCLK, AUXIOT_I2S_LRC, AUXIOT_I2S_DOUT);
    audio.setVolume(currentVolume);
    currentAction = "ready";

    Serial.println("\nCommands:");
    Serial.println("paste URL then Enter");
    Serial.println("vol 0-21");
    Serial.println("stop");
    Serial.println("status");
    Serial.println("WIFI your-ssid your-password");
    Serial.println();

    xTaskCreatePinnedToCore(
        controlPollTask,
        "auxiot-control",
        8192,
        nullptr,
        1,
        nullptr,
        0
    );
}

void loop() {
    readSerial();

    if (provisioningActive) {
        dnsServer.processNextRequest();
        provisioningServer.handleClient();
        if (restartAfterProvisioning && millis() >= restartAtMs) {
            ESP.restart();
        }
        return;
    }

    bool running = audio.isRunning();
    if (!running) {
        connectWifiIfNeeded();
    }
    audio.loop();
    applyQueuedCommand();

    if (audio.isRunning()) {
        return;
    }

    uint32_t now = millis();

    if (now - lastStatusMs >= IDLE_STATUS_INTERVAL_MS) {
        lastStatusMs = now;
        sendStatus();
    }

    if (!audio.isRunning() && now - lastSerialStatusMs >= 5000) {
        lastSerialStatusMs = now;
        Serial.print("[STATUS] heap=");
        Serial.print(ESP.getFreeHeap());
        Serial.print(" rssi=");
        Serial.println(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
    }
}

void audio_info(const char *info) {
    Serial.print("[INFO] ");
    Serial.println(info);
}

void audio_id3data(const char *info) {
    Serial.print("[ID3] ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info) {
    Serial.print("[EOF MP3] ");
    Serial.println(info);
}

void audio_showstation(const char *info) {
    Serial.print("[STATION] ");
    Serial.println(info);
}

void audio_showstreaminfo(const char *info) {
    Serial.print("[STREAM] ");
    Serial.println(info);
}

void audio_showstreamtitle(const char *info) {
    Serial.print("[TITLE] ");
    Serial.println(info);
}

void audio_bitrate(const char *info) {
    Serial.print("[BITRATE] ");
    Serial.println(info);
}

void audio_commercial(const char *info) {
    Serial.print("[COMMERCIAL] ");
    Serial.println(info);
}

void audio_icyurl(const char *info) {
    Serial.print("[ICY URL] ");
    Serial.println(info);
}

void audio_lasthost(const char *info) {
    Serial.print("[LAST HOST] ");
    Serial.println(info);
}
