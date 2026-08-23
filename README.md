# OmaDo

An ultra-lightweight, native tasks manager with support for **Microsoft To Do** Sync. Designed specifically for **Omarchy**.

---

## ✨ Features & Philosophy
I love Omarchy, but I have Samsung devices. Samsung Reminders is a beatiful and lightweight app for android and Samsung devices but you can only sync with Samsung Cloud or Microsoft To Do, Where unfortunaly there is no native client for Linux. And a web app is not a option for me. I see Omacalc and Omawrite projects and philosophy and I think that it can be a good idea make this app.   
- **Offline-First & Ultra-Lightweight:** Instant startup, minimal memory footprint, and complete offline functionality using a local SQLite database (WAL mode).
- **Dynamic Omarchy Theming:** Real-time theme synchronization with Omarchy.
- **Wayland Native & Frameless:** Sleek, minimal interface tailored for tiling window managers with keyboard-driven navigation.
- **MS To Do Layout:** Familiar 3-zone layout with special views (*My Day*, *Schedule*, *Tasks*), custom user lists, inline task creation bar, and slide-in task details drawer.
- **Monochromatic Vector Icons:** Crisp SVG iconography dynamically tinted to match the active system palette.

---

## 🗺 Roadmap & Current Progress

### ✅ Milestone 1: Offline Core & UI (Completed)
- [x] Modern target-based CMake build structure (`omado`, `omado_lib`).
- [x] Asynchronous SQLite database with dynamic thread-connection pooling and schema migrations.
- [x] Full local repository (CRUD for lists, tasks, and subtasks/steps).
- [x] Real-time TOML theme parser (`ThemeReader`) with atomic rename watching.
- [x] 100% Native English interface with Qt `qsTr` internationalization support.
- [x] 2-Section SidePanel (Special Views + Custom User Lists + List deletion).
- [x] Task list with animated circular checkboxes (`CheckCircle`) and importance toggles (★).
- [x] Bottom InputBar (Zone C) with Quick-Add metadata:
  - 📅 Interactive monthly calendar picker (`DatePickerPopup`).
  - ⏰ Custom time & preset reminder picker (`TimePickerPopup`).
  - 🔄 Recurrence selector (`RecurrencePopup` - Daily, Weekdays, Weekly, Monthly).
  - 📝 Inline draft subtasks builder (`DraftStepsPopup`).
- [x] Slide-in task detail panel (`TaskDetail.qml`) with notes editor, subtask checklist, and deletion.
- [x] Keyboard shortcuts (`N` for new task, `Space` to toggle, `Delete` to remove, `Arrows` to navigate, `Esc` to close drawer).

### 🔲 Milestone 2: Native Notifications & Background Daemon (Next)
- [ ] Direct D-Bus notification service via `org.freedesktop.Notifications`.
- [ ] Background daemon mode (`omado --daemon`) for scheduled reminder monitoring.
- [ ] Auto-start desktop entry / systemd user unit.

### 🔲 Milestone 3: Microsoft To Do Cloud Synchronization
- [ ] OAuth 2.0 PKCE authentication with Microsoft Entra ID (no client secrets).
- [ ] Secure token storage using system keyring via `qtkeychain-qt6` / `libsecret`.
- [ ] Bidirectional sync engine using Microsoft Graph API (`/me/todo/lists`).

### 🔲 Milestone 4: Quickshell Panel Plugin (v1.1)
- [ ] Dedicated panel widget (`omado-panel`) communicating with OmaDo via D-Bus (`io.omarchy.OmaDo`).

---

## 🛠 Prerequisites & Build Instructions

### Dependencies (Arch Linux / Omarchy)
```bash
sudo pacman -S cmake qt6-base qt6-declarative qt6-quickcontrols2 qtkeychain-qt6
```

### Build from Source
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Run
```bash
./build/omado
```

---

## ⌨ Keyboard Shortcuts

| Key | Action |
|---|---|
| `N` | Focus new task input bar |
| `Space` | Toggle completion on selected task |
| `Delete` | Delete selected task |
| `↑` / `↓` | Navigate task list |
| `Escape` | Close task detail drawer |

---

## 📄 License
GPL-3.0
