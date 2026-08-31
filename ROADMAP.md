# ROADMAP.md — Hitos de Desarrollo de OmaDo

> **v1.0:** Gestor de tareas local completo (offline-first)
> **v1.1:** Sincronización opcional con MS To Do + Plugin de panel Quickshell
> **Convención:** 🔲 Pendiente · 🔄 En progreso · ✅ Completado

---

## Hito 1 — Fundación: Estructura, Ventana, Theming y Base de Datos
> **Objetivo:** Proyecto compilable con ventana frameless, colores dinámicos y schema SQLite funcional.
> **Criterio de éxito:** `cmake --build . && ./omado` abre una ventana con el tema activo de Omarchy.
> La app puede crear, listar y marcar tareas localmente sin internet. Re-tiñe en caliente con `colors.toml`.

### 1.1 Esqueleto CMake
- ✅ `CMakeLists.txt` raíz con `cmake_minimum_required(VERSION 3.21)`.
- ✅ `find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Sql DBus Concurrent)`.
- ✅ `find_package(Qt6Keychain REQUIRED)` — preparado para Hito 3.
- ✅ Target de tests separado en `tests/CMakeLists.txt` con `Qt6::Test`.
- ✅ Librería interna `omado_lib` (estática) compartida entre app y tests.
- ✅ `.gitignore` con exclusiones para `build/`, `CMakeCache.txt`, `*.user`, binarios.
- ✅ Compilación limpia en Debug y Release.

### 1.2 Ventana Frameless
- ✅ `main.qml`: `ApplicationWindow { flags: Qt.FramelessWindowHint | Qt.Window; color: "transparent" }`.
- ✅ `DragHandler` para `window.startSystemMove()`.
- ✅ `Rectangle { radius: 8 }` como contenedor raíz.
- ✅ Botones de control (cerrar/minimizar) en QML puro con iconos de texto (`✕`, `−`).
- ✅ Validar en Hyprland/Wayland: sin decoraciones del compositor, bordes redondeados correctos.

### 1.3 ThemeReader — Parser TOML
- ✅ `ThemeReader : QObject` con parser TOML línea a línea via `QRegularExpression`.
- ✅ `Q_PROPERTY` para: `background`, `foreground`, `accent`, `surface`, `border`, `error`.
- ✅ `qmlRegisterSingletonInstance<ThemeReader>` → accesible como `Theme.*` en QML.
- ✅ `QFileSystemWatcher` sobre `~/.local/state/omarchy/current/theme/colors.toml`.
- ✅ `fileChanged` → re-parsear → emitir signals `Q_NOTIFY` → re-bind automático en QML.
- ✅ Fallback a colores hardcodeados si el archivo no existe.
- ✅ Tests en `tst_ThemeReader.cpp`: TOML válido, sección faltante, clave ausente, color malformado, archivo vacío.

### 1.4 Base de Datos SQLite
- ✅ `Database : QObject` — inicialización via `QSqlDatabase::addDatabase("QSQLITE")`.
- ✅ Ruta: `QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/omado.db"`.
- ✅ Activar WAL mode: `PRAGMA journal_mode=WAL`.
- ✅ Implementar sistema de migraciones con tabla `schema_version`.
- ✅ Crear schema completo (task_lists, tasks, task_steps) según SPECS §4.2.
- ✅ Insertar lista por defecto "Tareas" al crear la DB por primera vez.
- ✅ Tests en `tst_Database.cpp`: creación, migración incremental, integridad referencial.

### 1.5 LocalRepository — CRUD Completo
- ✅ `LocalRepository : QObject` — única clase con acceso a `QSqlQuery`.
- ✅ `fetchLists()`, `createList()`, `updateList()`, `deleteList()`.
- ✅ `fetchTasks(listId)`, `createTask()`, `updateTask()`, `deleteTask()`.
- ✅ `fetchTasksForToday()` — tareas con `is_my_day=1` o `due_date=hoy`.
- ✅ `getPendingCount(listId)` y `getTotalPendingCount()`.
- ✅ `fetchSteps(taskId)`, `createStep()`, `updateStep()`, `deleteStep()`.
- ✅ Métodos asíncronos via `QtConcurrent::run` → `QFutureWatcher` en el caller.
- ✅ Tests en `tst_LocalRepository.cpp` con DB en memoria (`:memory:`).

