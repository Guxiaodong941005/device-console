# Device Console · 设备调试台

C++17 + Qt 6 Widgets portfolio app for **industrial HMI / Qt 上位机** style demos.

A small desktop console that connects to an MQTT-like session, drives a **device connection state machine**, shows a message log, and can publish a JSON command. **Mock mode** (default) works offline via `QTimer` heartbeats and a simulated link drop — no broker required.

---

## Features · 功能

| EN | 中文 |
|----|------|
| Broker host / port / client id + Connect/Disconnect | Broker 地址与连接按钮 |
| State machine: Disconnected → Connecting → Online ↔ Reconnecting → Offline | 连接状态机（与 BLE/MQTT 重连体验同构） |
| Topic subscribe + scrolling message log | 订阅主题与消息日志 |
| Publish simple JSON command | 发布 JSON 指令 |
| Mock MQTT with heartbeats + simulated drop | Mock 心跳与断线演示 |
| `MqttClientInterface` + optional real `QMqttClient` hook | 可替换为真实 MQTT 客户端 |

## Build · 编译

```bash
# Debian/Ubuntu example
sudo apt install build-essential cmake qt6-base-dev

cd /workspace/cpp-portfolio/device-console   # or your clone path
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binary: `build/device_console`

### Optional real Qt MQTT

Qt MQTT is often **not** in distro packages. If you install it (Qt online installer module, or `qt6-mqtt-dev` where available):

```bash
cmake -B build -DDEVICE_CONSOLE_USE_QT_MQTT=ON
cmake --build build -j
```

See `include/QtMqttClient.h` and `src/QtMqttClient.cpp` for the wrapper skeleton.

## Run · 运行

```bash
./build/device_console
```

1. Leave **Mock mode** checked.
2. Click **Connect** — badge goes Connecting → Online; heartbeats appear in the log.
3. After ~12s Mock simulates a drop → Offline → auto Reconnecting → Online again.
4. Use **Subscribe** / **Publish JSON** while Online.

### Headless / CI note

This box may have no display. The project is expected to **compile** under `cmake --build`; running the GUI needs a desktop session or `QT_QPA_PLATFORM=offscreen` (UI will not be interactive).

```bash
# compile-only smoke (no window needed for link success)
cmake -B build && cmake --build build -j
```

## Layout · 结构

```
include/
  MqttClientInterface.h      # abstract client
  MockMqttClient.h           # offline demo client
  ConnectionStateMachine.h   # connection SM
  MainWindow.h
  QtMqttClient.h             # optional real client
src/
  main.cpp
  MainWindow.cpp
  ConnectionStateMachine.cpp
  MockMqttClient.cpp
  QtMqttClient.cpp
CMakeLists.txt
```

## Why this project · 作品意图

Maps smart-hardware client experience (MQTT session, reconnect state machine, command/telemetry topics) onto a **C++/Qt desktop** shape that Suzhou industrial / semiconductor / instrument software roles recognize.

License: MIT — see `LICENSE`.
