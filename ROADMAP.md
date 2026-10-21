# ROADMAP.md — Hitos de Desarrollo de OmaDo

> **v1.0:** Gestor de tareas local completo (offline-first) + Sincronización opcional con MS To Do
> **v1.1:** Plugin de panel Quickshell (`omado.panel`)
> **v1.2:** Parser de Lenguaje Natural bilingüe + Filtros globales
> **v1.3:** Personalización de listas (Emojis/Renombrado), Ordenamiento cronológico y Auditoría QA
> **v2.0:** Integración profunda con **Omarchy 4.5** (Notificaciones interactivas Omapager, Mi Día inteligente con reprogramación de vencidas, NLP 2.0 + D-Bus, Multilenguaje en vivo, Dock flotante, Quick-Add y CLI)
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
> **Nota:** El plugin vive en su propio repositorio (`omado-plugin` / `omado.panel`).

### 4.1 Plugin QML
- ✅ Crear repositorio separado `omado-plugin`.
- ✅ `manifest.json` siguiendo estándar de plugins de Omarchy Quattro.
- ✅ Instalar en `~/.config/omarchy/plugins/omado.panel/`.
- ✅ Comunicación D-Bus con `io.omarchy.OmaDo` para `GetTotalPendingCount()` y `GetTasksForToday()`.
- ✅ Badge en panel: número de tareas pendientes del día.
- ✅ Popup al clic: `ListView` con tareas de hoy, checkbox funcional via `ToggleTask()`.
- ✅ Escuchar señal D-Bus `TodayTasksChanged` para refresh reactivo.
- ✅ Theming idéntico a OmaDo (mismas variables de `colors.toml`).

### 4.2 PKGBUILD Final
- ✅ `PKGBUILD` con dependencias: `qt6-base`, `qt6-declarative`, `qt6-svg`, `qtkeychain-qt6`.
- ✅ Instalar `.desktop` para autostart del daemon.
- ✅ `provides=('omado')`, `conflicts=('omado')`.
- ✅ `install.sh` y `PKGBUILD` configurados para versión 1.3.

---

## Hito 5 — v1.2: Reconocimiento de Lenguaje Natural y Mejoras de Vista
- ✅ Parser en C++ de fechas y horas en lenguaje natural (bilingüe español/inglés).
- ✅ Limpieza automática del título de la tarea extrayendo las expresiones temporales.
- ✅ Badges de vista previa en tiempo real en la barra de creación (`InputBar.qml`).
- ✅ Inclusión automática de tareas con recordatorio para el día de hoy en la vista *My Day*.
- ✅ Botón conmutador para ocultar/mostrar tareas completadas globalmente con persistencia en `QSettings`.

---

## Hito 6 — v1.3: Personalización de Listas de Tareas, Ordenamiento y Auditoría QA
- ✅ Unificación del icono por defecto de lista y el emoji/icono personalizado en un solo slot visual en el panel lateral.
- ✅ Menú contextual nativo de Omarchy (`ListContextMenu.qml`) al hacer click derecho sobre cualquier lista personalizada (Cambiar icono, Renombrar lista, Eliminar lista).
- ✅ Selector visual de iconos/emojis (`ListIconPickerPopup.qml`) con estilo Omarchy y categorías compatibles con Microsoft To Do (General, Trabajo, Hogar, Ocio, Símbolos).
- ✅ Opción para restablecer el icono al default (3 rayas) o escribir/pegar cualquier emoji Unicode personalizado.
- ✅ Renombrado de listas directamente en línea o vía diálogo con preservación de icono y sincronización bidireccional vía Microsoft Graph API.
- ✅ Integración y visualización del icono de la lista activa en el encabezado principal de `TaskView.qml`.
- ✅ Botón conmutador minimalista para ordenar tareas cronológicamente por fecha de vencimiento y recordatorio más próximo con persistencia en `QSettings`.
- ✅ Auditoría integral de QA & C++/Qt6: mitigación de fugas y bloqueos en SQLite (`QThreadPool` dedicado, `QThreadStorage`, RAII, `PRAGMA cache_size=-512`), inicialización lazy y compartida de `QNetworkAccessManager`, sanitización XSS, limpieza de tokens en memoria en logout, índices v4 en SQLite y eliminación de consulta $N+1$ de subtareas.
- ✅ Tests unitarios completos para parseo, roles del modelo, ordenamiento cronológico, renombrado y actualización de iconos en `tst_TaskListModel.cpp` y `tst_TaskModel.cpp`.

---