### 1.6 Modelos QML
- ✅ `TaskListModel : QAbstractListModel` — roles: Id, DisplayName, SortOrder, PendingCount.
- ✅ `TaskModel : QAbstractListModel` — roles: Id, Title, IsCompleted, IsMyDay, DueDate, Importance, ReminderAt, HasSteps.
- ✅ `QSortFilterProxyModel` para filtrado por `isCompleted`.
- ✅ Inserción optimista (modelo local primero, SQLite asíncrono después).
- ✅ Tests: `fromSql()`, roles, rowCount, toggle, filtrado.

### 1.7 Tipografía y Assets
- ✅ Copiar `iAWriterMono-Regular.ttf` y `iAWriterMono-Bold.ttf` a `fonts/`.
- ✅ `QFontDatabase::addApplicationFont(":/fonts/iAWriterMono-Regular.ttf")` en `main.cpp`.
- ✅ `ThemedText.qml`: `font.family: "iA Writer Mono"`, `font.pixelSize: 13`.
- ✅ Escalar con `text-scaling-factor` de GNOME.

### 1.8 UI Local Completa — Layout del Wireframe

**SidePanel (Zona A):**
- ✅ `SpecialListSection.qml`: sección superior con My Day, Schedule, Tasks hardcodeados (no editables, no eliminables). Highlight con `Theme.accent` en la vista activa.
- ✅ `SectionSeparator.qml`: línea horizontal 1px `Theme.border`, padding vertical 8px — divide las dos secciones del SidePanel.
- ✅ `UserListSection.qml`: sección inferior con listas del usuario ordenadas por `sort_order`, más entrada `+ New List` al final (inline, activa con Enter sobre el ítem).
- ✅ Badge de conteo de tareas pendientes en cada ítem de lista (número a la derecha, `Theme.foreground` opacidad 0.4). Oculto si el conteo es 0.
- ✅ Footer del SidePanel: texto "Log in on MS To Do" pegado al borde inferior izquierdo. Tres estados (SPECS §11.6): no autenticado (texto clickeable `Theme.accent`) → conectando (spinner + "Connecting...") → autenticado (punto verde + email del usuario).

**TaskView (Zona B):**
- ✅ `TaskView.qml`: `ListView` con `TaskDelegate.qml`. Vacío al inicio con texto placeholder "Seleccioná una lista".
- ✅ `TaskDelegate.qml`: `CheckCircle.qml` (círculo 18px, NO cuadrado — ver SPECS §11.4) + título + `ImportanceDot` a la derecha.
- ✅ `CheckCircle.qml`: círculo con `border.color: Theme.border` vacío → `color: Theme.accent` con tilde ✓ al completar. Animación `Behavior on color` suave.
- ✅ `ScheduleView.qml`: vista especial para Schedule — `ListView` con `section.property: "dueDateSection"`, headers de fecha como separadores no interactivos (texto `Theme.foreground` opacidad 0.5). Usa `ScheduleDelegate.qml`.
- ✅ `TaskDetail.qml`: slide-in overlay desde la derecha con notas (TextArea), checklist de steps, due date picker, importancia, recurrencia. Activado con `→` o `F2`.
- ✅ `ImportanceDot.qml`: círculo 6px (bajo=gris, normal=invisible, alto=`Theme.accent`).
- ✅ `FocusRect.qml`: rectángulo 2px `Theme.accent`, `visible: parent.activeFocus`.

