# HMZ IoT LED Controller - Connections & Communication

## Hardware Connections

### ESP32-S3 Pin Assignments

| Pin | Function | Component | Notes |
|-----|----------|-----------|--------|
| GPIO 2 | LED_PIN | Addressable LED Strip | Data line for WS2812B/SK6812 |
| GPIO A10 | SENSOR_PIN | Light Sensor (Optional) | Analog input for ambient light |
| 3.3V | Power | LED Logic Level | Power for LED strip logic |
| 5V | Power | LED Strip Power | Main power for LED strip |
| GND | Ground | Common Ground | Shared between ESP32 and LEDs |

### LED Strip Wiring

```
ESP32-S3          LED Strip (WS2812B/SK6812)
--------          ---------------------------
GPIO 2    ------>    DIN (Data Input)
3.3V      ------>    VCC (Logic Power)
5V        ------>    +5V (LED Power)
GND       ------>    GND (Ground)
```

## Communication Architecture

### BLE Communication Structure

```
Phone/Tablet App  <--BLE-->  ESP32-S3 Controller  <--GPIO-->  LED Strip
     |                              |
     |                              |
     v                              v
JSON Commands                 SPIFFS Storage
& Responses                   (JSON Config)
```

## BLE Service & Characteristics

### Service UUID
- **Main Service:** `12345678-1234-1234-1234-123456789abc`

### Characteristics

| Characteristic | UUID | Type | Purpose |
|----------------|------|------|---------|
| Device Info TX | `********-****-****-****-************` | READ/NOTIFY | Send device info to phone |
| Device Info RX | `********-****-****-****-************` | WRITE | Receive device configs from phone |
| Theme RX | `********-****-****-****-************` | WRITE | Receive LED theme commands |
| Legacy | `********-****-****-****-************` | READ/WRITE/NOTIFY | Backward compatibility |

## Data Flow Diagrams

### Device Configuration Flow

```
Phone App                    ESP32 Controller
---------                    -----------------
    |                               |
    |  1. Connect via BLE          |
    |----------------------------->|
    |                               |
    |  2. Request device info       |
    |  {"command":"get_device_info"}|
    |----------------------------->|
    |                               |
    |  3. Send device info          |
    |  {device_name, led_type, etc} |
    |<-----------------------------|
    |                               |
    |  4. Send new device config    |
    |  {device_name, led_type, etc} |
    |----------------------------->|
    |                               |
    |  5. Save to SPIFFS            |
    |                               |-----> [SPIFFS]
    |                               |       /config.json
    |  6. Confirmation              |
    |<-----------------------------|
```

### LED Theme Control Flow

```
Phone App                    ESP32 Controller                LED Strip
---------                    -----------------                ----------
    |                               |                             |
    |  1. Send theme command        |                             |
    |  {"command":"theme",          |                             |
    |   "mode":"rainbow",           |                             |
    |   "brightness":128}           |                             |
    |----------------------------->|                             |
    |                               |                             |
    |  2. Process command           |                             |
    |                               |                             |
    |  3. Update LED controller     |                             |
    |                               |  4. Send color data         |
    |                               |---------------------------->|
    |                               |     (FastLED library)       |
    |  5. Send confirmation         |                             |
    |<-----------------------------|                             |
    |                               |                             |
    |                               |  6. Continuous updates      |
    |                               |---------------------------->|
    |                               |     (animation loop)        |
```

## JSON Command Structures

### Input Commands (Phone → ESP32)

#### 1. Get Device Info
```json
{
  "command": "get_device_info"
}
```

#### 2. LED Control
```json
{
  "command": "led",
  "state": "ON" | "OFF"
}
```

#### 3. Theme Control
```json
{
  "command": "theme",
  "mode": "solid" | "rainbow" | "breathe" | "theater_chase" | "color_wipe",
  "r": 255,
  "g": 0,
  "b": 0,
  "brightness": 128,
  "speed": 50
}
```

#### 4. Device Configuration
```json
{
  "device_name": "LEDStrip1",
  "device_type": "strip",
  "led_type": "WS2812B",
  "num_of_leds": 30,
  "mac_address": "F4:12:FA:CE:EF:C0"
}
```

#### 5. Network Configuration
```json
{
  "ssid": "HomeNetwork",
  "password": "password123"
}
```

