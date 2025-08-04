#ifndef BLE_COMM_H
#define BLE_COMM_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "pin_defn.h"
#include "global_vars.h"

void ble_setup();
void ble_loop();

#endif