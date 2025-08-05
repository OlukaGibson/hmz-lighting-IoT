#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <esp_system.h>
#include <esp_mac.h>

class PersistentStorage {
private:
    static const char* CONFIG_FILE;
    JsonDocument loadData();
    bool saveData(const JsonDocument& doc);

public:
    bool begin();
    bool initializeDefaultData();
    bool interactiveSetup();
    bool editDeviceConfig();
    bool editNetworkConfig();
    bool clearDataWithConfirmation();
    String getDeviceName();
    String getDeviceMacAddress();
    String getFirstDeviceName(); // Add this method
    bool addDevice(const String& deviceName, const String& deviceType, 
                   const String& ledType, int numLeds, const String& macAddress);
    bool addNetwork(const String& ssid, const String& password);
    bool updateDeviceProperty(const String& deviceName, const String& key, const String& value);
    bool updateDeviceProperty(const String& deviceName, const String& key, int value);
    bool updateNetworkProperty(const String& ssid, const String& key, const String& value);
    bool removeDevice(const String& deviceName);
    bool removeNetwork(const String& ssid);
    String getAllData();
    String getAllDataCompact();
    String getDevice(const String& deviceName);
    String getNetwork(const String& ssid);
    String getAllDevices();
    String getAllNetworks();
    bool deviceExists(const String& deviceName);
    bool networkExists(const String& ssid);
    int getDeviceCount();
    int getNetworkCount();
    bool clearAll();
    void getStorageInfo();
    bool formatSPIFFS();
};
