Menu Module

A polymorphic, memory-conscious menu system designed for small TFT displays.
Architecture

- Polymorphism: Utilizes a base MenuItem class with ActionItem (leaf nodes) and SubMenu (parent nodes).

- Dynamic Memory: Designed to safely clear children during runtime, preventing memory leaks during menu navigation.

Navigation

- Bounce2 Integration: Handles hardware button debouncing to ensure reliable input detection.

- State Machine: Menu screens are swapped dynamically based on the current system state, allowing for context-aware actions like "Delete Marked Files" only when inside the recording sub-folder.
