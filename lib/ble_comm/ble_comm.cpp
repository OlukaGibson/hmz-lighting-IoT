#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "pin_defn.h"
#include "global_vars.h"

// Forward declarations for external references
class PersistentStorage;
class LEDController;
extern PersistentStorage storage;
extern LEDController ledController;

// Additional BLE characteristics - Define them here
BLECharacteristic* pDeviceInfoTxCharacteristic = NULL;
BLECharacteristic* pDeviceInfoRxCharacteristic = NULL;
BLECharacteristic* pThemeRxCharacteristic = NULL;

// Function declarations
void handleCommand(String jsonCommand);
void handleLEDCommand(JsonDocument& doc);
void handleBlinkCommand(JsonDocument& doc);
void sendSensorData();
void sendDeviceStatus();
void sendResponse(String key, String value);
void readSensors();
String getChipInfo();
void sendDeviceInfo();
void handleDeviceInfoReceived(const String& jsonData);
void handleThemeCommand(const String& jsonData);

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
    }
};

// BLE Characteristic Callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() > 0) {
        Serial.println("Received: " + value);
        handleCommand(value);
      }
    }
};

// Device Info RX Callbacks
class DeviceInfoRxCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue().c_str();
        if (value.length() > 0) {
            Serial.println("Received device info: " + value);
            handleDeviceInfoReceived(value);
        }
    }
};

// Theme RX Callbacks  
class ThemeRxCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue().c_str();
        if (value.length() > 0) {
            Serial.println("Received theme command: " + value);
            handleThemeCommand(value);
        }
    }
};

void ble_setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init(deviceName.c_str());
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Original characteristic for backward compatibility
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  // Device Info TX (Send device info to phone)
  pDeviceInfoTxCharacteristic = pService->createCharacteristic(
                                  DEVICE_INFO_TX_UUID,
                                  BLECharacteristic::PROPERTY_READ |
                                  BLECharacteristic::PROPERTY_NOTIFY
                                );
  pDeviceInfoTxCharacteristic->addDescriptor(new BLE2902());

  // Device Info RX (Receive other devices from phone)
  pDeviceInfoRxCharacteristic = pService->createCharacteristic(
                                  DEVICE_INFO_RX_UUID,
                                  BLECharacteristic::PROPERTY_WRITE
                                );
  pDeviceInfoRxCharacteristic->setCallbacks(new DeviceInfoRxCallbacks());

  // Theme RX (Receive theme commands)
  pThemeRxCharacteristic = pService->createCharacteristic(
                             THEME_RX_UUID,
                             BLECharacteristic::PROPERTY_WRITE
                           );
  pThemeRxCharacteristic->setCallbacks(new ThemeRxCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  Serial.println("ESP32 BLE Server started!");
  Serial.println("Device name: " + deviceName);
  Serial.println("Waiting for client connection...");
}

void ble_loop() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Send device info periodically when connected
  static unsigned long lastDeviceInfoSent = 0;
  if (deviceConnected && millis() - lastDeviceInfoSent > 10000) {
    sendDeviceInfo();
    lastDeviceInfoSent = millis();
  }
  
  if (millis() - lastSensorRead > 2000) {
    readSensors();
    lastSensorRead = millis();
  }
  delay(100);
}

void handleCommand(String jsonCommand) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonCommand);
  if (error) {
    Serial.println("Failed to parse JSON command");
    sendResponse("error", "Invalid JSON format");
    return;
  }
  String command = doc["command"];
  Serial.println("Processing command: " + command);
  if (command == "led") {
    handleLEDCommand(doc);
  } else if (command == "sensors") {
    sendSensorData();
  } else if (command == "status") {
    sendDeviceStatus();
  } else if (command == "restart") {
    sendResponse("message", "Restarting ESP32...");
    delay(1000);
    ESP.restart();
  } else if (command == "blink") {
    handleBlinkCommand(doc);
  } else if (command == "get_device_info") {
    sendDeviceInfo();
  } else {
    sendResponse("error", "Unknown command: " + command);
  }
}

void handleLEDCommand(JsonDocument& doc) {
  String state = doc["state"];
  if (state == "ON") {
    ledState = true;
    digitalWrite(LED_PIN, HIGH);
    sendResponse("ledState", "ON");
    Serial.println("LED turned ON");
  } else if (state == "OFF") {
    ledState = false;
    digitalWrite(LED_PIN, LOW);
    sendResponse("ledState", "OFF");
    Serial.println("LED turned OFF");
  } else {
    sendResponse("error", "Invalid LED state. Use ON or OFF");
  }
}

