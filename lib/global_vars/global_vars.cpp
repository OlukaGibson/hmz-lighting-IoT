#include "global_vars.h"

// BLE variables
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Global variables
bool ledState = false;
unsigned long lastSensorRead = 0;
float sensorValue = 0.0;
String deviceName = "ESP32-BLE-Device";
