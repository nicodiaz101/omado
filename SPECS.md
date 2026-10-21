# SPECS.md — Especificaciones Técnicas de OmaDo
> Cliente nativo de tareas para Omarchy (Quattro / 4.5+), con sincronización opcional con Microsoft To Do / Samsung Reminders
> **Stack:** C++17 · Qt 6 · QML/Qt Quick · CMake 3.21+ · **Versión:** 2.0.0

---

## 1. Visión General

OmaDo sigue la filosofía minimalista de [OmaWrite](https://github.com/omacom-io/omawrite),
[OmaCalc](https://github.com/omacom-io/omacalc) y las aplicaciones nativas de Omarchy 4.5:
nativo, sin dependencias pesadas, integrado al 100% con el sistema de theming, notificaciones,
idiomas y modo de workspaces flotantes/tiling de Omarchy.

### Principio Arquitectural Fundamental: Local-First + Integración Nativa Omarchy 4.5

```
┌────────────────────────────────────────────────────────────────┐
│                       OmaDo v2.0                               │
│            Gestor de tareas 100% offline-first                 │
│        Almacenamiento: SQLite (WAL) via Qt6::Sql               │
│        GUI + Quick-Add + CLI + Daemon D-Bus (io.omarchy.OmaDo) │
└──────────────┬────────────────────────────────┬────────────────┘
               │ Nativo Omarchy 4.5             │ Opcional (OAuth2 PKCE)
┌──────────────▼─────────────────┐ ┌────────────▼─────────────────────┐
│ Integración Omarchy 4.5        │ │ MS To Do / Samsung Reminders     │
│ • Omapager (Acciones, Links,   │ │ • Microsoft Graph API v1.0       │
│   Urgencia, Transient, Icono)  │ │ • Sync bidireccional (listas,    │
│ • Idiomas en vivo (~/.config/  │ │   tareas, checklistItems)        │
│   omarchy/language)            │ └──────────────────────────────────┘
│ • Dock y Workspaces Flotantes  │
│ • Plugin Quickshell omado.panel│
└────────────────────────────────┘
```

**La sincronización con MS To Do es un módulo opcional**, no el núcleo. El núcleo es un
gestor de tareas local completo con paridad de features de MS To Do / Samsung Reminders.

El binario final debe arrancar en menos de 200 ms en hardware moderno (y `< 100 ms` en modo `--quick-add` o CLI).

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
├── PKGBUILD
├── install.sh / uninstall.sh
├── omado.desktop
├── io.omarchy.OmaDo.xml
├── LICENSE
├── README.md
│
├── src/
│   ├── main.cpp                        # Punto de entrada: GUI, --daemon, --quick-add o CLI
│   │
│   ├── core/
│   │   ├── ThemeReader.h/.cpp          # Parser TOML colors.toml → Q_PROPERTY para QML
│   │   ├── Translator.h/.cpp           # QTranslator JSON en vivo (~/.config/omarchy/language)
│   │   ├── Database.h/.cpp             # Inicialización y migraciones del schema SQLite (v1–v4+)
│   │   ├── AuthManager.h/.cpp          # OAuth2 PKCE via QNetworkAccessManager
│   │   ├── KeychainStore.h/.cpp        # Wrapper qtkeychain: read/write/delete tokens
│   │   ├── NotificationService.h/.cpp  # Notificaciones D-Bus (Omapager 4.5: actions, hints, signals)
│   │   ├── ReminderService.h/.cpp      # Monitor de recordatorios + handler de acciones D-Bus
│   │   ├── SyncEngine.h/.cpp           # Coordinador de sincronización bidireccional
│   │   ├── TaskInputParser.h/.cpp      # Parser NLP C++ (fechas, horas, !, #Lista, recurrencia)
│   │   ├── CliRunner.h/.cpp            # Subcomandos CLI (add, list, done, sync) via D-Bus / DB
│   │   └── DaemonService.h/.cpp        # Adaptador QtDBus: servicio io.omarchy.OmaDo
│   │
│   ├── models/
│   │   ├── TaskList.h                  # Struct TaskList y OmaDoListEntry (D-Bus)
│   │   ├── Task.h                      # Struct Task
│   │   ├── TaskStep.h                  # Struct TaskStep (subtareas/checklistItems)
│   │   ├── TaskListModel.h/.cpp        # QAbstractListModel para listas + emojis/renombrado
│   │   ├── TaskModel.h/.cpp            # QAbstractListModel para tareas, filtros, orden y sugerencias
│   │   ├── LocalRepository.h/.cpp      # CRUD asíncrono contra SQLite (fuente de verdad local)
│   │   └── GraphClient.h/.cpp          # Red: QNAM + Microsoft Graph API v1.0
│   │
│   └── qml/
│       ├── Main.qml                    # ApplicationWindow frameless principal
│       ├── QuickAddWindow.qml          # Mini-ventana flotante (--quick-add) centrada con NLP en vivo
│       ├── SidePanel.qml               # Panel izquierdo completo (Zona A)
│       ├── TaskView.qml                # Zona B: lista de tareas + sugerencias de Mi Día
│       ├── TaskDetail.qml              # Panel lateral: notas (con links clicables), steps, metadatos
│       ├── InputBar.qml                # Zona C: barra inferior con badges NLP en tiempo real
│       ├── TaskDelegate.qml            # Ítem de tarea con CheckCircle, badges e importancia
│       └── components/
│           ├── AppIcon.qml             # Renderizador de iconos SVG monocromáticos teñidos con Theme
│           ├── CheckCircle.qml         # Círculo de checkbox (outline/filled)
│           ├── SectionSeparator.qml    # Separador horizontal del SidePanel
│           ├── DatePickerPopup.qml     # Selector de fecha de vencimiento
│           ├── TimePickerPopup.qml     # Selector de hora de recordatorio
│           ├── DraftStepsPopup.qml     # Popup para subtareas en borrador desde InputBar
│           ├── RecurrencePopup.qml     # Selector de recurrencia
│           ├── ListContextMenu.qml     # Menú contextual nativo para listas personalizadas
│           ├── ListIconPickerPopup.qml # Selector de emojis/iconos por categorías
│           ├── ListRenameDialog.qml    # Diálogo inline/overlay de renombrado de lista
│           ├── MyDaySuggestionsPopup.qml # Panel/banner de sugerencias de tareas vencidas para Mi Día
│           └── ShortcutHelpOverlay.qml # Overlay de ayuda de atajos de teclado (?)
│
├── i18n/
│   ├── en.json                         # Catálogo base inglés (plurales CLDR)
│   ├── es.json                         # Catálogo español completo
│   └── ...                             # Catálogos compatibles con omarchy language
│
├── fonts/
│   ├── iAWriterMono-Regular.ttf
│   └── iAWriterMono-Bold.ttf
│
├── icons/
│   └── *.svg                           # Iconografía vectorial monocromática
│
├── autostart/
│   └── omado-daemon.desktop
│
├── systemd/
│   └── omado.service
│
└── tests/
    ├── CMakeLists.txt
    ├── fixtures/
    ├── tst_ThemeReader.cpp
    ├── tst_Database.cpp
    ├── tst_LocalRepository.cpp
    ├── tst_NotificationService.cpp
    ├── tst_ReminderService.cpp
    ├── tst_DaemonService.cpp
    ├── tst_AuthManager.cpp
    ├── tst_GraphClient.cpp
    ├── tst_TaskModel.cpp
    ├── tst_TaskListModel.cpp
    ├── tst_TaskInputParser.cpp
    ├── tst_Translator.cpp
    └── tst_CliRunner.cpp
```

---

## 5. Almacenamiento Local — SQLite

### 5.1 Stack de Datos

- **Motor:** SQLite via `Qt6::Sql` (`QSqlDatabase`, `QSqlQuery`) — incluido en `qt6-base`.
- **Ubicación del archivo:** `QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/omado.db"`.
- **Concurrencia:** `QThreadPool` dedicado (1 worker persistente con `QThreadStorage`) en `LocalRepository`.
- **Migraciones:** Sistema de versiones en tabla `schema_version` (actualmente v4). `Database::initialize()` aplica migraciones transaccionales en orden al arrancar.
- **SQLite WAL mode** (`PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON; PRAGMA busy_timeout=5000; PRAGMA cache_size=-512;`).

### 5.2 Schema SQL (v1–v4)

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
    due_date        TEXT,           -- ISO 8601 date (YYYY-MM-DD)
    reminder_at     TEXT,           -- ISO 8601 datetime
    reminded        INTEGER NOT NULL DEFAULT 0,
    recurrence      TEXT NOT NULL DEFAULT 'none',
    sort_order      INTEGER NOT NULL DEFAULT 0,
    created_at      TEXT NOT NULL,
    updated_at      TEXT,
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

CREATE TABLE IF NOT EXISTS deleted_tasks (
    id              TEXT PRIMARY KEY,
    remote_id       TEXT NOT NULL,
    list_remote_id  TEXT,
    deleted_at      TEXT NOT NULL
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

### 5.3 LocalRepository

`LocalRepository : QObject` es la única clase autorizada a ejecutar SQL.

```
Métodos principales (QFuture<T> via QtConcurrent::run):
  fetchLists()                          → QFuture<QList<TaskList>>
  fetchTasks(listId)                    → QFuture<QList<Task>>      (lista real)
  fetchMyDayTasks()                     → QFuture<QList<Task>>      (is_my_day=1 o due_date=hoy o reminder hoy)
  fetchOverdueTasks()                   → QFuture<QList<Task>>      (is_completed=0 y due_date < hoy o reminder_at < hoy no reprogramadas)
  fetchScheduleTasks()                  → QFuture<QList<Task>>      (due_date IS NOT NULL, ordenado)
  fetchAllTasks()                       → QFuture<QList<Task>>      (todas, para vista Tasks)
  createList(name)                      → QFuture<TaskList>
  updateList(list)                      → QFuture<bool>
  deleteList(id)                        → QFuture<bool>
  createTask(task)                      → QFuture<Task>
  updateTask(task)                      → QFuture<bool>
  deleteTask(id, recordForRemoteSync)   → QFuture<bool>
  toggleTask(taskId, completed)         → QFuture<bool>
  rescheduleTaskForMyDay(taskId, now)   → QFuture<bool>             (is_my_day=1, due_date=hoy, ajusta reminder_at conservando hora o +1h y reminded=0)
  snoozeTaskReminder(taskId, minutes)   → QFuture<bool>             (reminder_at=now+minutes, reminded=0)
  markReminded(taskId, reminded)        → QFuture<bool>
  addStep(taskId, title)                → QFuture<TaskStep>
  updateStep(step)                      → QFuture<bool>
  deleteStep(stepId)                    → QFuture<bool>
  getPendingCount(listId)               → QFuture<int>
  getTotalPendingCount()                → QFuture<int>
  getPendingReminders()                 → QFuture<QList<Task>>      (reminder_at <= now AND is_completed=0 AND reminded=0)
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

## 9. Notificaciones del Sistema (Motor Omapager — Omarchy 4.5)

OmaDo se comunica directamente con `org.freedesktop.Notifications` mediante `Qt6::DBus` (implementado por `omarchy.notifications` / Omapager dentro de `omarchy-shell` en Omarchy 4.5). Sin `QProcess`.

### 9.1 Firma y Capacidades de `NotificationService`

```cpp
struct NotificationOptions {
    QString     iconName   = QStringLiteral("omado");
    int         timeoutMs  = 5000;
    uint        replacesId = 0u;
    uchar       urgency    = 1;     // 0=Low, 1=Normal, 2=Critical
    bool        transient  = false; // true = no persiste en history.json de Omarchy 4.5
    QStringList actions;            // pares ["action_key", "Label", ...]
};
```

```cpp
QVariantMap hints;
hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("omado"));
hints.insert(QStringLiteral("urgency"), QVariant::fromValue<uchar>(opts.urgency));
hints.insert(QStringLiteral("transient"), opts.transient);

QDBusMessage reply = iface.call(
    QStringLiteral("Notify"),
    QStringLiteral("OmaDo"),
    opts.replacesId,
    opts.iconName,
    title,
    body,
    opts.actions,
    hints,
    opts.timeoutMs
);
uint notificationId = (!reply.arguments().isEmpty()) ? reply.arguments().first().toUInt() : 0u;
```

### 9.2 Integración Profunda con Omapager (Omarchy 4.5)

1. **Identidad, Agrupación y Snooze por App:**
   - Se envía siempre `app_name = "OmaDo"`, `app_icon = "omado"` y `hints["desktop-entry"] = "omado"`. Esto garantiza que `Icons.js` y `Markup.js` de Omarchy 4.5 agrupen las tarjetas bajo la clave `app:omado`, muestren el icono vectorial de OmaDo y habiliten el pospuesto (*Snooze*) por aplicación desde el centro de notificaciones.
2. **Botones de Acción Interactivos (`ActionInvoked`):**
   - Cada recordatorio de tarea incluye acciones D-Bus en la tarjeta:
     - `"complete"` → `"Complete"` / `"Completar"`: marca la tarea como completada en `LocalRepository`, encola sync si corresponde y emite `TasksChanged` / `TodayTasksChanged`.
     - `"snooze-15"` → `"Snooze 15 min"` / `"Posponer 15 min"`: reprograma `reminder_at = now + 15 min` con `reminded = 0`.
   - `NotificationService` se suscribe a las señales `ActionInvoked(uint id, QString action_key)` y `NotificationClosed(uint id, uint reason)` de `org.freedesktop.Notifications` en el bus de sesión.
   - `ReminderService` mantiene un mapa en memoria `QHash<uint, QString> m_activeNotifications` (`notificationId -> taskId`) para despachar la acción sobre la tarea exacta incluso sin abrir la ventana de OmaDo.
3. **Links Clicables, Reuniones y Resumen de Subtareas:**
   - El motor `Detect.js` y `Markup.js` de Omarchy 4.5 detecta automáticamente URLs (`https://...`) y enlaces de videollamadas (Google Meet, Zoom, Microsoft Teams, Jitsi) en el `body` de la notificación, añadiendo el botón nativo *"Open Link"* / *"Join"* en la tarjeta.
   - `ReminderService` construye el cuerpo de la notificación combinando:
     - El conteo de subtareas si la tarea tiene `steps` (ej. `"📋 1/3 pasos completados"`).
     - El contenido de `task.body` (preservando las URLs para que Omapager las haga clicables).
4. **Niveles de Urgencia e Historial Persistente:**
   - Tareas con `importance == "high"` se envían con `urgency = 2` (`Critical`), destacando con `Commons.Color.urgent` en Omarchy 4.5 y permaneciendo visibles hasta interacción del usuario.
   - Recordatorios normales usan `urgency = 1` (`Normal`) y `transient = false` (quedan registrados en el historial de 24h de Omarchy 4.5 si el usuario está en modo *Do Not Disturb* o *Snooze*).
   - Notificaciones meramente informativas usan `transient = true` para no ensuciar el historial.

---

## 10. Modelos de Datos C++

### 10.1 Structs

```cpp
struct TaskList {
    QString   id;
    QString   displayName;
    bool      isSpecial   = false;  // My Day, Schedule, Tasks (vistas virtuales)
    int       sortOrder   = 0;
    QString   remoteId;
    QDateTime syncedAt;
    bool      isSynced() const { return !remoteId.isEmpty(); }
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
    QString         importance   = "normal"; // "low" | "normal" | "high"
    QDate           dueDate;
    QDateTime       reminderAt;
    bool            reminded     = false;
    QString         recurrence   = "none";   // "none"|"daily"|"weekly"|"monthly"|"yearly"
    QList<TaskStep> steps;
    int             sortOrder    = 0;
    QDateTime       createdAt;
    QDateTime       updatedAt;
    QDateTime       completedAt;
    QDateTime       syncedAt;
    QString         remoteId;
};
```

### 10.2 Modelos QML

- `TaskListModel : QAbstractListModel` — roles: `IdRole`, `DisplayNameRole`, `NameRole` (sin emoji), `IconRole` (emoji), `IsSpecialRole`, `SortOrderRole`, `PendingCountRole`.
- `TaskModel : QAbstractListModel` — roles: `IdRole`, `ListIdRole`, `TitleRole`, `BodyRole`, `IsCompletedRole`, `IsMyDayRole`, `DueDateRole`, `ImportanceRole`, `ReminderAtRole`, `RecurrenceRole`, `StepsRole`, `HasStepsRole`, `StepsCountRole`, `CompletedStepsCountRole`.
  - Propiedades adicionales: `hideCompleted`, `sortByDueDate`, `overdueSuggestions` (`QVariantList` de tareas vencidas sugeridas para *My Day*), `overdueCount`.

---

## 11. UI — QML / Qt Quick e Integraciones de Escritorio 4.5

### 11.1 Ventana, Barra de Título en Modo Flotante (`hyprbars`) e Identidad en `omarchy.dock`

En Omarchy 4.5, cuando el usuario activa el modo de **Workspaces Flotantes con Dock** (`Super + L` o como layout por defecto para quienes prefieren usar el escritorio en modo ventana con mouse), el compositor carga automáticamente `/usr/lib/omarchy-hyprland-titlebars/titlebars.so` (`hyprbars`) sobre todas las ventanas con tag `omarchy-floating-workspace`.

`hyprbars` inyecta del lado del compositor (server-side) una **barra de título nativa de 30px** con los colores del tema activo (`lighter_background` / `foreground`) que ya incluye a la derecha los **3 botones de control de ventana**:
- **`−` (Minimizar al Dock):** Ejecuta `omarchy-hyprland-window-minimize`, enviando la ventana a `special:minimized` para que `omarchy.dock` la retenga y la restaure al hacer clic en su icono.
- **`□` (Maximizar / Restaurar):** Alterna el estado maximizado en el workspace flotante (también al hacer doble clic en la barra de título).
- **`×` (Cerrar):** Cierra la ventana limpiamente.

Para que OmaDo se integre sin fisuras tanto en modo *Tiling* como en modo *Floating + Dock*:

```cpp
// En main.cpp — obligatorio para que omarchy.dock y hyprbars asocien la ventana a omado.desktop
app.setDesktopFileName(QStringLiteral("omado"));
app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/OmaDo/icons/omado.svg")));
```

- **Geometría y bordes en `Main.qml`:**
  - Usar `ApplicationWindow` estándar de Wayland (permitiendo que Hyprland gestione las decoraciones *server-side* vía `xdg-decoration`) y dimensiones mínimas (`minimumWidth: 640`, `minimumHeight: 460`) para que el redimensionado con el mouse en modo ventana no rompa el layout de 3 paneles.
  - Eliminar el `radius: 8` fijo del contenedor raíz en `Main.qml` (usando `radius: 0` para que coincida con el `decoration:rounding = 0` de Hyprland y encaje al píxel debajo de la barra de título plana de 30px de `hyprbars` sin dejar huecos transparentes en las esquinas superiores ni doble borde).
  - Conservar el `DragHandler` (`window.startSystemMove()`) en las zonas libres del `SidePanel` y encabezado de `TaskView` para permitir arrastrar la ventana desde adentro incluso si alguien la flota individualmente con `Super + T` en un workspace tiling.

### 11.2 Sistema de Theming — ThemeReader

**Archivo:** `~/.local/state/omarchy/current/theme/colors.toml`

- Parser TOML manual con `QRegularExpression` (soporta `background`, `foreground`, `accent`, `surface`/`lighter_background`, `border`/`muted`, `error`/`red`).
- `QFileSystemWatcher` sobre el archivo y el directorio padre para recarga en caliente ante atomic renames de `omarchy theme set`.
- Singleton en QML: `import OmaDo.Theme 1.0` → `Theme.background`, `Theme.accent`, etc.

### 11.3 Tipografía

- **iA Writer Mono** (bundled, OFL 1.1). Tamaño base: 13px.
- Registro: `QFontDatabase::addApplicationFont(":/qt/qml/OmaDo/fonts/iAWriterMono-Regular.ttf")`.

### 11.4 CheckCircle — El Checkbox de OmaDo

El checkbox de tareas es un círculo vacío (`○`), nunca un cuadrado (`CheckCircle.qml`).

### 11.5 Navegación por Teclado (Keyboard-First)

| Tecla | Acción |
|---|---|
| `Tab` / `Shift+Tab` | Alternar foco entre SidePanel y TaskView |
| `↑` / `↓` o `k` / `j` | Mover foco entre ítems de la lista activa (cuando no hay un input de texto enfocado) |
| `Space` | Toggle completada en la tarea seleccionada |
| `Enter` / `F2` | Confirmar tarea nueva / abrir o enfocar `TaskDetail` de la tarea seleccionada |
| `N` | Activar `InputBar` (foco directo al campo de texto) |
| `Delete` | Eliminar tarea seleccionada |
| `Escape` | Cancelar edición / cerrar `TaskDetail` o popups / cerrar modo `--quick-add` |
| `Ctrl+R` | Forzar sincronización con MS To Do |
| `Ctrl+1` / `Ctrl+2` / `Ctrl+3` | Ir a *My Day* / *Schedule* / *Tasks* |
| `?` | Mostrar/ocultar overlay de atajos de teclado (`ShortcutHelpOverlay.qml`) |
| `Ctrl+Q` | Cerrar aplicación |

### 11.6 Internacionalización en Vivo — `Translator` (Omarchy 4.5)

Omarchy 4.5 soporta 50 idiomas configurables con `omarchy language`, guardando el código activo en `~/.config/omarchy/language`.

- `Translator : public QTranslator` lee `~/.config/omarchy/language` (con fallback a `QLocale::system().name()` y a `"en"`) y vigila cambios en tiempo real con `QFileSystemWatcher`.
- Los catálogos se empaquetan como recursos JSON planos en `:/qt/qml/OmaDo/i18n/<code>.json` (ej. `en.json`, `es.json`), mapeando el string fuente en inglés a su traducción o a un objeto de formas plurales CLDR (`{"one": "%n task", "other": "%n tasks"}`).
- Al cambiar el idioma en Omarchy, `Translator` recarga el catálogo, actualiza `QLocale::setDefault(...)`, emite `changed()` e invoca `QQmlEngine::retranslate()` sin reiniciar OmaDo.

### 11.7 Sugerencias de "Mi Día" y Reprogramación de Vencidas

- En la vista **My Day**, cuando existen tareas incompletas con `due_date < hoy` o `reminder_at < hoy`, `TaskView.qml` muestra un indicador/banner compacto de **"Tareas vencidas sugeridas (N)"**.
- Al desplegar el panel de sugerencias (`MyDaySuggestionsPopup.qml`), el usuario puede agregar tareas vencidas a **Mi Día** individualmente o todas juntas con un clic:
  - Se marca `is_my_day = 1`.
  - Se actualiza `due_date = QDate::currentDate()`.
  - Si la tarea tenía un `reminder_at` en el pasado, se conserva su **hora original (`reminder_at.time()`)** moviendo la fecha a **hoy**; si esa hora ya pasó en el día actual (`originalTime <= now.time()`), se reprograma a **`now + 1 hora`** y se resetea `reminded = 0` para que vuelva a sonar hoy en Omarchy 4.5.

### 11.8 Parser de Lenguaje Natural (`TaskInputParser` v2.0)

`TaskInputParser::parse(input, now)` extrae en C++ (bilingüe español/inglés) y limpia del título:
1. **Fechas relativas, días de la semana y fechas numéricas/textuales:** `hoy`/`today`, `mañana`/`tomorrow`, `pasado mañana`/`day after tomorrow`, `próximo lunes`/`next monday`, `25/10`, `15 de octubre`/`Oct 15`.
2. **Horas:** `a las 18:00`, `at 6pm`, `15hs`, `mediodía`/`noon`, `medianoche`/`midnight`.
3. **Importancia alta (`!` / `!!`):** Token `!` o `!!` (aislado o al final) → `importance = "high"`.
4. **Lista destino (`#NombreLista`):** Token `#Facultad` o `#"Compras Casa"` → `targetListName = "Facultad"`.
5. **Recurrencia:** `diario`/`daily`/`cada día`/`every day`, `semanal`/`weekly`/`cada semana`/`every week`, `mensual`/`monthly`/`cada mes`, `anual`/`yearly`/`cada año` → `recurrence = "daily" | "weekly" | "monthly" | "yearly"`.

### 11.9 Modo Flotante `omado --quick-add`

- Invocado mediante `omado --quick-add` (ideal para bindear en Hyprland o lanzar desde `omado.panel`).
- Abre `QuickAddWindow.qml`: ventana centrada frameless (`540x96`), siempre al frente, con bordes e iconografía del tema activo.
- Muestra el `TextInput` con chips/badges en vivo para fecha, hora, importancia (`!`), lista (`#`) y recurrencia.
- Al presionar `Enter`: envía la tarea al demonio vía D-Bus `CreateTaskFromNaturalText(text, true)` (o la guarda localmente vía `LocalRepository` si el demonio no está activo) y cierra el proceso en el acto. `Escape` cancela y cierra.

### 11.10 Notas con Links Clicables en `TaskDetail.qml`

- Cuando el campo `body` o `title` de la tarea seleccionada contiene URLs (`https?://...`), `TaskDetail.qml` muestra chips de acción rápida debajo del área de notas que abren el enlace con `QDesktopServices::openUrl(QUrl(url))` en un clic.

---

## 12. Arquitectura de Modo Demonio, CLI y Plugin de Panel (v2.0)

### 12.1 Modo Demonio (`omado --daemon`)

- `QCoreApplication` — sin motor QML, sin ventana.
- Instancia `LocalRepository`, `NotificationService`, `ReminderService`, `KeychainStore`, `AuthManager`, `GraphClient`, `SyncEngine` y `DaemonService`.
- Escucha señales de `org.freedesktop.Notifications` (`ActionInvoked`, `NotificationClosed`) para completar o posponer tareas directamente desde los toasts de Omarchy 4.5.
- En modo GUI, la ventana principal también se suscribe a las señales `TasksChanged` y `TodayTasksChanged` del demonio para refrescar `TaskModel` y `TaskListModel` en tiempo real cuando una tarea cambia desde una notificación, desde `omado.panel` o desde la CLI.

### 12.2 Interfaz D-Bus (`io.omarchy.OmaDo`)

```xml
<interface name="io.omarchy.OmaDo">
  <method name="GetLists">
    <arg direction="out" type="a(ss)"/>
    <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="QList&lt;OmaDoListEntry&gt;"/>
  </method>
  <method name="GetTasksForToday">
    <arg direction="out" type="s"/>
  </method>
  <method name="GetOverdueTasks">
    <arg direction="out" type="s"/>
  </method>
  <method name="GetPendingCount">
    <arg direction="out" type="i"/>
    <arg direction="in"  type="s" name="listId"/>
  </method>
  <method name="GetTotalPendingCount">
    <arg direction="out" type="i"/>
  </method>
  <method name="ToggleTask">
    <arg direction="out" type="b"/>
    <arg direction="in"  type="s" name="taskId"/>
    <arg direction="in"  type="b" name="completed"/>
  </method>
  <method name="DeleteTask">
    <arg direction="out" type="b"/>
    <arg direction="in"  type="s" name="taskId"/>
  </method>
  <method name="CreateTaskFromNaturalText">
    <arg direction="out" type="s"/>
    <arg direction="in"  type="s" name="rawInput"/>
    <arg direction="in"  type="b" name="addToMyDay"/>
  </method>
  <method name="PreviewNaturalText">
    <arg direction="out" type="s"/>
    <arg direction="in"  type="s" name="rawInput"/>
  </method>
  <method name="RescheduleTaskForMyDay">
    <arg direction="out" type="b"/>
    <arg direction="in"  type="s" name="taskId"/>
  </method>
  <method name="SnoozeTask">
    <arg direction="out" type="b"/>
    <arg direction="in"  type="s" name="taskId"/>
    <arg direction="in"  type="i" name="minutes"/>
  </method>
  <method name="RequestSync"/>
  <signal name="TasksChanged">
    <arg type="s" name="listId"/>
  </signal>
  <signal name="TodayTasksChanged"/>
  <signal name="SyncStarted"/>
  <signal name="SyncFinished">
    <arg type="b" name="success"/>
    <arg type="s" name="message"/>
  </signal>
</interface>
```

### 12.3 Subcomandos CLI (`CliRunner`)

El binario `omado` soporta invocación directa sin interfaz gráfica para scripts, agentes y keybindings de Hyprland:

- `omado add "<texto NLP>" [--my-day]` → Crea una tarea parseando lenguaje natural (vía D-Bus al demonio o directo en SQLite).
- `omado list [--today | --overdue | --json]` → Lista tareas pendientes en texto plano o JSON.
- `omado done <taskId>` → Marca la tarea como completada.
- `omado reschedule <taskId>` → Reprograma una tarea vencida para *Mi Día*.
- `omado sync` → Dispara `RequestSync` en el demonio.

### 12.4 Plugin de Panel Quickshell (`omado.panel`)

Plugin QML independiente (`~/.config/omarchy/plugins/omado.panel/` / repositorio `omado-plugin`) que consume el servicio D-Bus `io.omarchy.OmaDo`. En v2.0 puede invocar `CreateTaskFromNaturalText(text, true)`, `PreviewNaturalText(text)`, `GetOverdueTasks()` y `RescheduleTaskForMyDay(taskId)`.

---

## 13. Dependencias del Sistema y Empaquetado Multi-Arquitectura

| Paquete | Uso | Ya en Omarchy 4.5 |
|---|---|---|
| `qt6-base` | Core, Gui, Network, Sql, DBus, Concurrent | ✅ Sí |
| `qt6-declarative` | QML engine, Quick, QuickControls2 | ✅ Sí |
| `qt6-svg` | Renderizado de iconos SVG monocromáticos | ✅ Sí |
| `libsecret` / `gnome-keyring` | Secret Service API | ✅ Sí |
| `qtkeychain-qt6` | Wrapper Qt6 para libsecret | Instalar via pacman |

- **Arquitecturas soportadas en `PKGBUILD`:** `arch=('x86_64' 'aarch64')` (soporte nativo para PCs x86_64, Apple Silicon Mac y laptops ARM64 de Omarchy 4.5).

---

## 14. Build System — CMake

```cmake
cmake_minimum_required(VERSION 3.21)
project(omado VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Sql DBus Concurrent)
find_package(Qt6Keychain REQUIRED)
```

---

## 15. Convenciones de Código

- **C++:** `PascalCase` clases, `camelCase` métodos, `m_` prefix miembros privados.
- **QML:** `camelCase` propiedades e IDs, un componente por archivo.
- **Señales/Slots:** Puntero a función siempre. Nunca macros `SIGNAL()`/`SLOT()`.
- **Memoria:** Parent-ownership de Qt. `reply->deleteLater()` siempre.
- **SQL:** Solo via `LocalRepository`. `prepare()` + `bindValue()` obligatorio.
- **Logging:** `qDebug()` trazas, `qWarning()` errores recuperables.
