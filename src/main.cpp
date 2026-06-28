#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define TFT_GREY 0x5AEB

#include "Sensor.h"
#include "menu.h"
#include "systemManager.h"

TFT_eSPI tft;
Sensor gyro;
SystemManager sysMan;

const char *WIFI_SSID = "test123";
const char *WIFI_PASSWORD = "12397654";
const int SAMPLE_RATE_MS = 100;

enum DeviceState { IDLE, RECORDING, SYNCING };
DeviceState currentState = IDLE;

void updateDashboard(TelemetryData data);
void setupDashboardLayout();

SubMenu fileList("Recordings");
SubMenu mainMenu("Main menu");
ActionItem syncMenu("Upload files", []() { currentState = SYNCING; });

MenuManager menu(&mainMenu);

void listFiles();

// Action to handle deletion processing inside fileList
ActionItem deleteMarkedFiles("Delete Marked Files", []() {
  int totalChildren = fileList.getChildrenCount();

  // 1. Create a safe temporary container to hold filenames targeted for removal
  std::vector<std::string> filesToDelete;

  // Collect names first without modifying the menu elements yet
  for (int i = 0; i < totalChildren; i++) {
    MenuItem *item = fileList.getChild(i);
    if (item && item->isMarkedForDeletion()) {
      std::string label = item->getLabel();

      // Explicitly ensure it's a recording file layout component
      if (label.size() > 4 && label.compare(label.size() - 4, 4, ".txt") == 0) {
        filesToDelete.push_back(label);
      }
    }
  }

  // 2. Perform the actual file operations safely on disk
  if (!filesToDelete.empty()) {
    for (const std::string &fileName : filesToDelete) {
      std::string filePath = "/" + fileName;
      sysMan.deleteFile(filePath);
    }

    // Update storage tracker counts
    sysMan.init();
  }

  // 3. Now it is completely safe to purge memory handles and re-render
  listFiles();
  fileList.render();
});

// FIX: Added missing forward entry point action button
ActionItem listFilesAction("View Recordings", []() {
  listFiles();
  menu.changeScreen(&fileList);
});

ActionItem startRecording("Start Recording", []() {
  currentState = RECORDING;
  sysMan.startSession();
  setupDashboardLayout();
});

ActionItem clearStorage("Clear storage", []() {
  sysMan.reset();
  mainMenu.render();
});

void listFiles() {
  fileList.clearChildren();
  int filesCount = sysMan.getFilesCount();

  if (filesCount == 0) {
    ActionItem *emptyNotice = new ActionItem("No recordings found", nullptr);
    fileList.addChild(emptyNotice);
    return;
  }

  // FIX: Allocate the delete button on the heap so clearChildren() can safely
  // call delete on it later
  ActionItem *dynamicDeleteBtn = new ActionItem("Delete Marked Files", []() {
    int totalChildren = fileList.getChildrenCount();
    std::vector<std::string> filesToDelete;

    for (int i = 0; i < totalChildren; i++) {
      MenuItem *item = fileList.getChild(i);
      if (item && item->isMarkedForDeletion()) {
        std::string label = item->getLabel();
        if (label.size() > 4 &&
            label.compare(label.size() - 4, 4, ".txt") == 0) {
          filesToDelete.push_back(label);
        }
      }
    }

    if (!filesToDelete.empty()) {
      for (const std::string &fileName : filesToDelete) {
        std::string filePath = "/" + fileName;
        sysMan.deleteFile(filePath);
      }
      sysMan.init();
    }

    listFiles();
    fileList.resetSelection();
    fileList.render();
  });

  fileList.addChild(dynamicDeleteBtn);

  // Render files
  for (int i = 0; i < filesCount; i++) {
    std::string fileName = std::to_string(i + 1) + ".txt";
    ActionItem *fileItem = new ActionItem(fileName, nullptr);

    fileItem->setCallback([fileItem]() {
      if (fileItem) {
        fileItem->setMarked(!fileItem->isMarkedForDeletion());
        fileList.render();
      }
    });

    fileList.addChild(fileItem);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(100);

  while (!gyro.ready()) {
    gyro.init();
    delay(100);
  }

  while (!sysMan.ready()) {
    sysMan.init();
    delay(100);
  }

  tft.begin();
  tft.setRotation(2);
  tft.setTextColor(TFT_WHITE, TFT_GREY);
  tft.fillScreen(TFT_GREY);

  // Hook up structural sub-items to parent containers
  mainMenu.addChild(&startRecording);
  mainMenu.addChild(&listFilesAction);
  mainMenu.addChild(&syncMenu);
  mainMenu.addChild(&clearStorage);

  menu.init(35, 0);
}

void loop() {
  switch (currentState) {
  case RECORDING:
    gyro.update();
    sysMan.writeRecordRow(gyro.getFormattedCsvRow());
    updateDashboard(gyro.data());

    if (digitalRead(0) == LOW) {
      sysMan.stopSession();
      currentState = IDLE;
      mainMenu.render();
      delay(200);
    }
    break;

  case SYNCING:
    sysMan.uploadData(WIFI_SSID, WIFI_PASSWORD);
    if (sysMan.done()) {
      sysMan.reset();
      currentState = IDLE;
      mainMenu.render();
    }
    break;

  case IDLE:
    menu.update();
    break;
  }
}

void setupDashboardLayout() { tft.fillScreen(TFT_BLACK); }
void updateDashboard(TelemetryData data) { /* Realtime plotting handles */ }
