#include "WifiPortal.h"
#include "portal_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_random.h>
#include <esp_sntp.h>

namespace {
WebServer server(80);
DNSServer dns;
bool wifiBusy = false;
int wifiCount = 0;
struct SavedNetwork { char ssid[33]; char password[64]; };

// A fixed AP channel keeps the phone attached while the radio scans. Channel 1
// is safe because the station stays parked while the portal is open.
constexpr int PORTAL_AP_CHANNEL = 1;
constexpr uint32_t VERIFY_TIMEOUT_MS = 20000;
// Let the browser receive the response page before the AP goes down.
constexpr uint32_t SAVE_RESPONSE_GRACE_MS = 700;
constexpr uint32_t RECONNECT_INTERVAL_MS = 10000;
}

WifiPortal::WifiPortal(const WifiPortalConfig &config) : _config(config) {}

String WifiPortal::_jsonStr(const String &v) {
    String out = "\"";
    for (unsigned i = 0; i < v.length(); ++i) {
        uint8_t c = v[i];
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 32) { char e[7]; snprintf(e, sizeof(e), "\\u%04x", c); out += e; }
        else out += char(c);
    }
    return out + "\"";
}

String WifiPortal::_renderPage() {
    String html = FPSTR(PORTAL_HTML);
    html.replace("__PORTAL_APP_NAME__", _config.appName);
    html.replace("__PORTAL_ACCENT__", _config.accentColor);
    html.replace("__PORTAL_TOKEN__", _formToken);
    return html;
}

bool WifiPortal::_loadCredentials() {
    Preferences prefs;
    if (!prefs.begin(_config.nvsNamespace, true)) return false;
    _networks = {};
    NetworkStore stored = {};
    if (prefs.getBytesLength("networks") == sizeof(stored) &&
        prefs.getBytes("networks", &stored, sizeof(stored)) == sizeof(stored) &&
        stored.version == 1 && stored.count > 0 && stored.count <= MAX_NETWORKS) {
        bool valid = true;
        for (uint32_t i = 0; i < stored.count; ++i) {
            const Network &net = stored.networks[i];
            if (!net.ssid[0] || net.ssid[32] || net.password[63]) valid = false;
        }
        if (valid) _networks = stored;
    }
    if (!_networks.count) {
        SavedNetwork net = {};
        if (prefs.getBytesLength("network") != sizeof(net) ||
            prefs.getBytes("network", &net, sizeof(net)) != sizeof(net) ||
            !net.ssid[0] || net.ssid[32] || net.password[63]) return false;
        _networks.version = 1;
        _networks.count = 1;
        memcpy(&_networks.networks[0], &net, sizeof(net));
    }
    _networkIndex = 0;
    _ssid = _networks.networks[0].ssid;
    _password = _networks.networks[0].password;
    return true;
}

bool WifiPortal::_saveCredentials() {
    NetworkStore next = _networks;
    next.version = 1;
    uint32_t index = next.count;
    for (uint32_t i = 0; i < next.count; ++i) {
        if (_ssid == next.networks[i].ssid) { index = i; break; }
    }
    // Newest additions/updates first; at capacity evict the oldest entry.
    if (index == next.count && next.count < MAX_NETWORKS) ++next.count;
    if (index >= MAX_NETWORKS) index = MAX_NETWORKS - 1;
    for (uint32_t i = index; i > 0; --i) next.networks[i] = next.networks[i - 1];
    next.networks[0] = {};
    strncpy(next.networks[0].ssid, _ssid.c_str(), 32);
    strncpy(next.networks[0].password, _password.c_str(), 63);
    Preferences prefs;
    if (!prefs.begin(_config.nvsNamespace, false)) return false;
    if (prefs.putBytes("networks", &next, sizeof(next)) != sizeof(next)) return false;
    NetworkStore verify = {};
    if (prefs.getBytes("networks", &verify, sizeof(verify)) != sizeof(verify) ||
        memcmp(&next, &verify, sizeof(next)) != 0) return false;
    _networks = next;
    _networkIndex = 0;
    prefs.remove("network");
    return true;
}

void WifiPortal::_connectWifi() {
    if (_ssid.isEmpty()) return;
    WiFi.persistent(false);
    WiFi.mode(_provisioning ? WIFI_AP_STA : WIFI_STA);
    WiFi.setAutoReconnect(false);
    _lastConnect = millis();
    WiFi.disconnect(false, false);
    WiFi.begin(_ssid.c_str(), _password.c_str());
}

