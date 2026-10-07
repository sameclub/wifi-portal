# WifiPortal

[中文](README.zh-CN.md)

A captive-portal Wi-Fi provisioning library for ESP32 Arduino projects. It
raises a soft AP, serves a phone-friendly page that lists nearby networks,
stores the chosen credentials in NVS, and reconnects on every boot.

It is the shared provisioning layer behind [DotMic](https://github.com/sameclub/dotmic),
[PipBoy](https://github.com/sameclub/pipboy) and [VoxStick](https://github.com/sameclub/voxstick),
so one phone entry covers every firmware on the device.

## Why this one

Most provisioning libraries drop the AP as soon as they try new credentials,
which strands the phone on a dead hotspot when the password was wrong. This one
treats a failed attempt as recoverable:

- **The AP leaves channel 1 alone.** Scanning does not drag the hotspot across
  channels, so the phone stays associated while the user picks a network.
- **A failed attempt restores the old credentials** and reopens the hotspot
  under the same name and password. The phone rejoins on its own and shows the
  error, instead of the user having to hunt for a vanished AP.
- **NVS is only written after a connection succeeds**, so a bad attempt never
  destroys working credentials.
- **Saved networks unreachable for at least 60 s and one full retry cycle → the portal opens once by itself**, so a
  device carried to a new location is recoverable without a reset.

## Install

PlatformIO, as a git dependency:

```ini
lib_deps =
    https://github.com/sameclub/wifi-portal.git
```

Or vendored as a submodule, which is how the three firmware projects use it:

```bash
git submodule add https://github.com/sameclub/wifi-portal.git lib/wifi-portal
```

PlatformIO picks up anything under `lib/` automatically, so no `lib_deps` entry
is needed in that case.

## Use

```cpp
#include <WifiPortal.h>

WifiPortal portal({"MyApp", "#ff2900", "myapp-net"});
//                  ^name    ^accent    ^NVS namespace, one per app

void setup() {
    portal.begin();     // no stored credentials -> the portal opens itself
}

void loop() {
    portal.update();    // call every loop; drives the AP, DNS and HTTP server

    if (portal.isConnected()) {
        // online
    }
}
```

Toggle it from a button:

```cpp
if (portal.isProvisioning()) portal.stopProvisioning();
else                         portal.startProvisioning();
```

Show the hotspot details while it is up:

```cpp
if (portal.isProvisioning()) {
    draw("AP: " + portal.apName());
    draw("PW: " + portal.apPassword());
}
```

## Config

```cpp
struct WifiPortalConfig {
    const char *appName;          // shown on the portal page
    const char *accentColor;      // CSS colour, e.g. "#ff2900"
    const char *nvsNamespace;     // give each app its own
    uint32_t timeoutMs   = 300000; // close the AP after 5 min idle
    const char *apName     = "samestick";
    const char *apPassword = "samestick";
    uint32_t autoPortalMs = 60000; // offline this long -> open the portal once; 0 disables
};
```

`apName` / `apPassword` default to the same values across every app on purpose,
so the phone only has to remember one hotspot. Override them when two devices
must be provisioned side by side.

`autoPortalMs` also blocks sleep while the portal is open, so a battery-sensitive
app may prefer `0`.

## API

| Member | Meaning |
|---|---|
| `begin()` | Load NVS credentials and connect; opens the portal if there are none |
| `update()` | Pump the AP, DNS and HTTP server. Call every loop |
| `startProvisioning()` / `stopProvisioning()` | Open / close the hotspot |
| `isProvisioning()` | The hotspot is up |
| `isVerifying()` | AP is down while new credentials are being tried |
| `isConnected()` | Station is associated and has an IP |
| `hasCredentials()` | At least one saved network |
| `ssid()` | Current connection target SSID |
| `apName()` / `apPassword()` | Live hotspot details, for display |
| `state()` | Human-readable portal state |

## Portal

| | |
|---|---|
| Address | `192.168.4.1` |
| Channel | fixed at 1 |
| Timeout | 5 min idle, configurable |
| NVS record | Versioned list of up to 8 SSID/password records; legacy single-record read supported |

Connect the phone to the hotspot; the captive-portal prompt opens the page, or
browse to `192.168.4.1`. Pick a network, enter the password, save.

## Requirements

ESP32 Arduino core 3.x. Uses `WiFi`, `WebServer`, `DNSServer` and `Preferences`
from the core; no external dependencies.

## Licence

MIT. Portal page styles and the Wi-Fi icon are adapted from
[78/esp-wifi-connect](https://github.com/78/esp-wifi-connect), also MIT. See
[LICENSE](LICENSE).

## Saved networks

Stores up to 8 networks per app. Verified additions/updates move to the front; at capacity the oldest entry is replaced. Boot tries the newest entry first, then cycles through saved networks every 10 seconds while offline. A connected station stays on its network. Legacy single-network credentials load automatically. Failed verification never changes saved networks. The automatic portal waits at least one complete network cycle.
