ESP32 Lean Angle Telemetry Logger

A high-performance, low-memory footprint telemetry logging system for motorcycles, designed to run on ESP32 hardware. This project utilizes the ADXL345 accelerometer to log real-time lean angles and acceleration data to LittleFS storage, with a custom TFTP-based wireless transmission protocol to offload data to a host machine.
Features

    High-Efficiency Data Collection: Samples at 10Hz to ensure smooth motion profiling.

    Low-Memory TFTP Transfer: Custom raw UDP implementation bypasses the RAM overhead of heavy HTTP stacks.

    Dynamic Menu UI: A clean, hierarchy-based menu system rendered on a TFT display.

    Client-Side Visualization: Includes a lightweight web dashboard for visualizing logged sessions directly in your browser.

Libraries & Dependencies

This project relies on the following libraries (ensure they are installed in your Arduino IDE Library Manager):

    TFT_eSPI: High-speed display driver for the TFT screen.

    Adafruit_ADXL345: Driver for the 3-axis accelerometer.

    Adafruit_Sensor: Unified sensor abstraction layer.

    Bounce2: Reliable debouncing for the physical menu navigation buttons.

Project Architecture

    main.cpp: Orchestrates the device state machine (IDLE, RECORDING, SYNCING).

    systemManager.cpp/h: Manages LittleFS file I/O, session counters, and the raw TFTP UDP transmission engine.

    Sensor.cpp/h: Handles the ADXL345 sampling, math conversions (radians to degrees), and data formatting.

    menu.cpp/h: Object-oriented menu structure for device interaction.

Usage

    Recording: Select "Start Recording" from the main menu. The device will log data to a new .txt file. Press the physical button to stop and save the session.

    Uploading: Ensure the ESP32 is within range of your configured Wi-Fi. Select "Upload files" from the main menu to initiate the TFTP blast to the network gateway.

    Visualization:

        Use the provided index.html in a web browser.

        Select your downloaded .json recording file using the file picker. (or use one of the example files in the exampleJson folder)

        The chart will automatically render your lean angle and acceleration metrics.

Contributing & Troubleshooting

    Memory Limits: The TFTP implementation is optimized for low RAM. If you add large buffers, keep an eye on ESP.getFreeHeap().

    Deployment: This project is "serverless" regarding data visualization; all processing happens in the browser, making it compatible with hosting on GitHub Pages.
