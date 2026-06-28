#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <string>

class SystemManager {
private:
  bool isHardwareReady = false;
  bool isUploadInProgress = false;
  bool isUploadFinished = false;
  bool isFirstRowInSession = true; // Tracks JSON comma insertion

  File activeFile;
  WiFiUDP udp; // Lightweight UDP client for TFTP blasting
  
  int totalFilesToSync = 0;
  int currentFileSyncIndex = 0;

  void loadSessionCounter();
  void saveSessionCounter();
  
  // Internal spec-compliant TFTP binary file blaster
  bool sendTFTPFile(IPAddress serverIP, const char* localPath, const char* remoteName);

public:
  SystemManager();

  void deleteFile(const std::string &path);
  void init();
  bool ready() const;

  // Storage Utilities
  void startSession();
  void writeRecordRow(const std::string &csvRow);
  void stopSession();
  int getFilesCount() const;

  // Streamlined TFTP network engine methods
  void reset(); 
  void uploadData(const char *ssid, const char *password);
  bool done() const; 
};

#endif // SYSTEM_MANAGER_H
