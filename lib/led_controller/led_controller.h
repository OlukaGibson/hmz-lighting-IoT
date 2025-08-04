#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <ArduinoJson.h>

enum class AnimationType {
    SOLID,
    RAINBOW,
    BREATHE,
    THEATER_CHASE,
    COLOR_WIPE
};

class LEDController {
private:
    CRGB* leds;
    int numLeds;
    int ledPin;
    String ledType;
    AnimationType currentAnimation;
    CRGB currentColor;
    uint8_t brightness;
    uint16_t animationSpeed;
    bool animationDirection;
    unsigned long lastUpdate;
    uint16_t animationIndex;
    
    void updateSolid();
    void updateRainbow();
    void updateBreathe();
    void updateTheaterChase();
    void updateColorWipe();

public:
    LEDController();
    ~LEDController();
    
    bool initialize(const String& ledType, int numLeds, int pin);
    void setAnimation(AnimationType type);
    void setSolidColor(uint8_t r, uint8_t g, uint8_t b);
    void setBrightness(uint8_t brightness);
    void setSpeed(uint16_t speed);
    void setDirection(bool forward);
    void update();
    void clear();
    void show();
    
    // Command processing
    bool processThemeCommand(const String& jsonCommand);
    String getCurrentStatus();
};
