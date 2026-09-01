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

### ✅ Milestone 2: Native Notifications & Background Daemon (Completed)
- [x] Direct D-Bus notification service via `org.freedesktop.Notifications`.
- [x] Background daemon mode (`omado --daemon`) without GUI for scheduled reminder monitoring.
- [x] Full D-Bus IPC service (`io.omarchy.OmaDo`) exporting lists, today tasks, pending counts, and reactive signals.
- [x] Auto-start desktop entry (`autostart/omado-daemon.desktop`) and systemd user unit (`systemd/omado.service`).

### ✅ Milestone 3: Microsoft To Do Cloud Synchronization (Completed)
- [x] OAuth 2.0 PKCE authentication with Microsoft Entra ID (RFC 7636 compliant, no client secrets, CSRF-protected).
- [x] Secure token storage using system keyring via `qtkeychain-qt6` (`gnome-keyring` / `libsecret`).
- [x] Full Microsoft Graph REST client (`/v1.0/me/todo/` CRUD for lists, tasks, and checklist items).
- [x] Bidirectional sync engine (startup sync, 5-minute background sync, and debounced instant sync on task mutations).
- [x] Reactive connection footer and Omarchy-styled account management popup.

### 🔲 Milestone 4: Quickshell Panel Plugin (v1.1)
- [ ] Dedicated panel widget (`omado-panel`) communicating with OmaDo via D-Bus (`io.omarchy.OmaDo`).

---

## 🛠 Installation Guide

### Option 1: Quick Automated Install (Recommended)
Clonás el repositorio y ejecutás el script instalador:
```bash
git clone https://github.com/nicodiaz101/omado.git
cd omado
./install.sh
```
> El script compilará la app e instalará el binario, el icono oficial y el acceso directo `.desktop` en tu entorno.

---

### Option 2: Native Arch Package (`makepkg` / `pacman`)
Podés compilarlo e instalarlo como cualquier paquete de Arch Linux:
```bash
git clone https://github.com/nicodiaz101/omado.git
cd omado
makepkg -si
```
> Esto lo registra en tu base de datos de `pacman`, permitiéndote gestionarlo o desinstalarlo con `sudo pacman -R omado-git`.

---

### Option 3: Manual Build from Source
```bash
sudo pacman -S --needed cmake gcc git qt6-base qt6-declarative qt6-quickcontrols2 qtkeychain-qt6
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
sudo cmake --install .
```

---

## ⌨ Keyboard Shortcuts

| Key | Action |
|---|---|
| `N` | Focus new task input bar |
| `Space` | Toggle completion on selected task |
| `Delete` | Delete selected task |
| `Ctrl+R` | Manual cloud sync with Microsoft To Do |
| `↑` / `↓` | Navigate task list |
| `Escape` | Close task detail drawer / popups |
| `Ctrl+Q` | Quit application |

---

## 🤖 Background Daemon & D-Bus IPC

Run the lightweight background daemon (which handles scheduled reminders and exposes the D-Bus API without any UI overhead):

```bash
omado --daemon
```

### Autostart Options

#### 1. Via Hyprland (`hyprland.conf`) — Recommended for Omarchy
Add the following line to your `~/.config/hypr/hyprland.conf` (or `~/.config/hypr/autostart.conf`):

```ini
exec-once = omado --daemon
```

#### 2. Via systemd User Service
```bash
mkdir -p ~/.config/systemd/user
cp systemd/omado.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now omado.service
```

#### 3. Via XDG Autostart (.desktop)
```bash
mkdir -p ~/.config/autostart
cp autostart/omado-daemon.desktop ~/.config/autostart/
```

### Interacting via D-Bus (`io.omarchy.OmaDo`)
You can query or manipulate tasks directly from terminal or scripts (e.g. Quickshell, Waybar, or custom shortcuts):

```bash
# Get total pending task count
qdbus io.omarchy.OmaDo /io/omarchy/OmaDo io.omarchy.OmaDo.GetTotalPendingCount

# Get pending count for a specific list
qdbus io.omarchy.OmaDo /io/omarchy/OmaDo io.omarchy.OmaDo.GetPendingCount "special-myday"

# Toggle task completed status
qdbus io.omarchy.OmaDo /io/omarchy/OmaDo io.omarchy.OmaDo.ToggleTask "<task-id>" true
```

---

## 📄 License
GPL-3.0
