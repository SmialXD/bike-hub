#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Bounce2.h>
#include <TFT_eSPI.h>
#include <functional>
#include <string>
#include <vector>

extern TFT_eSPI tft;

// ==========================================
// BASE MENU ITEM
// ==========================================
enum class MenuType { BaseItem, BackItem, SubMenu, ActionItem };
class MenuItem {
protected:
  std::string label;
  MenuItem *parent = nullptr;
  bool isMarked = false;

public:
  MenuItem(const std::string &itemLabel);
  virtual ~MenuItem() = default;

  virtual MenuType getType() const { return MenuType::BaseItem; }

  std::string getLabel() const;
  MenuItem *getParent() const;
  void setParent(MenuItem *parentNode);
  void setMarked(bool state) { isMarked = state; }
  void markForDeletion() { isMarked = true; }
  bool isMarkedForDeletion() const { return isMarked; }
  virtual void execute() = 0;
};

// ==========================================
// AUTOMATIC BACK KEY
// ==========================================
class BackItem : public MenuItem {
public:
  BackItem();
  MenuType getType() const override { return MenuType::BackItem; }
  void execute() override;
};

// ==========================================
// SUBMENU (Folder)
// ==========================================
class SubMenu : public MenuItem {
private:
  std::vector<MenuItem *> childItems;
  int selectedIndex = 0;

public:
  SubMenu(const std::string &itemLabel);
  ~SubMenu() override;
  MenuType getType() const override { return MenuType::SubMenu; }
  void addChild(MenuItem *child);
  void clearChildren();
  void resetSelection() { selectedIndex = 0; }

  MenuItem *getChild(size_t index) {
    if (index < childItems.size()) {
      return childItems[index];
    }
    return nullptr;
  }
  int getChildrenCount() { return childItems.size(); }

  MenuItem *getSelectedChild();
  void moveUp();
  void moveDown();
  void execute() override;
  void render();
};

// ==========================================
// ACTION ITEM (Functional Leaf)
// ==========================================
class ActionItem : public MenuItem {
private:
  std::function<void()> callback;

public:
  ActionItem(const std::string &itemLabel, std::function<void()> callbackFunc);
  void execute() override;
  void setCallback(std::function<void()> cb) { callback = cb; }
};

// ==========================================
// THE MANAGER ENGINE
// ==========================================
class MenuManager {
private:
  SubMenu *currentScreen;
  Bounce2::Button downButton;
  Bounce2::Button selectButton;

public:
  MenuManager(SubMenu *rootMenu);
  void init(int downPin, int selectPin, int debounceMs = 25);
  void update();
  void changeScreen(SubMenu *newScreen);
};

#endif // MENU_H