**InputBar (Zona C — bottom del TaskView):**
- ✅ `InputBar.qml`: barra fija de 44px en la base del TaskView. Fondo `Theme.surface`.
- ✅ `CheckCircle` (○) a la izquierda: al clic activa el `TextInput` de nueva tarea.
- ✅ `TextInput` placeholder "Nueva tarea..." — al presionar Enter: crea tarea + limpia campo.
- ✅ Cuatro iconos a la derecha alineados con spacing 12px (para configurar la tarea antes de crearla):
  - 📅 (Fecha límite): Abre popup/calendario para setear due_date.
  - ⏰ (Recordatorio): Abre popup para setear reminder_at.
  - 🔄 (Repetición): Abre popup para setear recurrence (diario, semanal, etc).
  - 📝 (Subtareas): Abre un inline-list o expande el panel derecho para agregar steps a la tarea en borrador.
- ✅ Tecla `N` desde cualquier lugar del TaskView: foco directo al TextInput del InputBar.
- ✅ Implementar todos los atajos de teclado de SPECS §11.5.

---

## Hito 2 — Notificaciones y Reminders Nativos
> **Objetivo:** El daemon de OmaDo envía notificaciones de sistema nativas para reminders de tareas.
> **Criterio de éxito:** Una tarea con `reminder_at = ahora + 1 min` dispara una notificación
> via D-Bus sin depender de procesos externos.

### 2.1 NotificationService
- ✅ `NotificationService : QObject` — usa `QDBusInterface` a `org.freedesktop.Notifications`.
- ✅ Método `send(title, body, iconName, timeoutMs)`.
- ✅ Sin `QProcess`. Sin llamadas a `omarchy-notification-send`.
- ✅ Tests: mockear la interfaz D-Bus y verificar que se llama `Notify` con los parámetros correctos.

### 2.2 Monitor de Reminders
- ✅ `QTimer` de 60 segundos en `LocalRepository` o servicio dedicado.
- ✅ En cada tick: `fetchTasks` donde `reminder_at <= now AND is_completed = 0 AND reminded = 0`.
- ✅ Para cada tarea encontrada: `NotificationService::send(...)` y marcar `reminded=1` en DB.
- ✅ Agregar columna `reminded INTEGER DEFAULT 0` al schema (migración v2).
- ✅ Al crear/editar un reminder: resetear `reminded=0`.

### 2.3 DaemonService — Modo --daemon
- ✅ Parsing de `--daemon` en `main.cpp` → `QCoreApplication` sin QML.
- ✅ `DaemonService : QObject` con `Q_CLASSINFO("D-Bus Interface", "io.omarchy.OmaDo")`.
- ✅ `QDBusConnection::sessionBus().registerService("io.omarchy.OmaDo")`.
- ✅ Implementar todos los métodos de SPECS §11.2 como `Q_SCRIPTABLE` slots.
- ✅ Emitir `TasksChanged(listId)` y `TodayTasksChanged()` tras mutaciones.
- ✅ Generar `io.omarchy.OmaDo.xml` con `qdbuscpp2xml`.

### 2.4 Autostart del Daemon
- ✅ Archivo `.desktop` en `~/.config/autostart/omado-daemon.desktop`.
- ✅ `omado.service` (systemd user unit) como alternativa.
- ✅ Documentar en README.

---

## Hito 3 — Sincronización Opcional con Microsoft To Do
> **Objetivo:** El usuario puede conectar su cuenta Microsoft. Las tareas locales se sincronizan
> bidireccionalmente con MS To Do. La app sigue funcionando offline sin internet.
> **Criterio de éxito:** Crear tarea offline → se sincroniza al reconectar. Modificar en MS To Do web
> → aparece en OmaDo en el próximo ciclo de sync.

### 3.1 KeychainStore
- ✅ `KeychainStore : QObject` — wrapper sobre `qtkeychain`.
- ✅ `writeKey(key, value)`, `readKey(key, callback)`, `deleteKey(key)` — todos asíncronos.
- ✅ Error `QKeychain::EntryNotFound` → silencioso (primera vez).
- ✅ `find_package(Qt6Keychain REQUIRED)` activo en CMakeLists.

