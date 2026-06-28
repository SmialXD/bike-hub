#ifndef SENSOR_H
#define SENSOR_H

#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
#include <string>

// Struct to hold processed bike telemetry values
struct TelemetryData {
    float leanAngle;
    float accelerationX;
    float accelerationY;
    float accelerationZ;
};

class Sensor {
private:
    Adafruit_ADXL345_Unified adxl;
    bool isInitialized = false;
    TelemetryData currentData = {0.0f, 0.0f, 0.0f, 0.0f};
    
    unsigned long lastSampleTime = 0;
    const unsigned long sampleIntervalMs = 100; // 10Hz sampling rate

public:
    Sensor();
    
    // Hardware setup
    void init(); // this initializes via the adafruit lib
    bool ready() const;
    
    // Call this inside the RECORDING loop state to read and log metrics
    void update();
    
    // Returns the latest read values safely
    TelemetryData data() const;
    
    // Formats data into a single string row (e.g., CSV line) for storage
    std::string getFormattedCsvRow() const;
};

#endif // SENSOR_H