bool WifiPortal::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiPortal::isConnecting() const {
    return !_provisioning && hasCredentials() && !isConnected() &&
        millis() - _offlineSince <= uint32_t(_networks.count) * RECONNECT_INTERVAL_MS;
}

bool WifiPortal::begin() {
    _offlineSince = millis();
    if (!_loadCredentials()) {
        startProvisioning();
        return false;
    }
    _connectWifi();
    return true;
}

void WifiPortal::_setupHandlers() {
    if (_handlersSet) return;
    WifiPortal *self = this;

    server.on("/", HTTP_GET, [self] {
        server.send(200, "text/html; charset=utf-8", self->_renderPage());
    });

    server.on("/networks", HTTP_GET, [self] {
        server.sendHeader("Cache-Control", "no-store");
        if (self->_savePending) {
            server.send(200, "application/json", "{\"busy\":false,\"networks\":[],\"error\":\"connecting\"}");
            return;
        }
        if (server.hasArg("scan") && !wifiBusy) {
            WiFi.scanDelete(); wifiCount = 0;
            int r = WiFi.scanNetworks(true);
            wifiBusy = r == WIFI_SCAN_RUNNING;
            if (r >= 0) wifiCount = min(r, 255);
        }
        if (wifiBusy) { server.send(200, "application/json", "{\"busy\":true}"); return; }
        String resp = "{\"busy\":false,\"networks\":[";
        bool first = true;
        for (int i = 0; i < wifiCount && i < 40; ++i) {
            String name = WiFi.SSID(i);
            if (name.isEmpty()) continue;
            if (!first) resp += ',';
            first = false;
            resp += "{\"ssid\":" + _jsonStr(name) + ",\"rssi\":" + String(WiFi.RSSI(i)) +
                    ",\"secure\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
        }
        resp += "]}";
        server.send(200, "application/json; charset=utf-8", resp);
    });

    server.on("/status", HTTP_GET, [self] {
        server.sendHeader("Cache-Control", "no-store");
        const String &st = self->_portalState;
        String text = st == "SAVED" ? "连接成功，配置已保存。可断开手机热点。" :
                      st == "VERIFYING" ? "正在连接新网络，热点会暂时关闭。" :
                      st == "CONNECT FAILED" ? "连接失败，已恢复原来的网络。请检查名称和密码后重试。" :
                      st == "SAVE FAILED" ? "已连接，但保存失败，请重新提交。" :
                      st == "CONNECTING" ? "正在连接，设备会短暂关闭热点，手机稍后会自动重连。" : "";
        server.send(200, "text/plain; charset=utf-8", text);
    });

    server.on("/save", HTTP_POST, [self] {
        if (server.arg("token") != self->_formToken) {
            server.send(403, "text/plain", "Token mismatch, reload page.");
            return;
        }
        if (wifiBusy || self->_savePending) {
            server.send(409, "text/plain; charset=utf-8", "Scanning, retry shortly.");
            return;
        }
        String newSSID = server.arg("ssid"), newPwd = server.arg("password");
        if (newSSID.isEmpty() || newSSID.length() > 32 || newPwd.length() > 63 ||
            (newPwd.length() && newPwd.length() < 8)) {
            server.send(400, "text/plain; charset=utf-8", "SSID 1-32, password empty or 8-63.");
            return;
        }
        // New credentials stay staged until they are proven to work, so a wrong
        // password cannot displace a network that was connecting fine.
        self->_pendingSsid = newSSID;
        self->_pendingPassword = newPwd;
        self->_savePending = true;
        self->_saveRequested = millis();
        self->_portalState = "CONNECTING";
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "text/html; charset=utf-8", self->_renderPage());
    });

    server.onNotFound([] {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });

    _handlersSet = true;
}

bool WifiPortal::_startAccessPoint() {
    _apName = _config.apName;
    _apPassword = _config.apPassword;

    WiFi.persistent(false);
    // AP_STA rather than AP: scanning needs the station interface, even though
    // it stays unassociated for as long as the portal is open.
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    if (!WiFi.softAP(_apName.c_str(), _apPassword.c_str(), PORTAL_AP_CHANNEL)) {
        _portalState = "AP FAILED";
        WiFi.mode(_ssid.isEmpty() ? WIFI_OFF : WIFI_STA);
        return false;
    }
    _portalStarted = millis();
    dns.start(53, "*", WiFi.softAPIP());
    _setupHandlers();
    server.begin();
    return true;
}

