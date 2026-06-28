#include "systemManager.h"
#include <WiFi.h>

SystemManager::SystemManager() {}

void SystemManager::init() {
  if (!LittleFS.begin(true)) {
    Serial.println("[STORAGE] LittleFS Mount Failed!");
    isHardwareReady = false;
    return;
  }
  isHardwareReady = true;
  isUploadInProgress = false;
  isUploadFinished = false;
  loadSessionCounter();
}

bool SystemManager::ready() const { return isHardwareReady; }
int SystemManager::getFilesCount() const { return totalFilesToSync; }

void SystemManager::startSession() {
  if (!isHardwareReady) return;

  int nextFileNumber = totalFilesToSync + 1;
  std::string filePath = "/" + std::to_string(nextFileNumber) + ".txt";
  activeFile = LittleFS.open(filePath.c_str(), FILE_WRITE);

  if (!activeFile) {
    Serial.printf("[STORAGE] Failed to open %s for writing!\n", filePath.c_str());
  } else {
    Serial.printf("[STORAGE] Started JSON Recording Session: %s\n", filePath.c_str());
    activeFile.print("{\n"); // Open the JSON dictionary
    isFirstRowInSession = true;
  }
}

void SystemManager::writeRecordRow(const std::string &csvRow) {
  if (!isHardwareReady || !activeFile) return;

  // Extract timestamp and metrics from the raw row string
  int firstComma = csvRow.find(',');
  if (firstComma != -1) {
    std::string timestamp = csvRow.substr(0, firstComma);
    std::string metrics = csvRow.substr(firstComma + 1);

    // If this isn't the very first entry, append a comma to separate JSON keys
    if (!isFirstRowInSession) {
      activeFile.print(",\n");
    }

    // Write out formatted JSON map entry entry: "timestamp": "lean,ax,ay,az"
    activeFile.printf("  \"%s\": \"%s\"", timestamp.c_str(), metrics.c_str());
    activeFile.flush();
    isFirstRowInSession = false;
  }
}

void SystemManager::stopSession() {
  if (!isHardwareReady) return;

  if (activeFile) {
    activeFile.print("\n}\n"); // Close the valid JSON dictionary format
    activeFile.close();
    totalFilesToSync++;
    saveSessionCounter();
    Serial.printf("[STORAGE] Closed Session. Files waiting for sync: %d\n", totalFilesToSync);
  }
}

void SystemManager::uploadData(const char *ssid, const char *password) {
  if (!isHardwareReady || totalFilesToSync == 0) {
    isUploadFinished = true;
    return;
  }

  // --- Connect to Phone Hotspot ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SYNC] Connecting to Phone Hotspot via Wi-Fi...");
    WiFi.begin(ssid, password);

    int timeoutCounter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      timeoutCounter++;
      if (timeoutCounter > 30) {
        Serial.println("\n[SYNC] Hotspot connection timed out.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        return;
      }
    }
    Serial.print("\n[SYNC] Connected! Device IP: ");
    Serial.println(WiFi.localIP());
  }

  // --- Process Queue Sequence ---
  if (currentFileSyncIndex < totalFilesToSync) {
    isUploadInProgress = true;
    
    std::string localFileName = "/" + std::to_string(currentFileSyncIndex + 1) + ".txt";
    std::string remoteFileName = "session_" + std::to_string(currentFileSyncIndex + 1) + ".json";

    IPAddress phoneGatewayIP = WiFi.gatewayIP();
    Serial.printf("[TFTP] Blasting %s to Phone Server at IP: %s\n", localFileName.c_str(), phoneGatewayIP.toString().c_str());

    // Matches your high-port selection in TFTP CS
    bool success = sendTFTPFile(phoneGatewayIP, localFileName.c_str(), remoteFileName.c_str());

    if (success) {
      Serial.printf("[TFTP] Successfully transferred %s\n", remoteFileName.c_str());
      LittleFS.remove(localFileName.c_str()); 
      currentFileSyncIndex++;
    } else {
      Serial.println("[TFTP ERROR] Transfer failed. Retrying on next frame pass...");
      delay(1000); 
    }
  } 
  // Explicit alternative branch to separate the cleanup processing scope
  else {
    Serial.println("[SYNC] All session files sent to phone. Wiping storage cache counters...");
    reset();
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    isUploadInProgress = false;
    isUploadFinished = true;
  }
}

