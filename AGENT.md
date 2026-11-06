# AGENT.md — Reglas del Sistema para el Agente Desarrollador de OmaDo
> **Fase de aplicación:** Hito 1 en adelante (toda implementación de código fuente)
> **Nivel de cumplimiento:** OBLIGATORIO. Sin excepciones.
> **Audience:** Agente de IA (Antigravity / Gemini) que desarrollará el código del proyecto.

---

## PREÁMBULO

OmaDo es un gestor de tareas nativo para Omarchy (Quattro / 4.5+) — local-first, con sincronización
con MS To Do / Samsung Reminders opcional. Mismo ecosistema y filosofía que OmaWrite, OmaCalc, Omacut y Hype.

El agente DEBE leer `SPECS.md` completo antes de escribir cualquier línea de código.

Filosofía: **cero peso, máxima integración nativa con Omarchy 4.5**. Cada decisión se evalúa contra este criterio.

---

## REGLA 1 — Dependencias: Lista Blanca Exhaustiva y Cerrada

Solo se permite lo que figura aquí. Todo lo demás está prohibido por defecto.

### 1.1 Lista Blanca

**Qt 6 via `find_package(Qt6 ...)`:**
- `Qt6::Core` — QObject, signals/slots, QTimer, QFile, QFileSystemWatcher, QRegularExpression, QJsonDocument, QUuid, QDateTime, QTranslator, QLocale, QCommandLineParser
- `Qt6::Gui` — QGuiApplication, QFontDatabase, QDesktopServices, QColor, QIcon
- `Qt6::Quick` — QML engine, QQuickView, QAbstractListModel
- `Qt6::QuickControls2` — Controls básicos en QML
- `Qt6::Network` — QNetworkAccessManager, QTcpServer, QNetworkRequest
- `Qt6::Sql` — QSqlDatabase, QSqlQuery, QSqlError (SQLite driver incluido en qt6-base)
- `Qt6::DBus` — QDBusConnection, QDBusInterface, QDBusMessage, QDBusContext
- `Qt6::Concurrent` — QtConcurrent::run para operaciones DB asíncronas
- `Qt6::Test` — SOLO en target de tests, nunca en el binario principal

**Única dependencia externa permitida:**
- `Qt6Keychain` via `find_package(Qt6Keychain REQUIRED)` — solo para tokens OAuth2

**Build:**
- CMake 3.21+ exclusivamente (`arch=('x86_64' 'aarch64')` en `PKGBUILD`).

### 1.2 Lista Negra (Prohibiciones Explícitas)

| Categoría | Prohibido | Alternativa correcta |
|---|---|---|
| Web | Electron, Tauri, WebEngine, WebView | Qt Quick nativo |
| KDE | KIO, KWallet, Solid, KConfig | Qt6::DBus, Qt6Keychain |
| GNOME directo | libadwaita, GTK, GLib, Gio | — (no necesario) |
| JSON externos | nlohmann/json, rapidjson, jsoncpp | QJsonDocument (Qt6::Core) |
| TOML externos | toml11, cpptoml | Parser manual con QRegularExpression |
| HTTP externos | libcurl, cpp-httplib, Boost.Beast | QNetworkAccessManager |
| Crypto externos | OpenSSL directo, libsodium, Botan | QCryptographicHash, QRandomGenerator |
| Secret directo | libsecret.h directo | qtkeychain ÚNICAMENTE |
| ORM / SQL | sqlpp11, SOCI, Qt ActiveRecord | QSqlQuery directa en LocalRepository |
| Package managers C++ | Conan, vcpkg | CMake find_package nativo |

**Consecuencia:** Revertir inmediatamente y reimplementar con alternativa de la lista blanca.

---

## REGLA 2 — QProcess: PROHIBICIÓN TOTAL

```cpp
// NUNCA usar QProcess en OmaDo
QProcess::execute("omarchy-reminder", args);  // PROHIBIDO
QProcess proc; proc.start("notify-send");     // PROHIBIDO
system("omarchy-notification-send ...");       // PROHIBIDO
```