void WifiPortal::_stopAccessPoint() {
    server.stop();
    dns.stop();
    WiFi.softAPdisconnect(true);
}

void WifiPortal::startProvisioning() {
    if (_provisioning) return;
    // A connected station pins the soft AP to its channel and drags it along on
    // every scan and reconnect, which drops the phone. Park the station first.
    if (wifiBusy) { WiFi.scanDelete(); wifiBusy = false; wifiCount = 0; }
    WiFi.disconnect(false, false);
    // One token per session: the AP restarts after a failed attempt, and the
    // phone's open page must still be able to resubmit.
    _formToken = String(esp_random(), HEX) + String(esp_random(), HEX);
    if (!_startAccessPoint()) return;
    _provisioning = true;
    _portalState = "WAIT FOR PHONE";
}

void WifiPortal::stopProvisioning() {
    if (!_provisioning) return;
    _stopAccessPoint();
    _provisioning = false;
    _verifying = false;
    _savePending = false;
    if (_ssid.isEmpty()) {
        WiFi.mode(WIFI_OFF);
    } else {
        WiFi.mode(WIFI_STA);
        _connectWifi();
    }
}

void WifiPortal::_beginVerification() {
    _verifying = true;
    _portalState = "VERIFYING";
    // Taking the AP down first lets the radio follow the new network to whatever
    // channel it uses, instead of tearing the phone off mid-handshake.
    _stopAccessPoint();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, false);
    WiFi.setAutoReconnect(false);
    WiFi.begin(_pendingSsid.c_str(), _pendingPassword.c_str());
    _lastConnect = millis();
}

void WifiPortal::_finishVerification(bool connected) {
    _verifying = false;
    _savePending = false;
    if (connected) {
        _ssid = _pendingSsid;
        _password = _pendingPassword;
        _portalState = _saveCredentials() ? "SAVED" : "SAVE FAILED";
        _autoPortalUsed = false; // a new network earns a fresh automatic offer
        _provisioning = false;
        return;
    }
    // Restore the previous network and reopen the portal: the phone remembers
    // this fixed hotspot, so it reconnects on its own and sees the error.
    WiFi.disconnect(false, false);
    _provisioning = _startAccessPoint();
    if (_provisioning) _portalState = "CONNECT FAILED";
}

void WifiPortal::update() {
    if (wifiBusy) {
        int16_t r = WiFi.scanComplete();
        if (r >= 0) { wifiCount = r; wifiBusy = false; }
        else if (r != WIFI_SCAN_RUNNING) wifiBusy = false;
    }

    if (_provisioning) {
        dns.processNextRequest();
        server.handleClient();
        // Never time out in the middle of trying the credentials just submitted.
        if (_config.timeoutMs && !_savePending && !_verifying &&
            millis() - _portalStarted > _config.timeoutMs) {
            stopProvisioning();
            return;
        }
    }

    if (_savePending && !_verifying && millis() - _saveRequested >= SAVE_RESPONSE_GRACE_MS) {
        _beginVerification();
        return;
    }

    if (_verifying) {
        if (isConnected() && WiFi.localIP() != IPAddress(0, 0, 0, 0)) _finishVerification(true);
        else if (millis() - _lastConnect > VERIFY_TIMEOUT_MS) _finishVerification(false);
        return;
    }

    if (!_provisioning && hasCredentials() && !isConnected() && millis() - _lastConnect > RECONNECT_INTERVAL_MS) {
        if (_networks.count) {
            _networkIndex = (_networkIndex + 1) % _networks.count;
            _ssid = _networks.networks[_networkIndex].ssid;
            _password = _networks.networks[_networkIndex].password;
        }
        _connectWifi();
    }

    // Saved credentials that no longer reach anything usually mean the network
    // changed. Offer the portal once so the device is recoverable without a key.
    if (isConnected()) {
        _offlineSince = 0;
    } else if (!_provisioning && hasCredentials()) {
        if (_offlineSince == 0) _offlineSince = millis();
        else if (!_autoPortalUsed && _config.autoPortalMs &&
                 millis() - _offlineSince > max(_config.autoPortalMs, uint32_t(_networks.count) * RECONNECT_INTERVAL_MS)) {
            _autoPortalUsed = true;
            startProvisioning();
        }
    }
}