// Custom Light-weight Embedded TFTP Client Protocol Handler (RFC 1350)
bool SystemManager::sendTFTPFile(IPAddress serverIP, const char* localPath, const char* remoteName) {
  File file = LittleFS.open(localPath, FILE_READ);
  if (!file) return false;

  udp.begin(WiFi.localIP(), 49152 + random(1000)); // Dynamic source port selection
  
  // 1. Build Write Request Packet (WRQ) [Opcode (2 bytes) | Remote Name | 0 | Mode | 0]
  uint8_t packetBuffer[600];
  packetBuffer[0] = 0; packetBuffer[1] = 2; // WRQ Opcode
  int ptr = 2;
  strcpy((char*)&packetBuffer[ptr], remoteName);
  ptr += strlen(remoteName) + 1;
  strcpy((char*)&packetBuffer[ptr], "octet"); // Binary parsing format
  ptr += 5 + 1;

  udp.beginPacket(serverIP, 6969); // Matches your high-port selection in TFTP CS
  udp.write(packetBuffer, ptr);
  udp.endPacket();

  int blockNum = 1;
  uint16_t targetPort = 6969;

  // 2. Data Chunk Blaster Loop
  while (file.available() > 0 || blockNum == 1) {
    int packetSize = 0;
    unsigned long startWait = millis();
    while ((packetSize = udp.parsePacket()) == 0) {
      if (millis() - startWait > 2000) { // Timeout safety threshold
        file.close();
        udp.stop();
        return false;
      }
      delay(1);
    }

    targetPort = udp.remotePort();
    udp.read(packetBuffer, packetSize);

    // Check if received packet is an ACK (Opcode 4) and matches block index expectation
    if (packetBuffer[1] == 4) {
      uint16_t ackBlock = (packetBuffer[2] << 8) | packetBuffer[3];
      if (ackBlock != (blockNum - 1)) continue; // Drop out-of-order packets
    } else {
      file.close();
      udp.stop();
      return false; 
    }

    if (!file.available() && blockNum > 1) break; 

    // Assemble Data Payload Packet [Opcode (3) | Block # (2 bytes) | Data (0-512 bytes)]
    packetBuffer[0] = 0; packetBuffer[1] = 3; // DATA Opcode
    packetBuffer[2] = (blockNum >> 8) & 0xFF;
    packetBuffer[3] = blockNum & 0xFF;
    
    int bytesRead = file.read(&packetBuffer[4], 512);

    udp.beginPacket(serverIP, targetPort);
    udp.write(packetBuffer, bytesRead + 4);
    udp.endPacket();

    blockNum++;
  }

  file.close();
  udp.stop();
  return true;
}

void SystemManager::reset() {
  if (LittleFS.exists("/counter.cfg")) {
    LittleFS.remove("/counter.cfg");
  }
  
  totalFilesToSync = 0;
  currentFileSyncIndex = 0;
  isUploadFinished = false;
  isUploadInProgress = false;
  Serial.println("[STORAGE] Local session cache counters reset smoothly.");
}

void SystemManager::deleteFile(const std::string &path) {
  if (LittleFS.exists(path.c_str())) {
    LittleFS.remove(path.c_str());
    Serial.printf("[STORAGE] Deleted file: %s\n", path.c_str());
  }
}

void SystemManager::loadSessionCounter() {
  if (LittleFS.exists("/counter.cfg")) {
    File configFile = LittleFS.open("/counter.cfg", FILE_READ);
    if (configFile) {
      String val = configFile.readString();
      totalFilesToSync = val.toInt();
      configFile.close();
      Serial.printf("[STORAGE] Saved queue index layout count: %d\n", totalFilesToSync);
      return;
    }
  }
  totalFilesToSync = 0;
}

void SystemManager::saveSessionCounter() {
  File configFile = LittleFS.open("/counter.cfg", FILE_WRITE);
  if (configFile) {
    configFile.print(totalFilesToSync);
    configFile.close();
  }
}

bool SystemManager::done() const { return isUploadFinished; }
