#include "device_config.h"

const char* PersistentStorage::CONFIG_FILE = "/config.json";

JsonDocument PersistentStorage::loadData() {
    JsonDocument doc;
    if (!SPIFFS.exists(CONFIG_FILE)) {
        doc["devices"].to<JsonArray>();
        doc["networks"].to<JsonArray>();
        return doc;
    }
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open config file for reading");
        doc["devices"].to<JsonArray>();
        doc["networks"].to<JsonArray>();
        return doc;
    }
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        Serial.println("Failed to parse config file");
        doc.clear();
        doc["devices"].to<JsonArray>();
        doc["networks"].to<JsonArray>();
    }
    return doc;
}

bool PersistentStorage::saveData(const JsonDocument& doc) {
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to config file");
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool PersistentStorage::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    Serial.println("SPIFFS mounted successfully");
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Config file not found, creating with default data");
        initializeDefaultData();
    }
    return true;
}

bool PersistentStorage::initializeDefaultData() {
    JsonDocument doc;
    JsonArray devices = doc["devices"].to<JsonArray>();
    
    // Add default device with dynamic MAC address
    JsonObject device1 = devices.add<JsonObject>();
    device1["device_name"] = "LEDStrip1";
    device1["device_type"] = "strip";
    device1["led_type"] = "WS2812B";
    device1["num_of_leds"] = 30;
    device1["mac_address"] = getDeviceMacAddress();
    
    JsonArray networks = doc["networks"].to<JsonArray>();
    JsonObject network1 = networks.add<JsonObject>();
    network1["ssid"] = "HomeNetwork";
    network1["password"] = "password123";
    
    if (saveData(doc)) {
        Serial.println("Default data initialized successfully");
        return true;
    }
    return false;
}

String PersistentStorage::getDeviceMacAddress() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

bool PersistentStorage::addDevice(const String& deviceName, const String& deviceType, 
               const String& ledType, int numLeds, const String& macAddress) {
    JsonDocument doc = loadData();
    JsonArray devices = doc["devices"];
    for (JsonVariant device : devices) {
        if (device["device_name"] == deviceName || device["mac_address"] == macAddress) {
            Serial.println("Device with same name or MAC address already exists");
            return false;
        }
    }
    JsonObject newDevice = devices.add<JsonObject>();
    newDevice["device_name"] = deviceName;
    newDevice["device_type"] = deviceType;
    newDevice["led_type"] = ledType;
    newDevice["num_of_leds"] = numLeds;
    newDevice["mac_address"] = macAddress;
    if (saveData(doc)) {
        Serial.println("Device added: " + deviceName);
        return true;
    }
    return false;
}

bool PersistentStorage::addNetwork(const String& ssid, const String& password) {
    JsonDocument doc = loadData();
    JsonArray networks = doc["networks"];
    for (JsonVariant network : networks) {
        if (network["ssid"] == ssid) {
            Serial.println("Network with same SSID already exists");
            return false;
        }
    }
    JsonObject newNetwork = networks.add<JsonObject>();
    newNetwork["ssid"] = ssid;
    newNetwork["password"] = password;
    if (saveData(doc)) {
        Serial.println("Network added: " + ssid);
        return true;
    }
    return false;
}

bool PersistentStorage::updateDeviceProperty(const String& deviceName, const String& key, const String& value) {
    JsonDocument doc = loadData();
    JsonArray devices = doc["devices"];
    for (JsonVariant device : devices) {
        if (device["device_name"] == deviceName) {
            device[key] = value;
            if (saveData(doc)) {
                Serial.println("Updated device " + deviceName + " - " + key + ": " + value);
                return true;
            }
            return false;
        }
    }
    Serial.println("Device not found: " + deviceName);
    return false;
}

bool PersistentStorage::updateDeviceProperty(const String& deviceName, const String& key, int value) {
    JsonDocument doc = loadData();
    JsonArray devices = doc["devices"];
    for (JsonVariant device : devices) {
        if (device["device_name"] == deviceName) {
            device[key] = value;
            if (saveData(doc)) {
                Serial.println("Updated device " + deviceName + " - " + key + ": " + String(value));
                return true;
            }
            return false;
        }
    }
    Serial.println("Device not found: " + deviceName);
    return false;
}

bool PersistentStorage::updateNetworkProperty(const String& ssid, const String& key, const String& value) {
    JsonDocument doc = loadData();
    JsonArray networks = doc["networks"];
    for (JsonVariant network : networks) {
        if (network["ssid"] == ssid) {
            network[key] = value;
            if (saveData(doc)) {
                Serial.println("Updated network " + ssid + " - " + key + ": " + value);
                return true;
            }
            return false;
        }
    }
    Serial.println("Network not found: " + ssid);
    return false;
}