## Hito 7 — v2.0 (Parte 1): Motor de Notificaciones Interactivo de Omarchy 4.5 (Omapager)
> **Objetivo:** Aprovechar al 100% el nuevo motor nativo `omarchy.notifications` (Omapager) de Omarchy 4.5 con botones de acción, niveles de urgencia, enlaces clicables y agrupación por aplicación.
> **Criterio de éxito:** Un recordatorio disparado por `omado --daemon` muestra el icono de OmaDo, botones de acción *"Completar"* y *"Posponer 15 min"* funcionales sin abrir la ventana, resumen de subtareas y links clicables.

### 7.1 `NotificationService` Extendido para Omarchy 4.5
- 🔲 Extender `NotificationService` con `NotificationOptions` (`iconName = "omado"`, `replacesId`, `urgency`, `transient`, `actions`) y método `sendNotification(title, body, options)` que retorne el `uint notificationId` asignado por `org.freedesktop.Notifications`.
- 🔲 Incluir siempre `hints["desktop-entry"] = "omado"`, `hints["urgency"]` (`1` Normal o `2` Critical) y `hints["transient"]` (`false` para recordatorios persistentes, `true` para avisos efímeros).
- 🔲 Conectar señales D-Bus `ActionInvoked(uint id, QString actionKey)` y `NotificationClosed(uint id, uint reason)` de `org.freedesktop.Notifications` hacia señales propias de `NotificationService`.

### 7.2 `ReminderService` Interactivo y Cuerpo Enriquecido
- 🔲 Mantener mapa `QHash<uint, QString> m_activeNotifications` (`notificationId -> taskId`) en `ReminderService`.
- 🔲 Enviar acciones interactivas `["complete", tr("Complete"), "snooze-15", tr("Snooze 15 min")]` en cada recordatorio.
- 🔲 Al recibir `actionInvoked(id, "complete")`: marcar tarea completada en `LocalRepository`, disparar sync si está autenticado y emitir `TasksChanged` / `TodayTasksChanged`.
- 🔲 Al recibir `actionInvoked(id, "snooze-15")`: invocar `LocalRepository::snoozeTaskReminder(taskId, 15)` (`reminder_at = now + 15m`, `reminded = 0`) y emitir señales D-Bus.
- 🔲 Mapear `task.importance == "high"` a `urgency = 2` (`Critical`) en las opciones de notificación.
- 🔲 Enriquecer el `body` de la notificación con el progreso de subtareas (`"📋 X/Y pasos"`) y las notas (`task.body`), preservando URLs (`https://...`) para que `Detect.js` y `Markup.js` de Omarchy 4.5 habiliten links clicables y el botón nativo *"Open Link"* / *"Join"*.

---

## Hito 8 — v2.0 (Parte 2): "Mi Día" Inteligente (Reprogramación de Vencidas) y NLP 2.0 + D-Bus
> **Objetivo:** Incorporar sugerencias inteligentes de tareas vencidas para "Mi Día" con reprogramación automática de fecha/hora, expandir el parser de lenguaje natural en C++ y exponerlo por D-Bus para `omado.panel` y CLI.
> **Criterio de éxito:** Las tareas vencidas se sugieren en *My Day* y al aceptarlas actualizan su `due_date` a hoy y su `reminder_at` a hoy conservando la hora original (o `+1h` si ya pasó). `CreateTaskFromNaturalText` permite crear tareas con fecha, hora, `!` e `#Lista` vía D-Bus.

### 8.1 Sugerencias de Tareas Vencidas y Reprogramación para "Mi Día"
- 🔲 Implementar `LocalRepository::fetchOverdueTasks()` (tareas incompletas con `due_date < hoy` o `reminder_at < hoy` no reprogramadas).
- 🔲 Implementar `LocalRepository::rescheduleTaskForMyDay(taskId, now)`:
  - Setea `is_my_day = 1` y `due_date = QDate::currentDate()`.
  - Si `reminder_at` estaba en el pasado: conserva `reminder_at.time()` original en la fecha de hoy (o `now + 1 hora` si esa hora del día ya pasó) y resetea `reminded = 0`.
- 🔲 Exponer `overdueSuggestions`, `overdueCount`, `rescheduleForMyDay(taskId)` y `rescheduleAllOverdueForMyDay()` en `TaskModel`.
- 🔲 Crear `MyDaySuggestionsPopup.qml` e integrar banner/botón de sugerencias en el encabezado de `TaskView.qml` cuando la vista activa es *My Day*.

