#include <Arduino.h>
#include "secrets.h"
#include "pin_defn.h"
#include "global_vars.h"
#include "ble_comm.h"
#include "device_config.h"

// Temporary stub for LEDController since it's not implemented yet
class LEDController {
public:
    bool initialize(const String& ledType, int numLeds, int pin) {
        Serial.println("LED Controller stub - initialize: " + ledType + " (" + String(numLeds) + " LEDs) on pin " + String(pin));
        return true;
    }
    void setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
        Serial.println("LED Controller stub - setSolidColor: " + String(r) + "," + String(g) + "," + String(b));
    }
    void update() {
        // Stub - do nothing for now
    }
};

// Global instances
PersistentStorage storage;
LEDController ledController;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== HMZ IoT LED Controller Starting ===");
    
    // Initialize SPIFFS and config storage
    if (!storage.begin()) {
        Serial.println("Storage initialization failed!");
        return;
    }

    // Show SPIFFS info
    storage.getStorageInfo();
    
    // Interactive setup option
    Serial.println("\nPress 's' within 5 seconds to enter setup mode...");
    unsigned long startTime = millis();
    bool enterSetup = false;
    
    while (millis() - startTime < 5000) {
        if (Serial.available()) {
            char input = Serial.read();
            if (input == 's' || input == 'S') {
                enterSetup = true;
                break;
            }
        }
        delay(100);
    }
    
    if (enterSetup) {
        storage.interactiveSetup();
    }
    
    Serial.println("Current configuration:");
    Serial.println(storage.getAllData());

    // Initialize LED controller with device config
    String deviceConfig = storage.getDevice("LEDStrip1");
    if (deviceConfig != "Device not found") {
        JsonDocument doc;
        deserializeJson(doc, deviceConfig);
        
        String ledType = doc["led_type"] | "WS2812B";
        int numLeds = doc["num_of_leds"] | 30;
        
        ledController.initialize(ledType, numLeds, LED_PIN);
        ledController.setSolidColor(255, 0, 0); // Start with red
    } else {
        // Default initialization
        ledController.initialize("WS2812B", 30, LED_PIN);
        ledController.setSolidColor(255, 255, 255); // Start with white
    }

    // Initialize BLE
    ble_setup();
    
    Serial.println("Setup complete! Ready for BLE connections.");
}

void loop() {
    ble_loop();
    ledController.update();
    delay(20); // Small delay for stability
}
