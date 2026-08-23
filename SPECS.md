# SPECS.md — Especificaciones Técnicas de OmaDo
> Cliente nativo de tareas para Omarchy Quattro, con sincronización opcional con Microsoft To Do
> **Stack:** C++17 · Qt 6 · QML/Qt Quick · CMake 3.21+

---

## 1. Visión General

OmaDo sigue la filosofía minimalista de [OmaWrite](https://github.com/omacom-io/omawrite) y
[OmaCalc](https://github.com/omacom-io/omacalc): nativo, sin dependencias pesadas, integrado
al 100% con el sistema de theming de Omarchy Quattro.

### Principio Arquitectural Fundamental: Local-First

```
┌────────────────────────────────────────────────────────────────┐
│                       OmaDo v1.0                               │
│            Gestor de tareas 100% offline                       │
│        Almacenamiento: SQLite via Qt6::Sql                     │
│        (incluido en qt6-base, sin dependencias extra)          │
└──────────────────────────────┬─────────────────────────────────┘
                               │ v1.1+ — opcional, desactivado por defecto
               ┌───────────────▼──────────────────┐
               │       MS To Do Sync              │
               │  OAuth2 PKCE + Graph API         │
               │  Solo se activa si el usuario    │
               │  conecta su cuenta Microsoft     │
               └──────────────────────────────────┘
```

**La sincronización con MS To Do es un módulo opcional**, no el núcleo. El núcleo es un
gestor de tareas local completo con paridad de features de MS To Do.

El binario final debe arrancar en menos de 200 ms en hardware moderno.

---

## 2. Restricciones Absolutas

Las siguientes restricciones son **no negociables**.

| Prohibido | Permitido |
|---|---|
| Electron / Tauri / cualquier runtime web | Qt 6 puro (qt6-base, qt6-declarative, qt6-network) |
| KDE Frameworks (KIO, KWallet, Solid) | `qtkeychain` para tokens OAuth2 |
| GNOME libs directas (libadwaita, GLib) | `libsecret` solo a través de qtkeychain |
| `QSettings` para tokens de seguridad | `QKeychain::WritePasswordJob` / `ReadPasswordJob` |
| `QProcess` para cualquier propósito | QDBus nativo para notificaciones y IPC |
| Cualquier framework de UI adicional | QML/Qt Quick exclusivamente |
| `qmake` o autotools | CMake 3.21+ (target-based) |
| Parsers JSON/TOML externos | `QJsonDocument` y parser TOML manual con `QRegularExpression` |

---

## 3. Layout UI — Diseño de la Interfaz

### 3.1 Wireframe de Referencia

El wireframe original define la disposición general de la aplicación:

```
┌─────────────────────────┬────────────────────────────────────────────┐
│                         │                                            │
│  • My Day               │                                            │
│  • Schedule             │                                            │
│  • Tasks                │         TASK VIEW                          │
│  ─────────────────────  │         (lista de tareas de la             │
│  • Lista custom 1       │          vista/lista seleccionada)         │
│  • Lista custom 2       │                                            │
│  • Lista custom 3       │                                            │
│                         │                                            │
│  • New List             │                                            │
│                         │                                            │
│                         │                                            │
│  [Log in on MS To Do]   ├────────────────────────────────────────────┤
│                         │ ○  Nueva tarea…          🗑 👤 ≡          │
└─────────────────────────┴────────────────────────────────────────────┘
```

### 3.2 Zonas de la Interfaz

La ventana se divide en **tres zonas permanentes**:

#### Zona A — SidePanel (izquierda, ancho fijo)

Dividido internamente en dos secciones por un separador horizontal:

**Sección superior — Vistas Especiales (hardcodeadas, no editables):**
- `My Day` — tareas marcadas como "Mi día" o con `due_date = hoy`
- `Schedule` — tareas con `due_date` definido, agrupadas por fecha (ver §3.3)
- `Tasks` — vista global de todas las tareas sin filtro de lista

**Separador horizontal** (1px, `Theme.border`, con padding vertical 8px)

**Sección inferior — Listas del usuario:**
- Listas personalizadas creadas por el usuario (orden manual con drag-and-drop futuro)
- `+ New List` al final — inline input para nombrar la nueva lista

**Footer del SidePanel (pegado al borde inferior):**
- `Log in on MS To Do` — enlace/botón secundario de texto. Solo visible cuando el usuario
  NO está autenticado con Microsoft. Desaparece (o cambia a estado de cuenta) al conectarse.

#### Zona B — TaskView (derecha, ocupa el resto)

Lista de tareas de la vista o lista seleccionada en el SidePanel. Vacía cuando ninguna está seleccionada.

#### Zona C — Input Bar (bottom del TaskView)

Barra inferior del panel derecho. Contiene de izquierda a derecha:
- `○` — círculo vacío (checkbox sin marcar). Al hacer clic o presionar `Enter`, activa el input de nueva tarea inline. El cursor va directo al campo de texto.
- Campo de texto placeholder `"Nueva tarea..."` — se expande al escribir.
Cuatro iconos a la derecha (ver §3.4).

### 3.3 Vista "Schedule"

La vista Schedule es una **vista especial** (no una lista del usuario) que muestra todas las tareas
que tienen `due_date IS NOT NULL`, agrupadas por fecha en orden cronológico:

```
Schedule
─────────────────────────
  Hoy — domingo 24 ago
  ○  Preparar presentación
  ○  Llamar al cliente

  Mañana — lunes 25 ago
  ○  Revisión de código

  Miércoles 27 ago
  ○  Reunión mensual
─────────────────────────
```

- Secciones de fecha como headers no interactivos (texto en `Theme.foreground` opacidad 0.5).
- Tareas sin due date no aparecen en esta vista.
- Las tareas completadas no se muestran por defecto (toggle via icono ≡).

### 3.4 Iconos de la Input Bar (Zona C)

Los cuatro iconos a la derecha de la barra de entrada sirven para configurar la tarea **antes** de confirmarla, tal cual MS To Do, sumando la ventaja de las subtareas rápidas:

| Icono | Propósito | Acción al hacer clic |
|---|---|---|
| 📅 | Fecha Límite (`due_date`) | Abre un popup con calendario para elegir hoy, mañana, u otra fecha. |
| ⏰ | Recordatorio (`reminder_at`) | Abre un popup con horas rápidas (ej. Más tarde, Mañana 9:00). |
| 🔄 | Repetición (`recurrence`) | Abre popup con opciones: Diario, Semanal, Mensual, etc. |
| 📝 | Subtareas (`steps`) | Abre un panel/popup inline que permite tipear múltiples subtareas rápidamente antes de guardar. |

Los iconos se implementan con caracteres Unicode o fuentes de iconos (ej. Material Design Icons si se integran via font), consistente con Omarchy.
### 3.5 Proporciones y Dimensiones

- **SidePanel:** ancho fijo de **240px**. No redimensionable en v1.0.
- **Separador horizontal en SidePanel:** 1px, `Theme.border`, margen vertical 8px.
- **Input Bar (Zona C):** altura fija **44px**, `Theme.surface` como fondo.
- **Radio de esquinas de ventana:** 8px (contenedor raíz Rectangle).
- **Padding interno general:** 12px.
- **Altura de ítem en SidePanel:** 32px.
- **Altura de ítem de tarea en TaskView:** mínimo 36px, se expande si el título hace wrap.

---

## 4. Estructura de Directorios

```
omado/
├── CMakeLists.txt
├── SPECS.md
├── ROADMAP.md
├── AGENT.md
├── wireframe.jpeg                  # Diseño de referencia original
├── LICENSE                         # MIT
├── README.md
├── .gitignore
│
├── src/
│   ├── main.cpp                    # Punto de entrada: modo GUI o --daemon
│   │
│   ├── core/
│   │   ├── ThemeReader.h/.cpp      # Parser TOML colors.toml → Q_PROPERTY para QML
│   │   ├── Database.h/.cpp         # Inicialización y migraciones del schema SQLite
│   │   ├── AuthManager.h/.cpp      # OAuth2 PKCE via QNetworkAccessManager
│   │   ├── KeychainStore.h/.cpp    # Wrapper qtkeychain: read/write/delete tokens
│   │   ├── NotificationService.h/.cpp  # Notificaciones via QDBus → freedesktop
│   │   └── DaemonService.h/.cpp    # Adaptador QtDBus: servicio io.omarchy.OmaDo
│   │
│   ├── models/
│   │   ├── TaskList.h/.cpp         # Struct + QAbstractListModel para listas
│   │   ├── Task.h/.cpp             # Struct + QAbstractListModel para tareas
│   │   ├── TaskStep.h/.cpp         # Struct para subtareas/checklist
│   │   ├── LocalRepository.h/.cpp  # CRUD contra SQLite (fuente de verdad local)
│   │   └── GraphClient.h/.cpp      # Red: QNAM + Graph API (solo si sync activo)
│   │
│   └── qml/
│       ├── main.qml                # ApplicationWindow frameless, raíz
│       ├── SidePanel.qml           # Panel izquierdo completo (zonas A)
│       ├── SpecialListSection.qml  # Sección superior: My Day, Schedule, Tasks
│       ├── UserListSection.qml     # Sección inferior: listas custom + New List
│       ├── TaskView.qml            # Zona B: lista de tareas
│       ├── ScheduleView.qml        # Vista especial Schedule (agrupada por fecha)
│       ├── TaskDetail.qml          # Panel lateral (overlay): notas, steps, fecha, prioridad
│       ├── InputBar.qml            # Zona C: barra inferior de entrada de tarea
│       ├── TaskDelegate.qml        # Ítem de tarea con checkbox
│       ├── ScheduleDelegate.qml    # Ítem de tarea en vista Schedule (con fecha)
│       ├── AuthView.qml            # Pantalla de autenticación OAuth2 (overlay)
│       └── components/
│           ├── ThemedText.qml      # Text con iA Writer Mono + color del tema
│           ├── FocusRect.qml       # Indicador visual de foco para teclado
│           ├── StatusDot.qml       # Estado de sync (solo visible si sync activo)
│           ├── SectionSeparator.qml # Separador horizontal del SidePanel
│           ├── CheckCircle.qml     # Círculo de checkbox (outline/filled)
│           └── ImportanceDot.qml   # Indicador de prioridad (low/normal/high)
│
├── fonts/
│   ├── iAWriterMono-Regular.ttf
│   ├── iAWriterMono-Bold.ttf
│   └── OFL.txt
│
├── bin/
│   └── omado
│
├── pkgbuild/
│   └── PKGBUILD
│
└── tests/
    ├── CMakeLists.txt
    ├── fixtures/
    ├── tst_ThemeReader.cpp
    ├── tst_Database.cpp
    ├── tst_LocalRepository.cpp
    └── tst_AuthManager.cpp
```

---

## 5. Almacenamiento Local — SQLite

### 5.1 Stack de Datos

- **Motor:** SQLite via `Qt6::Sql` (`QSqlDatabase`, `QSqlQuery`) — incluido en `qt6-base`.
- **Ubicación del archivo:** `~/.local/share/omado/omado.db` (XDG Data Home).
- **Migraciones:** Sistema de versiones en tabla `schema_version`. `Database::initialize()`
  aplica migraciones en orden al arrancar.
- **SQLite WAL mode** activado por defecto.

### 5.2 Schema SQL (v1)

```sql
CREATE TABLE IF NOT EXISTS task_lists (
    id              TEXT PRIMARY KEY,
    display_name    TEXT NOT NULL,
    is_special      INTEGER NOT NULL DEFAULT 0, -- 1 = My Day/Schedule/Tasks (no editables)
    sort_order      INTEGER NOT NULL DEFAULT 0,
    created_at      TEXT NOT NULL,
    synced_at       TEXT,
    remote_id       TEXT
);

CREATE TABLE IF NOT EXISTS tasks (
    id              TEXT PRIMARY KEY,
    list_id         TEXT NOT NULL REFERENCES task_lists(id) ON DELETE CASCADE,
    title           TEXT NOT NULL,
    body            TEXT NOT NULL DEFAULT '',
    is_completed    INTEGER NOT NULL DEFAULT 0,
    is_my_day       INTEGER NOT NULL DEFAULT 0,
    importance      TEXT NOT NULL DEFAULT 'normal',
    due_date        TEXT,           -- ISO 8601 date
    reminder_at     TEXT,           -- ISO 8601 datetime
    reminded        INTEGER NOT NULL DEFAULT 0,
    recurrence      TEXT NOT NULL DEFAULT 'none',
    sort_order      INTEGER NOT NULL DEFAULT 0,
    created_at      TEXT NOT NULL,
    completed_at    TEXT,
    synced_at       TEXT,
    remote_id       TEXT
);

CREATE TABLE IF NOT EXISTS task_steps (
    id              TEXT PRIMARY KEY,
    task_id         TEXT NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    title           TEXT NOT NULL,
    is_completed    INTEGER NOT NULL DEFAULT 0,
    sort_order      INTEGER NOT NULL DEFAULT 0,
    remote_id       TEXT
);

CREATE TABLE IF NOT EXISTS schema_version (
    version         INTEGER PRIMARY KEY,
    applied_at      TEXT NOT NULL
);
```

**Datos iniciales (seed):**
```sql
INSERT INTO task_lists (id, display_name, is_special, sort_order, created_at)
VALUES
  ('special-myday',    'My Day',   1, 0, datetime('now')),
  ('special-schedule', 'Schedule', 1, 1, datetime('now')),
  ('special-tasks',    'Tasks',    1, 2, datetime('now')),
  ('default-tasks',    'Tasks',    0, 3, datetime('now'));
```

Las vistas especiales (`is_special=1`) no son listas reales de almacenamiento —
son **vistas virtuales** que hacen queries dinámicos. `LocalRepository` las trata diferente.

### 5.3 LocalRepository

`LocalRepository : QObject` es la única clase autorizada a ejecutar SQL.

```
Métodos (QFuture<T> via QtConcurrent::run):
  fetchLists()              → QFuture<QList<TaskList>>
  fetchTasks(listId)        → QFuture<QList<Task>>      (lista real)
  fetchMyDayTasks()         → QFuture<QList<Task>>      (is_my_day=1 o due_date=hoy)
  fetchScheduleTasks()      → QFuture<QList<Task>>      (due_date IS NOT NULL, ordenado)
  fetchAllTasks()           → QFuture<QList<Task>>      (todas, para vista Tasks)
  createList(name)          → QFuture<TaskList>
  updateList(list)          → QFuture<bool>
  deleteList(id)            → QFuture<bool>
  createTask(task)          → QFuture<Task>
  updateTask(task)          → QFuture<bool>
  deleteTask(id)            → QFuture<bool>
  fetchSteps(taskId)        → QFuture<QList<TaskStep>>
  createStep(step)          → QFuture<TaskStep>
  updateStep(step)          → QFuture<bool>
  deleteStep(id)            → QFuture<bool>
  getPendingCount(listId)   → QFuture<int>
  getTotalPendingCount()    → QFuture<int>
  getPendingReminders()     → QFuture<QList<Task>>     (reminder_at <= now AND reminded=0)
```

---

## 6. Seguridad — Persistencia de Tokens OAuth2

**Solo aplica cuando el usuario activa la sincronización con MS To Do.**

### 6.1 Cadena de Seguridad

```
OmaDo
  └─ qtkeychain (Qt wrapper, no es librería GNOME ni KDE)
       └─ libsecret (cliente D-Bus del Secret Service API)
            └─ gnome-keyring (daemon ya corriendo en Omarchy)
                  ← El mismo keyring donde Brave guarda sus contraseñas
```

### 6.2 Claves Almacenadas

| Key | Contenido |
|---|---|
| `ms_access_token` | Bearer token (~1h TTL) |
| `ms_refresh_token` | Token de refresco (~90 días) |
| `ms_token_expiry` | Timestamp ISO 8601 de expiración |

Ningún token puede escribirse en `QSettings`, archivos planos, o variables de entorno.

---

## 7. Backend de Red — Graph API (Módulo Opcional)

`GraphClient` solo se instancia cuando el usuario activa la sincronización.

**Política de errores de red:** Silenciosa. Errores → `qWarning()` + señal `networkError(QString)`.
`StatusDot` en la Input Bar indica el estado. Nunca diálogos modales.

**Endpoints:**

| Método | Endpoint | Propósito |
|---|---|---|
| `GET` | `/v1.0/me/todo/lists` | Obtener listas |
| `GET` | `/v1.0/me/todo/lists/{id}/tasks` | Obtener tareas |
| `POST` | `/v1.0/me/todo/lists/{id}/tasks` | Crear tarea |
| `PATCH` | `/v1.0/me/todo/lists/{id}/tasks/{tid}` | Actualizar |
| `DELETE` | `/v1.0/me/todo/lists/{id}/tasks/{tid}` | Eliminar |

---

## 8. Flujo OAuth2 PKCE

```
AuthManager
  │ 1. generateCodeVerifier(): QRandomGenerator → 64 bytes → Base64URL
  │    codeChallenge = SHA256(verifier) → Base64URL via QCryptographicHash
  │
  │ 2. QDesktopServices::openUrl(authUrl)
  │
  │ 3. QTcpServer efímero en localhost → escucha redirect
  │
  │ 4. Extraer code → destruir QTcpServer
  │
  │ 5. POST /token via QNAM → { access_token, refresh_token }
  │
  │ 6. KeychainStore::writeToken(...)
  ▼
✅ Autenticado — icono 👤 en Input Bar cambia a estado conectado
               — footer "Log in on MS To Do" desaparece del SidePanel
```

Scopes: `Tasks.ReadWrite offline_access`

---

## 9. Notificaciones del Sistema

Via `org.freedesktop.Notifications` directamente con `Qt6::DBus`. Sin `QProcess`.

```cpp
QDBusInterface iface(
    "org.freedesktop.Notifications",
    "/org/freedesktop/Notifications",
    "org.freedesktop.Notifications",
    QDBusConnection::sessionBus()
);
iface.call("Notify",
    "OmaDo", 0u, "checkbox-checked",
    title, body,
    QStringList(), QVariantMap(), 5000
);
```

**Trigger de notificaciones:**
- Reminder de tarea: daemon monitorea `reminder_at` con QTimer de 60s.
- Tarea marcada como completada desde el plugin de panel.

---

## 10. Modelos de Datos C++

### 10.1 Structs

```cpp
struct TaskList {
    QString  id;
    QString  displayName;
    bool     isSpecial   = false;  // My Day, Schedule, Tasks (vistas virtuales)
    int      sortOrder   = 0;
    QString  remoteId;
    bool     isSynced() const { return !remoteId.isEmpty(); }
};

struct TaskStep {
    QString id;
    QString taskId;
    QString title;
    bool    isCompleted = false;
    int     sortOrder   = 0;
};

struct Task {
    QString         id;
    QString         listId;
    QString         title;
    QString         body;
    bool            isCompleted  = false;
    bool            isMyDay      = false;
    QString         importance;      // "low" | "normal" | "high"
    QDate           dueDate;
    QDateTime       reminderAt;
    QString         recurrence;      // "none"|"daily"|"weekly"|"monthly"|"yearly"
    QList<TaskStep> steps;
    int             sortOrder    = 0;
    QDateTime       createdAt;
    QDateTime       completedAt;
    QString         remoteId;
};
```

### 10.2 Modelos QML

- `TaskListModel : QAbstractListModel` — roles: Id, DisplayName, IsSpecial, SortOrder, PendingCount.
- `TaskModel : QAbstractListModel` — roles: Id, Title, IsCompleted, IsMyDay, DueDate, Importance, ReminderAt, HasSteps.
- `ScheduleTaskModel` — extiende `TaskModel` con rol `DueDateSection` (string de fecha para secciones).
- `QSortFilterProxyModel` para filtrado de completadas (toggle via icono ≡).

---

## 11. UI — QML / Qt Quick

### 11.1 Ventana Frameless

```qml
ApplicationWindow {
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"
    DragHandler {
        target: null
        onActiveChanged: if (active) window.startSystemMove()
    }
}
```

El draggable cubre todo el SidePanel. El TaskView puede tener su propio DragHandler
solo en áreas sin elementos interactivos.

### 11.2 Sistema de Theming — ThemeReader

**Archivo:** `~/.local/state/omarchy/current/theme/colors.toml`

```toml
[colors]
background = "#1a1a2e"
foreground = "#e0e0e0"
accent     = "#7aa2f7"
surface    = "#16213e"
border     = "#2a2a4a"
error      = "#f7768e"
```

- Parser TOML manual con `QRegularExpression`.
- `QFileSystemWatcher` para recarga en caliente.
- Singleton en QML: `import OmaDo.Theme 1.0` → `Theme.background`, `Theme.accent`, etc.

### 11.3 Tipografía

- **iA Writer Mono** (bundled, OFL 1.1). Tamaño base: 13px.
- Registro: `QFontDatabase::addApplicationFont(":/fonts/iAWriterMono-Regular.ttf")`.

### 11.4 CheckCircle — El Checkbox de OmaDo

El checkbox de tareas es un círculo vacío (`○`), no un cuadrado. Al completar la tarea,
el círculo se llena o se tacha:

```qml
// CheckCircle.qml
Rectangle {
    width: 18; height: 18
    radius: 9   // círculo perfecto
    color: isCompleted ? Theme.accent : "transparent"
    border.color: isCompleted ? Theme.accent : Theme.border
    border.width: 1.5

    // Tick o tilde al completar
    Text {
        visible: isCompleted
        text: "✓"
        color: Theme.background
        font.pixelSize: 11
        anchors.centerIn: parent
    }
}
```

Este componente es el `CheckCircle.qml` y se usa tanto en `TaskDelegate` como en `InputBar`.

### 11.5 Navegación por Teclado

| Tecla | Acción |
|---|---|
| `Tab` / `Shift+Tab` | Alternar foco entre SidePanel y TaskView |
| `↑` / `↓` | Mover foco entre ítems de la lista activa |
| `Space` | Toggle completada en la tarea con foco |
| `Enter` | Confirmar tarea nueva / abrir detalle de tarea con foco |
| `N` | Activar InputBar (foco al campo de texto) |
| `→` / `F2` | Abrir TaskDetail del ítem con foco |
| `Delete` | Eliminar ítem con foco (confirmación inline en la barra) |
| `Escape` | Cancelar edición / cerrar overlay de detalle |
| `Ctrl+R` | Forzar sync (solo si sync activo) |
| `Ctrl+Q` | Cerrar aplicación |
| `Ctrl+1` | Ir a My Day |
| `Ctrl+2` | Ir a Schedule |
| `Ctrl+3` | Ir a Tasks (global) |
| `Ctrl+4..9` | Ir a lista custom por índice |

### 11.6 Estados del Footer "Log in on MS To Do"

```
Estado: No autenticado
  → Mostrar: texto "Log in on MS To Do" en color Theme.accent (clickeable)
  → Al clic: abrir AuthView (overlay)

Estado: Autenticando
  → Mostrar: texto "Connecting..." + indicador de carga

Estado: Autenticado
  → Mostrar: email del usuario conectado + "●" verde
  → Opcional: al clic → popup con opción "Disconnect"
```

---

## 12. Arquitectura de Modo Demonio + Plugin de Panel (v1.1)

### 12.1 Modo Demonio

```
omado --daemon
```

- `QCoreApplication` — sin motor QML, sin ventana.
- Comparte `LocalRepository`, `NotificationService`, opcionalmente `GraphClient`.
- Sync automático cada 5 minutos (si sync activo).
- Monitor de reminders (QTimer, 60s).
- Servicio D-Bus: `io.omarchy.OmaDo`.

### 12.2 Interfaz D-Bus

```xml
<interface name="io.omarchy.OmaDo">
  <method name="GetLists">
    <arg direction="out" type="a(ss)" name="lists"/>
  </method>
  <method name="GetTasksForToday">
    <arg direction="out" type="aa{sv}" name="tasks"/>
  </method>
  <method name="GetPendingCount">
    <arg direction="in"  type="s" name="listId"/>
    <arg direction="out" type="i" name="count"/>
  </method>
  <method name="GetTotalPendingCount">
    <arg direction="out" type="i" name="count"/>
  </method>
  <method name="ToggleTask">
    <arg direction="in"  type="s" name="taskId"/>
    <arg direction="in"  type="b" name="completed"/>
    <arg direction="out" type="b" name="success"/>
  </method>
  <signal name="TasksChanged">
    <arg type="s" name="listId"/>
  </signal>
  <signal name="TodayTasksChanged"/>
</interface>
```

### 12.3 Plugin de Panel Quickshell (v1.1)

Plugin QML independiente (`~/.config/omarchy/plugins/omado/`) que consume el D-Bus de OmaDo.
Vive en su propio repositorio. OmaDo no contiene código del plugin.

---

## 13. Dependencias del Sistema

| Paquete | Uso | Ya en Omarchy |
|---|---|---|
| `qt6-base` | Core, Network, Sql, DBus, Concurrent | ✅ Sí |
| `qt6-declarative` | QML engine | ✅ Sí |
| `qt6-quickcontrols2` | Controls básicos QML | ✅ Sí |
| `libsecret` | Backend Secret Service | ✅ Sí |
| `gnome-keyring` | Daemon del Secret Service | ✅ Sí |
| `qtkeychain` | Wrapper Qt para libsecret | ❌ Instalar |
| `cmake` | Sistema de build | ❌ Instalar |

```bash
sudo pacman -S cmake qtkeychain-qt6
```

---

## 14. Build System — CMake

```cmake
cmake_minimum_required(VERSION 3.21)
project(omado VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Sql DBus Concurrent)
find_package(Qt6Keychain REQUIRED)

qt_add_executable(omado src/main.cpp ...)
qt_add_qml_module(omado URI "OmaDo" VERSION "1.0" ...)

target_link_libraries(omado PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    Qt6::Network Qt6::Sql Qt6::DBus Qt6::Concurrent
    Qt6Keychain::Qt6Keychain
)
```

---

## 15. Convenciones de Código

- **C++:** `PascalCase` clases, `camelCase` métodos, `m_` prefix miembros privados.
- **QML:** `camelCase` propiedades e IDs, un componente por archivo.
- **Señales/Slots:** Puntero a función siempre. Nunca macros `SIGNAL()`/`SLOT()`.
- **Memoria:** Parent-ownership de Qt. `reply->deleteLater()` siempre.
- **SQL:** Solo via `LocalRepository`. `prepare()` + `bindValue()` obligatorio.
- **Logging:** `qDebug()` trazas, `qWarning()` errores recuperables.
