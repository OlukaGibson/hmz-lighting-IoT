#include "led_controller.h"

LEDController::LEDController() : leds(nullptr), numLeds(0), ledPin(2), 
    currentAnimation(AnimationType::SOLID), currentColor(CRGB::Black),
    brightness(128), animationSpeed(50), animationDirection(true),
    lastUpdate(0), animationIndex(0) {}

LEDController::~LEDController() {
    if (leds) {
        delete[] leds;
    }
}

bool LEDController::initialize(const String& ledType, int numLeds, int pin) {
    this->numLeds = numLeds;
    this->ledPin = pin;
    this->ledType = ledType;
    
    if (leds) {
        delete[] leds;
    }
    leds = new CRGB[numLeds];
    
    // Configure FastLED based on LED type
    if (ledType == "WS2812B") {
        FastLED.addLeds<WS2812B, 2, GRB>(leds, numLeds);
    } else if (ledType == "SK6812") {
        FastLED.addLeds<SK6812, 2, GRB>(leds, numLeds);
    } else if (ledType == "WS2811") {
        FastLED.addLeds<WS2811, 2, RGB>(leds, numLeds);
    } else {
        // Default to WS2812B
        FastLED.addLeds<WS2812B, 2, GRB>(leds, numLeds);
    }
    
    FastLED.setBrightness(brightness);
    clear();
    show();
    
    Serial.println("LED Controller initialized: " + ledType + " (" + String(numLeds) + " LEDs)");
    return true;
}

void LEDController::setAnimation(AnimationType type) {
    currentAnimation = type;
    animationIndex = 0;
    Serial.println("Animation set to: " + String((int)type));
}

void LEDController::setSolidColor(uint8_t r, uint8_t g, uint8_t b) {
    currentColor = CRGB(r, g, b);
    setAnimation(AnimationType::SOLID);
}

void LEDController::setBrightness(uint8_t brightness) {
    this->brightness = brightness;
    FastLED.setBrightness(brightness);
    Serial.println("Brightness set to: " + String(brightness));
}

void LEDController::setSpeed(uint16_t speed) {
    this->animationSpeed = speed;
}

void LEDController::setDirection(bool forward) {
    this->animationDirection = forward;
}

void LEDController::update() {
    if (millis() - lastUpdate < animationSpeed) {
        return;
    }
    
    switch (currentAnimation) {
        case AnimationType::SOLID:
            updateSolid();
            break;
        case AnimationType::RAINBOW:
            updateRainbow();
            break;
        case AnimationType::BREATHE:
            updateBreathe();
            break;
        case AnimationType::THEATER_CHASE:
            updateTheaterChase();
            break;
        case AnimationType::COLOR_WIPE:
            updateColorWipe();
            break;
    }
    
    show();
    lastUpdate = millis();
}

void LEDController::updateSolid() {
    for (int i = 0; i < numLeds; i++) {
        leds[i] = currentColor;
    }
}

void LEDController::updateRainbow() {
    for (int i = 0; i < numLeds; i++) {
        leds[i] = CHSV((animationIndex + i * 255 / numLeds) % 255, 255, 255);
    }
    animationIndex = (animationIndex + (animationDirection ? 1 : -1)) % 255;
}

void LEDController::updateBreathe() {
    uint8_t breatheValue = (sin8(animationIndex) / 255.0) * brightness;
    for (int i = 0; i < numLeds; i++) {
        leds[i] = currentColor;
        leds[i].nscale8(breatheValue);
    }
    animationIndex += animationDirection ? 2 : -2;
}

void LEDController::updateTheaterChase() {
    clear();
    for (int i = animationIndex % 3; i < numLeds; i += 3) {
        leds[i] = currentColor;
    }
    animationIndex = (animationIndex + (animationDirection ? 1 : -1)) % 3;
}

void LEDController::updateColorWipe() {
    if (animationDirection) {
        if (animationIndex < numLeds) {
            leds[animationIndex] = currentColor;
            animationIndex++;
        }
    } else {
        if (animationIndex > 0) {
            animationIndex--;
            leds[animationIndex] = CRGB::Black;
        }
    }
}

void LEDController::clear() {
    for (int i = 0; i < numLeds; i++) {
        leds[i] = CRGB::Black;
    }
}

void LEDController::show() {
    FastLED.show();
}

bool LEDController::processThemeCommand(const String& jsonCommand) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
        Serial.println("Failed to parse theme command");
        return false;
    }
    
    String command = doc["command"];
    if (command != "theme") return false;
    
    if (doc.containsKey("brightness")) {
        setBrightness(doc["brightness"]);
    }
    
    if (doc.containsKey("speed")) {
        setSpeed(doc["speed"]);
    }
    
    String mode = doc["mode"];
    if (mode == "solid") {
        setSolidColor(doc["r"] | 255, doc["g"] | 255, doc["b"] | 255);
    } else if (mode == "rainbow") {
        setAnimation(AnimationType::RAINBOW);
    } else if (mode == "breathe") {
        if (doc.containsKey("r")) {
            setSolidColor(doc["r"], doc["g"], doc["b"]);
        }
        setAnimation(AnimationType::BREATHE);
    } else if (mode == "theater_chase") {
        if (doc.containsKey("r")) {
            setSolidColor(doc["r"], doc["g"], doc["b"]);
        }
        setAnimation(AnimationType::THEATER_CHASE);
    } else if (mode == "color_wipe") {
        if (doc.containsKey("r")) {
            setSolidColor(doc["r"], doc["g"], doc["b"]);
        }
        setAnimation(AnimationType::COLOR_WIPE);
    }
    
    return true;
}

String LEDController::getCurrentStatus() {
    JsonDocument doc;
    doc["led_type"] = ledType;
    doc["num_leds"] = numLeds;
    doc["brightness"] = brightness;
    doc["animation"] = (int)currentAnimation;
    doc["speed"] = animationSpeed;
    
    String output;
    serializeJson(doc, output);
    return output;
}