### 3.2 AuthManager — Flujo PKCE
- ✅ `generateCodeVerifier()`: 32 bytes via `QRandomGenerator` → Base64URL sin padding.
- ✅ `generateCodeChallenge(verifier)`: SHA256 via `QCryptographicHash` → Base64URL sin padding.
- ✅ URL de autorización con scope `Tasks.ReadWrite User.Read offline_access`.
- ✅ `QDesktopServices::openUrl(authUrl)`.
- ✅ `QTcpServer` efímero en `localhost:8080` (o dinámico) → extraer `code` → destruir.
- ✅ POST `/token` via QNAM → parsear → `KeychainStore::writeKey`.
- ✅ `refreshAccessToken()` con `grant_type=refresh_token`.
- ✅ `ensureValidToken()` en `GraphClient`: verificar expiración antes de cada request.

### 3.3 GraphClient y Sync Engine
- ✅ `GraphClient` implementa todos los endpoints de SPECS §6 (listas, tareas, steps/checklistItems).
- ✅ `SyncEngine : QObject` — coordina sync bidireccional:
  - Local → Remote: tareas y listas con `remote_id IS NULL` se crean en Graph API.
  - Remote → Local: tareas y listas nuevas de Graph API se upsert en SQLite.
  - Mapeo de listas (incluyendo lista por defecto).
- ✅ Sync automático cada 5 minutos via `QTimer` (en GUI y daemon).
- ✅ Sync manual via `Ctrl+R` y botón en SidePanel.

### 3.4 UI de Autenticación en SidePanel
- ✅ Footer reactivo en `SidePanel.qml`:
  - No autenticado: Botón "Log in on MS To Do".
  - Autenticando: Indicador "Connecting...".
  - Autenticado: Indicador verde + email del usuario + botón de Sync + menú con opción "Log out".

### 3.5 Tests de Autenticación y Sync
- ✅ Tests unitarios en `tst_AuthManager.cpp` (validación PKCE, Client ID, estado y logout).
- ✅ Tests unitarios en `tst_GraphClient.cpp` (serialización JSON para Microsoft Graph API y persistencia en DB).

---

## Hito 4 — v1.1: Plugin de Panel Quickshell
> **Objetivo:** Plugin QML nativo para el panel de Omarchy que consume el D-Bus de OmaDo.
> **Criterio de éxito:** El panel muestra badge con conteo de tareas del día. Clic abre popup
> con lista de tareas marcables. Se actualiza reactivamente cuando el daemon emite `TodayTasksChanged`.
> **Nota:** El plugin vive en su propio repositorio (omacom-io/omado-panel o similar).

### 4.1 Plugin QML
- 🔲 Crear repositorio separado `omado-panel`.
- 🔲 `manifest.json` siguiendo estándar de plugins de Omarchy Quattro.
- 🔲 Instalar en `~/.config/omarchy/plugins/omado/`.
- 🔲 `QDBusInterface` a `io.omarchy.OmaDo` para `GetTotalPendingCount()` y `GetTasksForToday()`.
- 🔲 Badge en panel: número de tareas pendientes del día.
- 🔲 Popup al clic: `ListView` con tareas de hoy, checkbox funcional via `ToggleTask()`.
- 🔲 Escuchar señal D-Bus `TodayTasksChanged` para refresh reactivo.
- 🔲 Theming idéntico a OmaDo (mismas variables de `colors.toml`).

### 4.2 PKGBUILD Final
- 🔲 `pkgbuild/PKGBUILD` con dependencias: `qt6-base`, `qtkeychain-qt6`.
- 🔲 Instalar `.desktop` para autostart del daemon.
- 🔲 `provides=('omado')`, `conflicts=('omado-git')`.

---

## Tabla de Versiones

| Versión | Hitos completados | Estado |
|---|---|---|
| v0.1.0-alpha | 1 (offline completo) | ✅ |
| v0.2.0-alpha | 2 (notificaciones + daemon D-Bus) | ✅ |
| v1.0.0 | 3 (sync MS To Do opcional) | ✅ |
| v1.1.0 | 4 (plugin de panel Quickshell) | 🔲 |