bool PersistentStorage::removeDevice(const String& deviceName) {
    JsonDocument doc = loadData();
    JsonArray devices = doc["devices"];
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i]["device_name"] == deviceName) {
            devices.remove(i);
            if (saveData(doc)) {
                Serial.println("Device removed: " + deviceName);
                return true;
            }
            return false;
        }
    }
    Serial.println("Device not found: " + deviceName);
    return false;
}

bool PersistentStorage::removeNetwork(const String& ssid) {
    JsonDocument doc = loadData();
    JsonArray networks = doc["networks"];
    for (int i = 0; i < networks.size(); i++) {
        if (networks[i]["ssid"] == ssid) {
            networks.remove(i);
            if (saveData(doc)) {
                Serial.println("Network removed: " + ssid);
                return true;
            }
            return false;
        }
    }
    Serial.println("Network not found: " + ssid);
    return false;
}

String PersistentStorage::getAllData() {
    JsonDocument doc = loadData();
    String output;
    serializeJsonPretty(doc, output);
    return output;
}

String PersistentStorage::getAllDataCompact() {
    JsonDocument doc = loadData();
    String output;
    serializeJson(doc, output);
    return output;
}

String PersistentStorage::getDevice(const String& deviceName) {
    JsonDocument doc = loadData();
    JsonArray devices = doc["devices"];
    for (JsonVariant device : devices) {
        if (device["device_name"] == deviceName) {
            String output;
            serializeJsonPretty(device, output);
            return output;
        }
    }
    return "Device not found";
}

String PersistentStorage::getNetwork(const String& ssid) {
    JsonDocument doc = loadData();
    JsonArray networks = doc["networks"];
    for (JsonVariant network : networks) {
        if (network["ssid"] == ssid) {
            String output;
            serializeJsonPretty(network, output);
            return output;
        }
    }
    return "Network not found";
}

String PersistentStorage::getAllDevices() {
    JsonDocument doc = loadData();
    String output;
    serializeJsonPretty(doc["devices"], output);
    return output;
}

String PersistentStorage::getAllNetworks() {
    JsonDocument doc = loadData();
    String output;
    serializeJsonPretty(doc["networks"], output);
    return output;
}

bool PersistentStorage::deviceExists(const String& deviceName) {
    JsonDocument doc = loadData();
    JsonArray devices = doc["devices"];
    for (JsonVariant device : devices) {
        if (device["device_name"] == deviceName) {
            return true;
        }
    }
    return false;
}

bool PersistentStorage::networkExists(const String& ssid) {
    JsonDocument doc = loadData();
    JsonArray networks = doc["networks"];
    for (JsonVariant network : networks) {
        if (network["ssid"] == ssid) {
            return true;
        }
    }
    return false;
}

int PersistentStorage::getDeviceCount() {
    JsonDocument doc = loadData();
    return doc["devices"].size();
}

int PersistentStorage::getNetworkCount() {
    JsonDocument doc = loadData();
    return doc["networks"].size();
}

bool PersistentStorage::clearAll() {
    if (SPIFFS.remove(CONFIG_FILE)) {
        Serial.println("All data cleared");
        return true;
    }
    Serial.println("Failed to clear data");
    return false;
}

void PersistentStorage::getStorageInfo() {
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    Serial.println("\n--- SPIFFS Storage Info ---");
    Serial.println("Total space: " + String(totalBytes) + " bytes");
    Serial.println("Used space: " + String(usedBytes) + " bytes");
    Serial.println("Free space: " + String(totalBytes - usedBytes) + " bytes");
    Serial.println("Usage: " + String((usedBytes * 100) / totalBytes) + "%");
    if (SPIFFS.exists(CONFIG_FILE)) {
        File file = SPIFFS.open(CONFIG_FILE, "r");
        Serial.println("Config file size: " + String(file.size()) + " bytes");
        file.close();
    }
}

bool PersistentStorage::formatSPIFFS() {
    Serial.println("Formatting SPIFFS...");
    if (SPIFFS.format()) {
        Serial.println("SPIFFS formatted successfully");
        return true;
    }
    Serial.println("SPIFFS format failed");
    return false;
}

