SystemManager Module

Oversees LittleFS file operations and the custom TFTP networking stack.
Storage Logic

    Uses a session-based counter to create incremental filenames (1.txt, 2.txt).

    Writes data as raw strings to avoid the overhead of JSON parsing on the ESP32.

Networking (TFTP Engine)

    Uses raw UDP packets to "blast" files to the server. This avoids the memory overhead of the standard HTTPClient stack.

    Protocol: Implements the Write Request (WRQ) and Data (DATA) sequences defined in RFC 1350.

    Safety: Includes an automatic timeout and closure mechanism to ensure network sockets are freed if the connection is interrupted.