### 8.2 Parser de Lenguaje Natural Extendido (`TaskInputParser` 2.0) y Links en Notas
- 🔲 Extender `TaskParseResult` y `TaskInputParser::parse` para detectar y limpiar del título:
  - Prioridad alta: `!` o `!!` → `importance = "high"`.
  - Lista destino: `#NombreLista` o `#"Nombre Lista"` → `targetListName`.
  - Recurrencia: `diario`/`daily`/`cada día`/`every day`, `semanal`/`weekly`, `mensual`/`monthly`, `anual`/`yearly` → `recurrence`.
- 🔲 Mostrar chips visuales en vivo en `InputBar.qml` para importancia (`!`), lista destino (`#`) y recurrencia detectados por NLP.
- 🔲 Detectar URLs (`https?://...`) en las notas de `TaskDetail.qml` y mostrar chips clicables que abran el navegador vía `QDesktopServices::openUrl`.

### 8.3 Expansión de `DaemonService` (`io.omarchy.OmaDo`) y Reactividad Bidireccional
- 🔲 Añadir métodos D-Bus en `DaemonService` y actualizar `io.omarchy.OmaDo.xml`:
  - `CreateTaskFromNaturalText(QString rawInput, bool addToMyDay) -> QString (JSON)`
  - `PreviewNaturalText(QString rawInput) -> QString (JSON)`
  - `GetOverdueTasks() -> QString (JSON)`
  - `RescheduleTaskForMyDay(QString taskId) -> bool`
  - `SnoozeTask(QString taskId, int minutes) -> bool`
- 🔲 Conectar la instancia GUI de OmaDo a las señales D-Bus `TasksChanged` y `TodayTasksChanged` del demonio (y emitir notificación al demonio al mutar desde GUI) para sincronización en tiempo real entre ventana, notificaciones, `omado.panel` y CLI.

---

## Hito 9 — v2.0 (Parte 3): Multilenguaje en Vivo 4.5, Dock Flotante, Quick-Add y CLI
> **Objetivo:** Integrar OmaDo con el selector de 50 idiomas de Omarchy 4.5 (`omarchy language`), el nuevo modo de Workspaces Flotantes + Dock (`Super + L` / `Super + M`), una ventana de captura rápida (`--quick-add`) y subcomandos CLI.
> **Criterio de éxito:** Cambiar el idioma en Omarchy traduce OmaDo en vivo sin reiniciar; OmaDo se fija y minimiza correctamente en `omarchy.dock`; `omado --quick-add` y `omado add/list/done` funcionan instantáneamente.

### 9.1 Soporte Multilenguaje en Vivo (`Translator` — Omarchy 4.5)
- 🔲 Implementar `Translator : public QTranslator` (`src/core/Translator.h/.cpp`) que observe `~/.config/omarchy/language` con `QFileSystemWatcher` y cargue catálogos JSON embebidos (`:/qt/qml/OmaDo/i18n/<code>.json`) con soporte de plurales CLDR.
- 🔲 Conectar `Translator::changed` a `QQmlEngine::retranslate()` en `main.cpp`.
- 🔲 Crear catálogos completos `i18n/en.json` y `i18n/es.json` (y estructura lista para los idiomas de Omarchy 4.5) y unificar todos los strings de C++ y QML bajo `tr()` / `qsTr()`.

### 9.2 Workspaces Flotantes (`hyprbars`), `omarchy.dock` y Modo `omado --quick-add`
- 🔲 Configurar `QGuiApplication::setDesktopFileName(QStringLiteral("omado"))` y `setWindowIcon(...)` en `main.cpp` para que `omarchy.dock` y el plugin nativo `/usr/lib/omarchy-hyprland-titlebars/titlebars.so` (`hyprbars`) reconozcan la ventana, muestren su icono en el Dock y permitan minimizarla con el botón `−` de la barra de título (o `Super + M`), maximizarla con `□` (o doble clic) y cerrarla con `×`.
- 🔲 Adaptar `Main.qml` para el modo ventana/flotante de Omarchy 4.5: quitar `Qt.FramelessWindowHint` y el `radius: 8` interno del contenedor raíz (para que encaje al píxel debajo de la barra de título de 30px de `hyprbars` sin esquinas huecas ni doble borde) y definir `minimumWidth: 640` / `minimumHeight: 460` para redimensionado limpio con el mouse.
- 🔲 Implementar flag `--quick-add` en `main.cpp` y componente `QuickAddWindow.qml` (barra flotante centrada con bordes del tema, chips NLP en vivo y creación rápida hacia *My Day*).
- 🔲 Añadir navegación por teclado estilo Vim/Omarchy (`j`/`k`, `Ctrl+1..3`, `?` para abrir `ShortcutHelpOverlay.qml`).