**Justificación:** OmaDo tiene D-Bus nativo para todo:
- **Notificaciones** → `QDBusInterface` a `org.freedesktop.Notifications` (SPECS §8).
- **IPC con Omarchy shell** → `QDBusInterface` a `io.omarchy.OmaDo`.
- **No hay ningún caso de uso que requiera QProcess.**

---

## REGLA 3 — Acceso a Base de Datos: Solo via LocalRepository

```cpp
// CORRECTO — toda SQL pasa por LocalRepository
auto future = m_repository->fetchTasks(listId);
QFutureWatcher<QList<Task>> *watcher = new QFutureWatcher<QList<Task>>(this);
connect(watcher, &QFutureWatcher<QList<Task>>::finished, this, [watcher, this]() {
    m_taskModel->setTasks(watcher->result());
    watcher->deleteLater();
});
watcher->setFuture(future);

// INCORRECTO — SQL fuera de LocalRepository
QSqlQuery q;
q.exec("SELECT * FROM tasks");  // PROHIBIDO en cualquier otra clase
```

**Reglas de SQLite:**
- WAL mode siempre activo.
- Nunca ejecutar SQL en el main thread — siempre via `QtConcurrent::run`.
- Usar `QSqlQuery::prepare()` + `bindValue()` siempre. Nunca concatenar SQL con strings del usuario.
- Transacciones explícitas para operaciones de múltiples pasos.

---

## REGLA 4 — Sistema de Build: CMake Target-Based

```cmake
# CORRECTO — propiedades aplicadas al target
target_include_directories(omado PRIVATE src/)
target_compile_options(omado PRIVATE -Wall -Wextra)
target_link_libraries(omado PRIVATE Qt6::Core Qt6::Sql)

# INCORRECTO — variables globales
include_directories(src/)       # PROHIBIDO
add_compile_options(-Wall)      # PROHIBIDO (a nivel global)
link_libraries(Qt6::Core)       # PROHIBIDO
```

**Estructura obligatoria del CMakeLists.txt raíz:**

```cmake
cmake_minimum_required(VERSION 3.21)
project(omado VERSION X.Y.Z LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Sql DBus Concurrent)
find_package(Qt6Keychain REQUIRED)

# Librería interna compartida entre app y tests
add_library(omado_lib STATIC ...)
target_link_libraries(omado_lib PUBLIC
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Network Qt6::Sql Qt6::DBus Qt6::Concurrent
    Qt6Keychain::Qt6Keychain
)

qt_add_executable(omado src/main.cpp)
qt_add_qml_module(omado URI "OmaDo" VERSION "1.0" ...)
target_link_libraries(omado PRIVATE omado_lib Qt6::QuickControls2)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

**Reglas adicionales:**
- NO `file(GLOB ...)` — listar cada archivo fuente explícitamente.
- NO `CMAKE_BUILD_TYPE` hardcodeado — el usuario lo pasa via CLI.
- Fuentes (fonts/) como recursos Qt via `qt_add_resources`, no archivos sueltos.

---

## REGLA 5 — Manejo de Errores de Red: Silencioso y No Bloqueante

Los errores de red **nunca** bloquean la UI ni interrumpen el flujo.

```cpp
// CORRECTO
void GraphClient::onNetworkReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[GraphClient]" << reply->errorString();
        emit networkError(reply->errorString());  // UI muestra StatusDot
        reply->deleteLater();
        return;
    }
    // parsear...
    reply->deleteLater();
}

// INCORRECTO
QMessageBox::critical(nullptr, "Error", reply->errorString()); // PROHIBIDO
qFatal("Network error");                                        // PROHIBIDO
```

| Error HTTP | Acción |
|---|---|
| 401 Unauthorized | Intentar refresh token; si falla: `emit reauthenticationRequired()` |
| 404 Not Found | `qWarning()` + limpiar ítem local |
| 429 Rate Limit | Esperar `Retry-After` header via `QTimer::singleShot()` |
| 5xx Server Error | `qWarning()` + `emit networkError(...)` |
| Timeout | Reintentar 1 vez, luego `emit networkError(...)` |

---

## REGLA 6 — Convenciones de Código C++

```cpp
// Clases: PascalCase
class LocalRepository : public QObject { ... };

