#include "Sensor.h"
#include <Arduino.h>

Sensor::Sensor() : adxl(12345) {} // 12345 is a unique sensor ID

void Sensor::init() {
    Wire.begin();
    if (!adxl.begin()) {
        isInitialized = false;
        return;
    }
    
    // Configure sensor for typical motorcycle motion profile
    adxl.setRange(ADXL345_RANGE_16_G);
    isInitialized = true;
    lastSampleTime = millis();
}

bool Sensor::ready() const {
    return isInitialized;
}

void Sensor::update() {
    if (!isInitialized) return;

    unsigned long currentTime = millis();
    if (currentTime - lastSampleTime >= sampleIntervalMs) {
        lastSampleTime = currentTime;

        sensors_event_t event;
        adxl.getEvent(&event);

        // Capture raw acceleration data (m/s^2)
        currentData.accelerationX = event.acceleration.x;
        currentData.accelerationY = event.acceleration.y;
        currentData.accelerationZ = event.acceleration.z;

        // Calculate Lean Angle in degrees based on gravity vector positioning
        // atan2 returns radians, multiply by 180 / PI to convert to degrees
        float angleRad = atan2(currentData.accelerationX, currentData.accelerationZ);
        currentData.leanAngle = angleRad * (180.0f / PI);
    }
}

TelemetryData Sensor::data() const {
    return currentData;
}

std::string Sensor::getFormattedCsvRow() const {
    char buffer[64];
    // Keep it as a raw flat string: timestamp,lean_angle,accX,accY,accZ
    // SystemManager will handle the split and parse it instantly to JSON on disk
    snprintf(buffer, sizeof(buffer), "%lu,%.2f,%.2f,%.2f,%.2f", 
             millis(), 
             currentData.leanAngle, 
             currentData.accelerationX, 
             currentData.accelerationY, 
             currentData.accelerationZ);
    return std::string(buffer);
}