### 9.3 Subcomandos CLI (`CliRunner`)
- 🔲 Implementar `CliRunner` (`src/core/CliRunner.h/.cpp`) soportando:
  - `omado add "<texto NLP>" [--my-day]`
  - `omado list [--today | --overdue | --json]`
  - `omado done <taskId>`
  - `omado reschedule <taskId>`
  - `omado sync`

---

## Hito 10 — v2.0 (Parte 4): Suite de Tests Unitarios 2.0 y Empaquetado Multi-Arquitectura (`x86_64` + `aarch64`)
> **Objetivo:** Garantizar cobertura total de tests unitarios para todas las features e integraciones de OmaDo 2.0 y preparar el lanzamiento multi-arquitectura junto a Omarchy 4.5 estable.
> **Criterio de éxito:** 100% de tests pasando en `ctest` con aislamiento de datos (`QStandardPaths::setTestModeEnabled(true)`) y paquete compilable en `x86_64` y `aarch64`.

### 10.1 Actualización y Nuevos Tests Unitarios
- 🔲 Aislar todas las suites existentes con `QStandardPaths::setTestModeEnabled(true)` para no tocar la DB de usuario.
- 🔲 Actualizar `tst_NotificationService.cpp`: verificar envío de `actions`, `hints` (`desktop-entry`, `urgency`, `transient`), `replacesId`, retorno de `notificationId` y recepción de señal `actionInvoked` contra un servicio D-Bus mock/loopback.
- 🔲 Actualizar `tst_ReminderService.cpp`: testear el mapa `notificationId -> taskId`, acciones `"complete"` y `"snooze-15"`, urgencia crítica en tareas de alta importancia y formato enriquecido del cuerpo (subtareas + links).
- 🔲 Actualizar `tst_LocalRepository.cpp` y `tst_TaskModel.cpp`: testear `fetchOverdueTasks()`, `rescheduleTaskForMyDay()` (preservación de hora futura vs ajuste `+1h` en hora pasada y reset de `reminded=0`) y `snoozeTaskReminder()`.
- 🔲 Actualizar `tst_TaskInputParser.cpp`: testear extracción combinada de fechas, horas, prioridad (`!`/`!!`), listas (`#Lista`) y recurrencia en español e inglés.
- 🔲 Actualizar `tst_DaemonService.cpp`: testear `CreateTaskFromNaturalText`, `PreviewNaturalText`, `GetOverdueTasks`, `RescheduleTaskForMyDay` y `SnoozeTask`.
- 🔲 Crear `tst_Translator.cpp`: testear carga de catálogos JSON, cambio en caliente del archivo `language`, cadena de fallback de idioma y reglas de plurales.
- 🔲 Crear `tst_CliRunner.cpp`: testear parseo de argumentos CLI (`add`, `list`, `done`, `reschedule`, `sync`) y salida JSON/texto.

### 10.2 Empaquetado v2.0.0 (`x86_64` + `aarch64`)
- 🔲 Actualizar versión a `2.0.0` en `CMakeLists.txt`, `main.cpp`, `PKGBUILD` e `install.sh`.
- 🔲 Agregar soporte oficial ARM64 en `PKGBUILD`: `arch=('x86_64' 'aarch64')`.
- 🔲 Actualizar `omado.desktop` con traducciones de `Name` y `Comment` y soporte de acción `QuickAdd`.

---

## Tabla de Versiones

| Versión | Hitos completados | Estado |
|---|---|---|
| v0.1.0-alpha | 1 (offline completo) | ✅ |
| v0.2.0-alpha | 2 (notificaciones + daemon D-Bus) | ✅ |
| v1.0.0 | 3 (sync MS To Do opcional) | ✅ |
| v1.1.0 | 4 (plugin de panel Quickshell) | ✅ |
| v1.2.0 | 5 (parser NLP, filtro completadas, recordatorios en My Day) | ✅ |
| v1.3.0 | 6 (personalización de listas, ordenamiento por vencimiento, QA & optimización RAM) | ✅ |
| v2.0.0 | 7, 8, 9 y 10 (Integración Omarchy 4.5: Omapager interactivo, Mi Día inteligente, NLP 2.0, i18n en vivo, Dock/Quick-Add, CLI y soporte aarch64) | 🔲 |