### Output Responses (ESP32 → Phone)

#### 1. Device Info Response
```json
{
  "device_name": "ESP32-BLE-Device",
  "mac_address": "F412FACEEF C0",
  "device_type": "controller",
  "led_type": "WS2812B",
  "num_of_leds": 30
}
```

#### 2. Sensor Data
```json
{
  "sensors": {
    "temperature": 25,
    "humidity": 60,
    "light": 75.5,
    "ledState": "ON",
    "uptime": 123456,
    "timestamp": 123456
  }
}
```

#### 3. Device Status
```json
{
  "status": {
    "chipModel": "ESP32-S3",
    "chipRevision": 0,
    "cpuFreq": 240,
    "freeHeap": 234567,
    "totalHeap": 327680,
    "uptime": 123456,
    "deviceName": "ESP32-BLE-Device",
    "macAddress": "f412face",
    "ledState": "ON"
  }
}
```

#### 4. Error Response
```json
{
  "error": "Unknown command: invalid_cmd"
}
```

#### 5. Success Response
```json
{
  "theme_status": "success",
  "ledState": "ON",
  "message": "LED turned ON"
}
```

## SPIFFS Storage Structure

### File: `/config.json`
```json
{
  "devices": [
    {
      "device_name": "LEDStrip1",
      "device_type": "strip",
      "led_type": "WS2812B",
      "num_of_leds": 30,
      "mac_address": "F4:12:FA:CE:EF:C0"
    }
  ],
  "networks": [
    {
      "ssid": "HomeNetwork",
      "password": "password123"
    }
  ]
}
```

## Serial Monitor Interface

### Startup Options
```
Press 's' within 5 seconds to enter setup mode...

=== HMZ LED Controller Setup ===
1. Edit device configuration
2. Edit network configuration  
3. Clear all data (with confirmation)
4. Continue with current settings
5. Show current configuration
Select option (1-5):
```

### Interactive Prompts
```
Device name [LEDStrip1]: MyLEDStrip
Device type (strip/bar/ring/matrix) [strip]: strip
LED type (WS2812B/SK6812/WS2811) [WS2812B]: WS2812B
Number of LEDs [30]: 60
```

## LED Animation Types

| Animation | Description | Parameters |
|-----------|-------------|------------|
| **solid** | Single static color | r, g, b values |
| **rainbow** | Cycling rainbow effect | speed, direction |
| **breathe** | Pulsing brightness effect | color, speed |
| **theater_chase** | Running light pattern | color, speed |
| **color_wipe** | Progressive color fill | color, speed, direction |

## System States

### Initialization Sequence
1. **Serial Monitor Start** → 115200 baud
2. **SPIFFS Mount** → Load/create config.json
3. **Storage Info Display** → Show memory usage
4. **Interactive Setup** → 5-second window for 's' key
5. **Device Config Load** → Parse JSON configuration
6. **LED Controller Init** → Configure FastLED
7. **BLE Server Start** → Begin advertising
8. **Main Loop** → Handle BLE + LED updates

### Operating States
- **Disconnected**: LED shows default pattern, BLE advertising
- **Connected**: Process BLE commands, send periodic updates
- **Setup Mode**: Interactive configuration via Serial Monitor
- **Error State**: LED off, error messages via Serial

## Power Requirements

| Component | Voltage | Current (Typical) | Notes |
|-----------|---------|-------------------|--------|
| ESP32-S3 | 3.3V | 80-240mA | Varies with WiFi/BLE usage |
| WS2812B (per LED) | 5V | 60mA | At full white brightness |
| 30 LEDs (max) | 5V | 1.8A | Requires external power supply |

## Future Expansion

### Planned Features
- **WiFi Connectivity**: OTA updates, web dashboard
- **MQTT Integration**: Remote control via broker
- **Multiple LED Strips**: Support for multiple zones
- **Audio Reactive**: LED patterns based on microphone input
- **Time-based Themes**: Automatic patterns based on time of day

### Additional I/O Pins Available
- GPIO 4, 5, 6, 7: Available for sensors/buttons
- GPIO 15, 16: Available for additional LED strips
- I2C (GPIO 21, 22): For sensors/displays
- SPI: For additional peripherals