// Métodos: camelCase
QFuture<QList<Task>> fetchTasks(const QString &listId);

// Miembros privados: m_ + camelCase
private:
    QSqlDatabase  m_db;
    QString       m_currentListId;
    QTimer       *m_reminderTimer;

// Constantes: kCamelCase o ALL_CAPS para #define
static constexpr int kReminderCheckInterval = 60'000; // 60s en ms

// Enum class (preferido sobre enum)
enum class SyncState { Idle, Syncing, Error, Disabled };
```

**Signals/Slots — SIEMPRE sintaxis de puntero a función:**
```cpp
connect(m_repository, &LocalRepository::tasksReady,
        this,         &TaskModel::onTasksReady);
// NUNCA: connect(m_repository, SIGNAL(tasksReady(...)), ...)
```

**Memoria:**
- Parent-ownership de Qt para todo `QObject`.
- `reply->deleteLater()` siempre al final del slot de red.
- `watcher->deleteLater()` siempre en el slot `finished` del `QFutureWatcher`.
- `job->deleteLater()` siempre en el slot `finished` de `QKeychain::Job`.

**Tipos preferidos:**
- `QString` sobre `std::string`
- `QList<T>` sobre `std::vector<T>`
- `QHash<K,V>` sobre `std::unordered_map<K,V>`
- `QUuid::createUuid().toString(QUuid::WithoutBraces)` para IDs locales

---

## REGLA 7 — Convenciones QML

**Estructura de archivo:**
```qml
// Un componente por archivo. PascalCase = nombre del archivo.
import QtQuick
import QtQuick.Controls
import OmaDo 1.0
import OmaDo.Theme 1.0

Item {
    id: root
    // 1. required properties
    // 2. optional properties con defaults
    // 3. señales
    // 4. layout (anchors, width, height)
    // 5. hijos
    // 6. handlers (Keys, TapHandler)
    // 7. animaciones (Behavior, Transition)
    // 8. states
}
```

**Theming — SIEMPRE via Theme.*:**
```qml
// CORRECTO
Rectangle { color: Theme.surface; border.color: Theme.border }
Text { color: Theme.foreground }

// INCORRECTO — colores hardcodeados PROHIBIDOS
Rectangle { color: "#1a1a2e" }
Text { color: "white" }
```

**Fuentes — SIEMPRE iA Writer Mono:**
```qml
// CORRECTO
Text { font.family: "iA Writer Mono"; font.pixelSize: 13 }

// INCORRECTO
Text { font.family: "monospace" } // PROHIBIDO
```

**Internacionalización (i18n — Omarchy 4.5):**
- Todo string visible en QML DEBE envolverse en `qsTr("...")` (con clave base en inglés) y en C++ en `QCoreApplication::translate(...)` o `tr("...")`.
- Usar placeholders `%1` con `.arg()` en lugar de concatenar fragmentos traducidos.
- `Translator` (`QTranslator` respaldado por catálogos JSON en `:/qt/qml/OmaDo/i18n/<code>.json`) observa `~/.config/omarchy/language` con `QFileSystemWatcher` y dispara `QQmlEngine::retranslate()` en caliente.

**Integración con Wayland, Floating Workspaces y Dock (Omarchy 4.5):**
- Siempre invocar `QGuiApplication::setDesktopFileName(QStringLiteral("omado"))` y `setWindowIcon(...)` en `main.cpp` para que el compositor Hyprland y `omarchy.dock` asocien la ventana a `omado.desktop` al fijar al dock o minimizar (`Super + M`).

**Foco:**
- TODO elemento interactivo: `activeFocusOnTab: true`.
- TODO elemento con foco: mostrar `FocusRect` (2px, `Theme.accent`).
- NUNCA ocultar el indicador de foco.

---

## REGLA 8 — Seguridad: Tokens y Credenciales

```cpp
// CORRECTO — solo qtkeychain
auto *job = new QKeychain::WritePasswordJob("omado", this);
job->setKey("ms_access_token");
job->setTextData(token);

