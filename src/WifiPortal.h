#pragma once
#include <Arduino.h>

// Every S3AI app raises the same provisioning hotspot, so one phone entry works
// for all of them. Override per app only if two devices must coexist.
#define WIFI_PORTAL_AP_NAME "samestick"
#define WIFI_PORTAL_AP_PASSWORD "samestick"

struct WifiPortalConfig {
    const char *appName;
    const char *accentColor;
    const char *nvsNamespace;
    uint32_t timeoutMs = 300000;
    const char *apName = WIFI_PORTAL_AP_NAME;
    const char *apPassword = WIFI_PORTAL_AP_PASSWORD;
    // Offline this long with saved credentials: open the portal once, so a
    // device that moved to a new network is recoverable. 0 disables it, which
    // a battery-sensitive app may prefer since the portal also blocks sleep.
    uint32_t autoPortalMs = 60000;
};

class WifiPortal {
public:
    WifiPortal(const WifiPortalConfig &config);

    bool begin();
    void update();
    void startProvisioning();
    void stopProvisioning();

    bool isProvisioning() const { return _provisioning; }
    bool isConnected() const;
    bool hasCredentials() const { return _ssid.length() > 0; }
    const String &ssid() const { return _ssid; }
    const String &apName() const { return _apName; }
    const String &apPassword() const { return _apPassword; }
    const String &state() const { return _portalState; }
    // True while the AP is down and the new credentials are being tried.
    bool isVerifying() const { return _verifying; }

private:
    WifiPortalConfig _config;
    static constexpr uint8_t MAX_NETWORKS = 8;
    struct Network { char ssid[33]; char password[64]; };
    struct NetworkStore { uint32_t version; uint32_t count; Network networks[MAX_NETWORKS]; };
    NetworkStore _networks = {};
    uint8_t _networkIndex = 0;
    String _ssid, _password;
    String _pendingSsid, _pendingPassword;
    String _apName, _apPassword, _formToken;
    String _portalState;
    bool _provisioning = false, _savePending = false, _handlersSet = false;
    bool _verifying = false, _autoPortalUsed = false;
    uint32_t _portalStarted = 0, _lastConnect = 0, _saveRequested = 0, _offlineSince = 0;

    String _renderPage();
    void _setupHandlers();
    void _connectWifi();
    bool _loadCredentials();
    bool _saveCredentials();
    bool _startAccessPoint();
    void _stopAccessPoint();
    void _beginVerification();
    void _finishVerification(bool connected);
    static String _jsonStr(const String &v);
};
