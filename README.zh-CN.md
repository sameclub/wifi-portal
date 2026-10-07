# WifiPortal

每个应用最多保存 8 个网络。新网络验证成功后追加保存，同名网络更新密码并移到首位；满额时替换最早添加或更新的网络。开机先尝试最近添加或更新的网络，离线时每 10 秒依次尝试其他已保存网络，已连接时保持当前网络。旧版单网络凭据自动兼容，验证失败不修改已保存列表。自动配网至少等待完整尝试一轮所有网络。

[English](README.md)

ESP32 Arduino 项目用的 Wi-Fi 配网库。开一个热点，手机连上后自动弹出网页列出附近
的网络，选好填密码存进 NVS，之后每次开机自动重连。

它是 [DotMic](https://github.com/sameclub/dotmic)、[PipBoy](https://github.com/sameclub/pipboy)
和 [VoxStick](https://github.com/sameclub/voxstick) 共用的配网层，所以同一台设备上
不管刷哪个固件，手机只需要记住一个热点。

## 为什么不用现成的

多数配网库一拿到新凭据就立刻关掉热点去试连，密码填错时手机就被晾在一个已经消失的
热点上。这个库把失败当成可恢复的情况处理：

- **热点钉死在信道 1**。扫描时不会被拖着换信道，用户挑网络的过程中手机不会掉线。
- **连接失败会恢复原凭据**，并用同样的名称密码重新开热点。手机自己就重连回来了，
  直接看到错误提示，不用去找那个消失的 AP。
- **只有连接成功才写 NVS**，一次失败的尝试不会毁掉原本能用的凭据。
- **已保存的网络连不上超过 60 秒，设备会自动开一次配网**，换了地方的设备不用复位
  也能救回来。

## 安装

PlatformIO，作为 git 依赖：

```ini
lib_deps =
    https://github.com/sameclub/wifi-portal.git
```

或者作为 submodule 嵌进项目，三个固件项目都是这么用的：

```bash
git submodule add https://github.com/sameclub/wifi-portal.git lib/wifi-portal
```

PlatformIO 会自动发现 `lib/` 下的库，这种方式不需要写 `lib_deps`。

## 用法

```cpp
#include <WifiPortal.h>

WifiPortal portal({"MyApp", "#ff2900", "myapp-net"});
//                  ^名称     ^强调色     ^NVS 命名空间，每个应用一个

void setup() {
    portal.begin();     // 没有存过凭据就自动进配网
}

void loop() {
    portal.update();    // 每个 loop 都要调，驱动 AP、DNS 和 HTTP 服务

    if (portal.isConnected()) {
        // 已联网
    }
}
```

按键切换：

```cpp
if (portal.isProvisioning()) portal.stopProvisioning();
else                         portal.startProvisioning();
```

配网时在屏幕上显示热点信息：

```cpp
if (portal.isProvisioning()) {
    draw("AP: " + portal.apName());
    draw("PW: " + portal.apPassword());
}
```

## 配置

```cpp
struct WifiPortalConfig {
    const char *appName;          // 显示在配网页上
    const char *accentColor;      // CSS 颜色，如 "#ff2900"
    const char *nvsNamespace;     // 每个应用用各自的
    uint32_t timeoutMs   = 300000; // 空闲 5 分钟自动关 AP
    const char *apName     = "samestick";
    const char *apPassword = "samestick";
    uint32_t autoPortalMs = 60000; // 离线这么久就自动开一次配网；0 关闭
};
```

`apName` / `apPassword` 在所有应用里故意保持一致，手机只用记一个热点。两台设备要
同时配网时再改成不同的值。

配网期间会阻止休眠，所以对电池敏感的应用可以把 `autoPortalMs` 设成 `0`。

## API

| 成员 | 含义 |
|---|---|
| `begin()` | 读 NVS 凭据并连接；没有凭据则自动开配网 |
| `update()` | 驱动 AP、DNS 和 HTTP 服务，每个 loop 调用 |
| `startProvisioning()` / `stopProvisioning()` | 开 / 关热点 |
| `isProvisioning()` | 热点是否开着 |
| `isVerifying()` | 热点已关、正在验证新凭据 |
| `isConnected()` | STA 已连接并拿到 IP |
| `hasCredentials()` | NVS 里有 SSID |
| `ssid()` | 已保存的 SSID |
| `apName()` / `apPassword()` | 当前热点信息，用于显示 |
| `state()` | 可读的配网状态 |

## 配网热点

| | |
|---|---|
| 地址 | `192.168.4.1` |
| 信道 | 固定 1 |
| 超时 | 空闲 5 分钟，可配置 |
| NVS 结构 | `{ char ssid[33]; char password[64]; }` |

手机连上热点后一般会自动弹出页面，没弹就访问 `192.168.4.1`。选网络、填密码、保存。

## 依赖

ESP32 Arduino core 3.x。只用到 core 自带的 `WiFi`、`WebServer`、`DNSServer` 和
`Preferences`，无外部依赖。

## 许可

MIT。配网页样式和 Wi-Fi 图标改编自
[78/esp-wifi-connect](https://github.com/78/esp-wifi-connect)（同为 MIT），
详见 [LICENSE](LICENSE)。