// CORRECTO — logging sin exponer el token
qDebug() << "Token cargado, longitud:" << token.length();

// INCORRECTO — NUNCA loguear el token
qDebug() << "Token:" << accessToken;              // PROHIBIDO
qDebug() << "Token parcial:" << token.left(8);   // PROHIBIDO (ni parcial)
```

Los tokens NUNCA pueden ir a: `QSettings`, archivos planos, variables de entorno, argumentos CLI.

---

## REGLA 9 — Notificaciones: Solo via QDBus (Motor Omapager / Omarchy 4.5)

```cpp
// CORRECTO — D-Bus nativo con hints y acciones de Omarchy 4.5, sin QProcess
QDBusInterface iface("org.freedesktop.Notifications",
                     "/org/freedesktop/Notifications",
                     "org.freedesktop.Notifications",
                     QDBusConnection::sessionBus());

QStringList actions = {
    QStringLiteral("complete"), tr("Complete"),
    QStringLiteral("snooze-15"), tr("Snooze 15 min")
};
QVariantMap hints;
hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("omado"));
hints.insert(QStringLiteral("urgency"), QVariant::fromValue<uchar>(isHighImportance ? 2 : 1));
hints.insert(QStringLiteral("transient"), isEphemeral);

QDBusMessage reply = iface.call("Notify", "OmaDo", replacesId, "omado",
                                title, body, actions, hints, timeoutMs);

// INCORRECTO
QProcess::execute("notify-send", {title, body});          // PROHIBIDO
QProcess::execute("omarchy-notification-send", {title});  // PROHIBIDO
```

**Reglas del motor de notificaciones de Omarchy 4.5 (`omarchy.notifications`):**
- Siempre enviar `app_name = "OmaDo"`, `app_icon = "omado"` y `hints["desktop-entry"] = "omado"` para asegurar agrupación por remitente, resolución de icono y soporte de *Snooze* por app en el shell.
- Suscribirse en el demonio a las señales D-Bus `ActionInvoked(uint id, QString action_key)` y `NotificationClosed(uint id, uint reason)` mediante `QDBusConnection::connect(...)`.
- Enviar `hints["transient"] = true` en avisos efímeros (ej. estado de sync) para no contaminar el historial persistente (`history.json`) de Omarchy 4.5; dejar `transient = false` únicamente en recordatorios reales de tareas.
- Mapear `importance == "high"` a `urgency = 2` (Critical) y el resto a `urgency = 1` (Normal).

---

## REGLA 10 — Testing

**Cobertura mínima por hito:**
- **Hito 1:** `tst_ThemeReader` (5+ fixtures), `tst_Database` (schema + migraciones v1–v4+), `tst_LocalRepository` (CRUD completo).
- **Hito 2:** `tst_NotificationService` (mock D-Bus), `tst_ReminderService` (monitor y marcado en DB), `tst_DaemonService` (métodos y señales D-Bus).
- **Hito 3:** `tst_AuthManager` (PKCE generation + estado), `tst_GraphClient` (serialización JSON Graph API y sync).
- **Hito 5–6:** `tst_TaskInputParser` (fechas/horas bilingües ES/EN), `tst_TaskModel` (filtros, ordenamiento por vencimiento, subtareas), `tst_TaskListModel` (emojis, renombrado).
- **Hito 7 (Notificaciones 4.5):** `tst_NotificationService` (verificación de `actions`, `hints` [`desktop-entry`, `urgency`, `transient`], `replacesId`, captura de `uint notificationId` y emisión de `actionInvoked`), `tst_ReminderService` (mapeo `notificationId -> taskId`, ejecución de acciones `"complete"` y `"snooze-15"`, cuerpo enriquecido con conteo de subtareas y preservación de URLs).
- **Hito 8 (Mi Día Inteligente & NLP 2.0):** `tst_LocalRepository` (`fetchOverdueTasks`, `rescheduleTaskForMyDay` preservando hora original o fallback `+1h`, `snoozeTaskReminder`), `tst_TaskInputParser` (tokens `!`, `#Lista` y recurrencia), `tst_DaemonService` (`CreateTaskFromNaturalText`, `PreviewNaturalText`, `GetOverdueTasks`, `RescheduleTaskForMyDay`, `SnoozeTask`).
- **Hito 9 (Idiomas 4.5, Quick-Add y CLI):** `tst_Translator` (lectura y hot-reload de archivo `language` temporal, cadena de fallback `es-MX -> es -> en`, plurales CLDR), `tst_CliRunner` (parseo y ejecución de subcomandos `add`, `list`, `done`, `sync`).

