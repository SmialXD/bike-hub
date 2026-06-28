Sensor Module

Handles low-level communication with the ADXL345 accelerometer.
Core Functions

- update(): Samples raw data at 10Hz and computes the lean angle using atan2 on the gravity vector.

- getFormattedCsvRow(): Flattens the latest telemetry into a CSV string, optimized for minimal processing delay during recording sessions.

Configuration

- Range: Set to ADXL345_RANGE_16_G to accommodate high-G motorcycle maneuvers without clipping.

- Frequency: Hardcoded to 100ms intervals (10Hz) to balance data resolution and storage longevity.