bool PersistentStorage::interactiveSetup() {
    Serial.println("\n=== HMZ LED Controller Setup ===");
    Serial.println("1. Edit device configuration");
    Serial.println("2. Edit network configuration");
    Serial.println("3. Clear all data (with confirmation)");
    Serial.println("4. Continue with current settings");
    Serial.println("5. Show current configuration");
    Serial.print("Select option (1-5): ");
    
    while (!Serial.available()) {
        delay(100);
    }
    
    String input = Serial.readStringUntil('\n');
    input.trim();
    int choice = input.toInt();
    
    switch (choice) {
        case 1:
            return editDeviceConfig();
        case 2:
            return editNetworkConfig();
        case 3:
            return clearDataWithConfirmation();
        case 4:
            Serial.println("Continuing with current settings...");
            return true;
        case 5:
            Serial.println("\nCurrent configuration:");
            Serial.println(getAllData());
            return interactiveSetup(); // Show menu again
        default:
            Serial.println("Invalid option. Please try again.");
            return interactiveSetup();
    }
}

bool PersistentStorage::editDeviceConfig() {
    Serial.println("\n=== Edit Device Configuration ===");
    
    String deviceName;
    Serial.print("Device name [LEDStrip1]: ");
    while (!Serial.available()) delay(100);
    deviceName = Serial.readStringUntil('\n');
    deviceName.trim();
    if (deviceName.length() == 0) deviceName = "LEDStrip1";
    
    String deviceType;
    Serial.print("Device type (strip/bar/ring/matrix) [strip]: ");
    while (!Serial.available()) delay(100);
    deviceType = Serial.readStringUntil('\n');
    deviceType.trim();
    if (deviceType.length() == 0) deviceType = "strip";
    
    String ledType;
    Serial.print("LED type (WS2812B/SK6812/WS2811) [WS2812B]: ");
    while (!Serial.available()) delay(100);
    ledType = Serial.readStringUntil('\n');
    ledType.trim();
    if (ledType.length() == 0) ledType = "WS2812B";
    
    int numLeds;
    Serial.print("Number of LEDs [30]: ");
    while (!Serial.available()) delay(100);
    String numLedsStr = Serial.readStringUntil('\n');
    numLedsStr.trim();
    numLeds = (numLedsStr.length() == 0) ? 30 : numLedsStr.toInt();
    if (numLeds <= 0) numLeds = 30;
    
    // Clear existing config and create new one
    clearAll();
    JsonDocument doc;
    JsonArray devices = doc["devices"].to<JsonArray>();
    JsonObject device1 = devices.add<JsonObject>();
    device1["device_name"] = deviceName;
    device1["device_type"] = deviceType;
    device1["led_type"] = ledType;
    device1["num_of_leds"] = numLeds;
    device1["mac_address"] = getDeviceMacAddress();
    
    // Add default network
    JsonArray networks = doc["networks"].to<JsonArray>();
    JsonObject network1 = networks.add<JsonObject>();
    network1["ssid"] = "HomeNetwork";
    network1["password"] = "password123";
    
    if (saveData(doc)) {
        Serial.println("Device configuration saved successfully!");
        Serial.println("New configuration:");
        Serial.println(getAllData());
        return true;
    } else {
        Serial.println("Failed to save device configuration!");
        return false;
    }
}

bool PersistentStorage::editNetworkConfig() {
    Serial.println("\n=== Edit Network Configuration ===");
    
    String ssid;
    Serial.print("WiFi SSID [HomeNetwork]: ");
    while (!Serial.available()) delay(100);
    ssid = Serial.readStringUntil('\n');
    ssid.trim();
    if (ssid.length() == 0) ssid = "HomeNetwork";
    
    String password;
    Serial.print("WiFi Password [password123]: ");
    while (!Serial.available()) delay(100);
    password = Serial.readStringUntil('\n');
    password.trim();
    if (password.length() == 0) password = "password123";
    
    JsonDocument doc = loadData();
    JsonArray networks = doc["networks"];
    
    // Update first network or create new one
    if (networks.size() > 0) {
        networks[0]["ssid"] = ssid;
        networks[0]["password"] = password;
    } else {
        JsonObject newNetwork = networks.add<JsonObject>();
        newNetwork["ssid"] = ssid;
        newNetwork["password"] = password;
    }
    
    if (saveData(doc)) {
        Serial.println("Network configuration saved successfully!");
        return true;
    } else {
        Serial.println("Failed to save network configuration!");
        return false;
    }
}

bool PersistentStorage::clearDataWithConfirmation() {
    Serial.println("\n=== Clear All Data ===");
    Serial.println("WARNING: This will erase all device and network configurations!");
    Serial.print("Type 'YES' to confirm or anything else to cancel: ");
    
    while (!Serial.available()) delay(100);
    String confirmation = Serial.readStringUntil('\n');
    confirmation.trim();
    
    if (confirmation == "YES") {
        if (clearAll()) {
            Serial.println("All data cleared successfully!");
            Serial.println("Reinitializing with default data...");
            return initializeDefaultData();
        } else {
            Serial.println("Failed to clear data!");
            return false;
        }
    } else {
        Serial.println("Operation cancelled.");
        return true;
    }
}