**Estructura:**
```cmake
# tests/CMakeLists.txt — un ejecutable por suite registrado en CTest
find_package(Qt6 REQUIRED COMPONENTS Core Test Sql DBus)
foreach(test_name ${TEST_TARGETS})
    qt_add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE Qt6::Core Qt6::Test Qt6::Sql Qt6::DBus omado_lib)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
```

- Activar siempre `QStandardPaths::setTestModeEnabled(true)` en `initTestCase()` para aislar la base de datos SQLite de prueba de la base de datos real del usuario.
- Fixtures JSON y TOML en `tests/fixtures/`. Nunca requests de red reales en tests automáticos.

---

## REGLA 11 — Checklist Pre-Código (Obligatorio)

Antes de implementar cualquier feature, en este orden:

1. **Leer SPECS.md** — ¿la feature está especificada? Si no: detener y preguntar.
2. **Verificar ROADMAP.md** — ¿la tarea está en el hito activo?
3. **Comprobar dependencias** — ¿requiere algo? ¿está en la Lista Blanca de REGLA 1?
4. **Identificar el archivo** — ¿qué clase del árbol de SPECS §4?
5. **Escribir código** — seguir REGLAS 5, 6, 7, 9.
6. **Escribir/actualizar tests** — REGLA 10. Sin código sin tests.
7. **Verificar compilación y tests** — `cmake --build build && ctest --test-dir build --output-on-failure` sin warnings con `-Wall -Wextra`.
8. **Actualizar ROADMAP.md** — marcar tarea como ✅.

---

## REGLA 12 — Prohibiciones Estructurales

- NO bifurcar el árbol de fuentes sin actualizar SPECS.md §4.
- NO usar `QApplication` (solo `QGuiApplication` para GUI/Quick-Add o `QCoreApplication` para daemon/CLI).
- NO usar `QThread` sin justificación en el commit. Preferir `QtConcurrent::run` sobre el `QThreadPool` dedicado de `LocalRepository`.
- NO emitir signals desde constructores.
- NO usar `Q_OBJECT` en structs de datos (`Task`, `TaskList`, `TaskStep`).
- NO `#ifdef Q_OS_WIN` ni guards de plataforma no-Linux.
- NO concatenar strings del usuario en queries SQL — usar `bindValue()` siempre.
- NO SQL fuera de `LocalRepository`.

---

## Apéndice: Referencias Rápidas Qt

| Necesidad | Solución Qt |
|---|---|
| Parsear JSON | `QJsonDocument::fromJson(data)` |
| Hash SHA256 | `QCryptographicHash::hash(data, QCryptographicHash::Sha256)` |
| Bytes aleatorios | `QRandomGenerator::global()->generate64()` |
| Base64URL encode | `data.toBase64(QByteArray::Base64UrlEncoding \| QByteArray::OmitTrailingEquals)` |
| Abrir URL en browser | `QDesktopServices::openUrl(url)` |
| ID local único | `QUuid::createUuid().toString(QUuid::WithoutBraces)` |
| Leer archivo | `QFile f(path); f.open(QIODevice::ReadOnly); f.readAll()` |
| Watch archivo | `QFileSystemWatcher::addPath(path)` |
| Traducción dinámica 4.5 | `Translator : public QTranslator` observando `~/.config/omarchy/language` |
| Timer one-shot | `QTimer::singleShot(ms, this, &MyClass::slot)` |
| Fecha hoy (SQL) | `QDate::currentDate().toString(Qt::ISODate)` |
| Aislamiento DB (tests) | `QStandardPaths::setTestModeEnabled(true)` |
| Operación asíncrona DB | `QtConcurrent::run(&m_threadPool, [=]() { ... })` + `QFutureWatcher` |
| Notificación D-Bus | `QDBusInterface("org.freedesktop.Notifications", ...)` |
| Señales D-Bus externas | `QDBusConnection::sessionBus().connect("org.freedesktop.Notifications", ...)` |
| Servicio D-Bus propio | `QDBusConnection::sessionBus().registerService("io.omarchy.OmaDo")` |