void handleBlinkCommand(JsonDocument& doc) {
  int times = doc["times"] | 3;
  sendResponse("message", "Blinking LED " + String(times) + " times");
  bool originalState = ledState;
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  digitalWrite(LED_PIN, originalState ? HIGH : LOW);
}

void sendSensorData() {
  JsonDocument doc;
  doc["sensors"]["temperature"] = random(20, 35);
  doc["sensors"]["humidity"] = random(40, 80);
  doc["sensors"]["light"] = sensorValue;
  doc["sensors"]["ledState"] = ledState ? "ON" : "OFF";
  doc["sensors"]["uptime"] = millis();
  doc["sensors"]["timestamp"] = millis();
  String jsonString;
  serializeJson(doc, jsonString);
  if (deviceConnected) {
    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify();
    Serial.println("Sent sensor data: " + jsonString);
  }
}

void sendDeviceStatus() {
  JsonDocument doc;
  doc["status"]["chipModel"] = ESP.getChipModel();
  doc["status"]["chipRevision"] = ESP.getChipRevision();
  doc["status"]["cpuFreq"] = ESP.getCpuFreqMHz();
  doc["status"]["freeHeap"] = ESP.getFreeHeap();
  doc["status"]["totalHeap"] = ESP.getHeapSize();
  doc["status"]["uptime"] = millis();
  doc["status"]["deviceName"] = deviceName;
  doc["status"]["macAddress"] = String((uint32_t)ESP.getEfuseMac(), HEX);
  doc["status"]["ledState"] = ledState ? "ON" : "OFF";
  String jsonString;
  serializeJson(doc, jsonString);
  if (deviceConnected) {
    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify();
    Serial.println("Sent device status: " + jsonString);
  }
}

void sendResponse(String key, String value) {
  JsonDocument doc;
  doc[key] = value;
  String jsonString;
  serializeJson(doc, jsonString);
  if (deviceConnected) {
    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify();
    Serial.println("Sent response: " + jsonString);
  }
}

void readSensors() {
  int rawValue = analogRead(SENSOR_PIN);
  sensorValue = (rawValue / 4095.0) * 100.0;
}

String getChipInfo() {
  String info = "ESP32 Chip: ";
  info += ESP.getChipModel();
  info += " Rev ";
  info += ESP.getChipRevision();
  info += " (";
  info += ESP.getCpuFreqMHz();
  info += " MHz)";
  return info;
}

void sendDeviceInfo() {
    if (!deviceConnected || !pDeviceInfoTxCharacteristic) return;
    
    // Create a simple device info response
    JsonDocument doc;
    doc["device_name"] = deviceName;
    doc["mac_address"] = String((uint32_t)ESP.getEfuseMac(), HEX);
    doc["device_type"] = "controller";
    doc["led_type"] = "WS2812B";
    doc["num_of_leds"] = 30;
    
    String deviceInfo;
    serializeJson(doc, deviceInfo);
    pDeviceInfoTxCharacteristic->setValue(deviceInfo.c_str());
    pDeviceInfoTxCharacteristic->notify();
    Serial.println("Sent device info: " + deviceInfo);
}

void handleDeviceInfoReceived(const String& jsonData) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        Serial.println("Failed to parse received device info");
        return;
    }
    
    if (doc["device_name"].is<const char*>()) {
        String deviceName = doc["device_name"];
        String deviceType = doc["device_type"] | "strip";
        String ledType = doc["led_type"] | "WS2812B";
        int numLeds = doc["num_of_leds"] | 30;
        String macAddress = doc["mac_address"] | "00:00:00:00:00:00";
        
        Serial.println("Received device info - would add to storage");
        // Note: storage.addDevice() will be called when storage is properly linked
    }
    
    if (doc["ssid"].is<const char*>()) {
        String ssid = doc["ssid"];
        String password = doc["password"] | "";
        Serial.println("Received network info - would add to storage");
        // Note: storage.addNetwork() will be called when storage is properly linked
    }
}

void handleThemeCommand(const String& jsonData) {
    Serial.println("Received theme command - would process with LED controller");
    // Note: ledController.processThemeCommand() will be called when ledController is properly linked
    sendResponse("theme_status", "received");
}