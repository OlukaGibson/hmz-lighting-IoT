#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "pin_defn.h"

// BLE variables
extern BLEServer* pServer;
extern BLECharacteristic* pCharacteristic;
extern bool deviceConnected;
extern bool oldDeviceConnected;

// Global variables
extern bool ledState;
extern unsigned long lastSensorRead;
extern float sensorValue;
extern String deviceName;

#endif