---

## REGLA 13 — Diseño Visual: Fidelidad al Wireframe

Las siguientes restricciones de diseño derivan directamente del wireframe de referencia
(`wireframe.jpeg`) y son obligatorias. La estética sigue la línea de OmaCalc y OmaWrite.

### 13.1 Checkboxes: Siempre Círculos, Nunca Cuadrados

```qml
// CORRECTO — CheckCircle.qml
Rectangle {
    radius: width / 2  // círculo perfecto
    width: 18; height: 18
    color: isCompleted ? Theme.accent : "transparent"
    border.color: isCompleted ? Theme.accent : Theme.border
    border.width: 1.5
}

// INCORRECTO — cuadrado
CheckBox { ... }                          // PROHIBIDO (control de QtQuick.Controls)
Rectangle { radius: 0 ... }              // PROHIBIDO (cuadrado)
Rectangle { radius: 3 ... }              // PROHIBIDO (cuadrado redondeado)
```

### 13.2 Estructura del SidePanel: Dos Secciones Obligatorias

El SidePanel SIEMPRE tiene exactamente esta estructura vertical:

```
┌──────────────────────────┐
│  My Day                  │  ← Vistas especiales fijas
│  Schedule                │     (hardcodeadas, no editables)
│  Tasks                   │
│ ─────────────────────── │  ← SectionSeparator.qml (1px, Theme.border)
│  Lista custom 1          │  ← Listas del usuario (con soporte de emoji/icono)
│  Lista custom 2          │
│  + New List              │
│                          │
│ [Log in on MS To Do]     │  ← footer, pegado al bottom
└──────────────────────────┘
```

- Las 3 vistas especiales (My Day, Schedule, Tasks) son **fijas e inmutables** en el orden de arriba.
- No se puede mover, renombrar ni eliminar My Day, Schedule ni Tasks.
- El separador `SectionSeparator.qml` SIEMPRE está presente.
- El footer de cuenta/sincronización SIEMPRE está pegado al `bottom` del SidePanel.

### 13.3 InputBar: Zona C Obligatoria

La `InputBar.qml` SIEMPRE aparece anclada al bottom del `TaskView`. Nunca desaparece.
Los cuatro iconos de configuración rápida (calendario, recordatorio, repetición, subtareas) son SIEMPRE visibles en el InputBar — no se ocultan.

```qml
// CORRECTO — InputBar pegada al bottom
TaskView {
    InputBar {
        Layout.fillWidth: true
        height: 46
    }
}

// INCORRECTO — InputBar flotante o condicional
InputBar { visible: listSelected }  // PROHIBIDO — siempre visible
```

### 13.4 Iconos: SVG Monocromáticos Embebidos (`AppIcon.qml`) y Unicode

- Los iconos del sistema de la interfaz se implementan mediante el componente `AppIcon.qml` usando exclusivamente los vectores SVG monocromáticos embebidos en recursos (`icons/*.svg`) teñidos dinámicamente con `Theme.*`, o glifos Unicode de `iA Writer Mono`.
- Las listas personalizadas del usuario soportan un slot único que alterna entre el icono vectorial `list` por defecto y un emoji Unicode seleccionado en `ListIconPickerPopup.qml`.
- **Prohibido** cargar imágenes raster externas (PNG/JPG) para iconos de UI.

### 13.5 Sin Diálogos Nativos Bloqueantes

- **Prohibido** usar `QMessageBox` o diálogos del sistema operativo que bloqueen el event loop. Todos los menús y popups (`DatePickerPopup`, `TimePickerPopup`, `ListContextMenu`, `ListIconPickerPopup`, `ListRenameDialog`) se renderizan como componentes QML inline/overlay estilizados con `Theme.*`.
