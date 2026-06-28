#include "menu.h"

#define TFT_GREY 0x5AEB

// --- MenuItem ---
MenuItem::MenuItem(const std::string &itemLabel)
    : label(itemLabel), isMarked(false) {}
std::string MenuItem::getLabel() const { return label; }
MenuItem *MenuItem::getParent() const { return parent; }
void MenuItem::setParent(MenuItem *parentNode) { parent = parentNode; }

// --- BackItem ---
BackItem::BackItem() : MenuItem("back") {}
void BackItem::execute() {
    if (parent) {}
}

// --- SubMenu ---
SubMenu::SubMenu(const std::string &itemLabel) : MenuItem(itemLabel) {
  // Automatically inject a back action on creation
  addChild(new BackItem());
}

SubMenu::~SubMenu() {
  for (auto child : childItems) {
    // FIX: Replaced dynamic_cast with lightweight type enum check
    if (child->getType() == MenuType::BackItem) {
      delete child;
    }
  }
}

void SubMenu::addChild(MenuItem *child) {
  child->setParent(this);
  // Keep the "back" button at the absolute bottom element index
  if (child->getType() == MenuType::BackItem) {
    // If we only have the auto-generated back item, insert at end
    if (childItems.empty()) {
      childItems.push_back(child);
    } else {
      childItems.insert(childItems.end() - 1, child);
    }
  } else {
    childItems.push_back(child);
  }
}

MenuItem *SubMenu::getSelectedChild() {
  if (childItems.empty())
    return nullptr;
  return childItems[selectedIndex];
}

void SubMenu::clearChildren() {
  for (int i = childItems.size() - 1; i >= 0; i--) {
    MenuItem *child = childItems[i];

    // FIX: Replaced dynamic_cast with lightweight type enum check
    if (child->getType() != MenuType::BackItem) {
      childItems.erase(childItems.begin() + i);
      delete child; // Safely free the dynamic RAM allocation
    }
  }
}

void SubMenu::moveUp() {
  if (childItems.empty())
    return;
  selectedIndex = (selectedIndex - 1 + childItems.size()) % childItems.size();
}

void SubMenu::moveDown() {
  if (childItems.empty())
    return;
  selectedIndex = (selectedIndex + 1) % childItems.size();
}

void SubMenu::execute() {}

void SubMenu::render() {
  tft.fillScreen(TFT_GREY);
  for (size_t i = 0; i < childItems.size(); i++) {
    tft.setCursor(0, i * 20, 2);

    // Render current selection cursor
    if (i == selectedIndex) {
      tft.print("> ");
    } else {
      tft.print("  ");
    }

    // Print the primary file label row
    if (childItems[i]->isMarkedForDeletion()) {
   std::string markedLabel = "[D] " + childItems[i]->getLabel(); 
   tft.print(markedLabel.c_str());
    }
    else {
    tft.print(childItems[i]->getLabel().c_str());
    printf("%s\n",childItems[i]->getLabel().c_str());
    }
    // Append a clean tag indicator if the file item is flagged
  }
}

// --- ActionItem ---
ActionItem::ActionItem(const std::string &itemLabel,
                       std::function<void()> callbackFunc)
    : MenuItem(itemLabel), callback(callbackFunc) {}

void ActionItem::execute() {
  if (callback)
    callback();
}

// --- MenuManager ---
MenuManager::MenuManager(SubMenu *rootMenu) : currentScreen(rootMenu) {}

void MenuManager::init(int downPin, int selectPin, int debounceMs) {

  downButton.attach(downPin, INPUT_PULLUP);
  selectButton.attach(selectPin, INPUT_PULLUP);
  downButton.interval(debounceMs);
  selectButton.interval(debounceMs);
  if (currentScreen)
    currentScreen->render();
}
void MenuManager::changeScreen(SubMenu *newScreen) {
    if (newScreen) {
        // Link the parent relationship dynamically so 'back' buttons know where to point
        newScreen->setParent(currentScreen);
        currentScreen = newScreen;
        currentScreen->render();
    }
}
void MenuManager::update() {
  downButton.update();
  selectButton.update();
  bool screenChanged = false;

  if (downButton.pressed()) {
      printf("DOWN\n");
    currentScreen->moveDown();
    screenChanged = true;
  }

  if (selectButton.pressed()) {
    MenuItem *selected = currentScreen->getSelectedChild();
    if (selected != nullptr) {
        if (selected->getType() == MenuType::BackItem) {
            // FIX: Safely bounce back to the parent screen
            if (currentScreen->getParent() != nullptr) {
                currentScreen = static_cast<SubMenu*>(currentScreen->getParent());
                screenChanged = true;
            }
        } 
        else if (selected->getType() == MenuType::SubMenu) {
            SubMenu *nextSub = static_cast<SubMenu *>(selected);
            changeScreen(nextSub); // Uses the new clean transition rule
            screenChanged = true;
        } 
        else {
            selected->execute();
            return; 
        }
    }
}

  if (screenChanged && currentScreen) {
      printf("Screen rendered\n");
    currentScreen->render();
  }
}
