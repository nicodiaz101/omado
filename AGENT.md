# AGENT.md — Reglas del Sistema para el Agente Desarrollador de OmaDo
> **Fase de aplicación:** Hito 1 en adelante (toda implementación de código fuente)
> **Nivel de cumplimiento:** OBLIGATORIO. Sin excepciones.
> **Audience:** Agente de IA (Antigravity / Gemini) que desarrollará el código del proyecto.

---

## PREÁMBULO

OmaDo es un gestor de tareas nativo para Omarchy Quattro — local-first, sincronización con
MS To Do opcional. Mismo ecosistema que OmaWrite y OmaCalc (omacom-io en GitHub).

El agente DEBE leer `SPECS.md` completo antes de escribir cualquier línea de código.

Filosofía: **cero peso, máxima integración nativa**. Cada decisión se evalúa contra este criterio.

---

## REGLA 1 — Dependencias: Lista Blanca Exhaustiva y Cerrada

Solo se permite lo que figura aquí. Todo lo demás está prohibido por defecto.

### 1.1 Lista Blanca

**Qt 6 via `find_package(Qt6 ...)`:**
- `Qt6::Core` — QObject, signals/slots, QTimer, QFile, QRegularExpression, QJsonDocument, QUuid, QDateTime
- `Qt6::Gui` — QGuiApplication, QFontDatabase, QDesktopServices, QColor
- `Qt6::Quick` — QML engine, QQuickView, QAbstractListModel
- `Qt6::QuickControls2` — Controls básicos en QML
- `Qt6::Network` — QNetworkAccessManager, QTcpServer, QNetworkRequest
- `Qt6::Sql` — QSqlDatabase, QSqlQuery, QSqlError (SQLite driver incluido en qt6-base)
- `Qt6::DBus` — QDBusConnection, QDBusInterface, QDBusMessage
- `Qt6::Concurrent` — QtConcurrent::run para operaciones DB asíncronas
- `Qt6::Test` — SOLO en target de tests, nunca en el binario principal

**Única dependencia externa permitida:**
- `Qt6Keychain` via `find_package(Qt6Keychain REQUIRED)` — solo para tokens OAuth2

**Build:**
- CMake 3.21+ exclusivamente.

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

## REGLA 9 — Notificaciones: Solo via QDBus

```cpp
// CORRECTO — D-Bus nativo, sin QProcess
QDBusInterface iface("org.freedesktop.Notifications",
                     "/org/freedesktop/Notifications",
                     "org.freedesktop.Notifications",
                     QDBusConnection::sessionBus());
iface.call("Notify", "OmaDo", 0u, "checkbox-checked",
           title, body, QStringList(), QVariantMap(), 5000);

// INCORRECTO
QProcess::execute("notify-send", {title, body});          // PROHIBIDO
QProcess::execute("omarchy-notification-send", {title});  // PROHIBIDO
```

---

## REGLA 10 — Testing

**Cobertura mínima por hito:**
- Hito 1: ThemeReader (5+ fixtures), Database (schema + migraciones), LocalRepository (CRUD completo).
- Hito 2: NotificationService (mock D-Bus), reminder monitor (QTimer mock).
- Hito 3: AuthManager (PKCE generation + server mock), SyncEngine (GraphClient mockeado).

**Estructura:**
```cmake
# tests/CMakeLists.txt
find_package(Qt6 REQUIRED COMPONENTS Test Sql)
qt_add_executable(omado_tests
    tst_ThemeReader.cpp
    tst_Database.cpp
    tst_LocalRepository.cpp
    ...
)
target_link_libraries(omado_tests PRIVATE Qt6::Test Qt6::Sql omado_lib)
add_test(NAME OmaDoTests COMMAND omado_tests)
```

Fixtures JSON y TOML en `tests/fixtures/`. Nunca requests reales en tests automáticos.
DB de tests siempre en `:memory:` (no en disco).

---

## REGLA 11 — Checklist Pre-Código (Obligatorio)

Antes de implementar cualquier feature, en este orden:

1. **Leer SPECS.md** — ¿la feature está especificada? Si no: detener y preguntar.
2. **Verificar ROADMAP.md** — ¿la tarea está en el hito activo?
3. **Comprobar dependencias** — ¿requiere algo? ¿está en la Lista Blanca de REGLA 1?
4. **Identificar el archivo** — ¿qué clase del árbol de SPECS §3?
5. **Escribir código** — seguir REGLAS 5, 6, 7.
6. **Escribir/actualizar tests** — REGLA 10. Sin código sin tests.
7. **Verificar compilación** — `cmake --build build` sin warnings con `-Wall -Wextra`.
8. **Actualizar ROADMAP.md** — marcar tarea como ✅.

---

## REGLA 12 — Prohibiciones Estructurales

- NO bifurcar el árbol de fuentes sin actualizar SPECS.md §3.
- NO usar `QApplication` (solo `QGuiApplication` o `QCoreApplication` para daemon).
- NO usar `QThread` sin justificación en el commit. Preferir `QtConcurrent::run`.
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
| Timer one-shot | `QTimer::singleShot(ms, this, &MyClass::slot)` |
| Fecha hoy (SQL) | `QDate::currentDate().toString(Qt::ISODate)` |
| DB en memoria (tests) | `QSqlDatabase::addDatabase("QSQLITE"); db.setDatabaseName(":memory:")` |
| Operación asíncrona DB | `QtConcurrent::run([=]() { ... })` + `QFutureWatcher` |
| Notificación D-Bus | `QDBusInterface("org.freedesktop.Notifications", ...)` |
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
│  My Day                  │  ← SpecialListSection.qml
│  Schedule                │     (hardcodeado, no editable)
│  Tasks                   │
│ ─────────────────────── │  ← SectionSeparator.qml (1px, Theme.border)
│  Lista custom 1          │  ← UserListSection.qml
│  Lista custom 2          │     (listas del usuario)
│  + New List              │
│                          │
│ [Log in on MS To Do]     │  ← footer, pegado al bottom
└──────────────────────────┘
```

- Las 3 vistas especiales (My Day, Schedule, Tasks) son **fijas e inmutables** en el orden de arriba.
- No se puede mover, renombrar ni eliminar My Day, Schedule ni Tasks.
- El separador `SectionSeparator.qml` SIEMPRE está presente.
- El footer "Log in on MS To Do" SIEMPRE pegado al `bottom` del SidePanel con `anchors.bottom`.

### 13.3 InputBar: Zona C Obligatoria

La `InputBar.qml` SIEMPRE aparece anclada al bottom del `TaskView`. Nunca desaparece.
Los tres iconos (📅 ⏰ 🔄 📝) son SIEMPRE visibles en el InputBar — no se ocultan.

```qml
// CORRECTO — InputBar pegada al bottom
TaskView {
    InputBar {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 44
    }
}

// INCORRECTO — InputBar flotante o condicional
InputBar { visible: listSelected }  // PROHIBIDO — siempre visible
```

### 13.4 Iconos: Solo Unicode / Texto

Los iconos de la interfaz se implementan con caracteres Unicode.
**Prohibido** usar archivos de imagen (PNG, SVG externos) para iconos de UI.

```qml
// CORRECTO
Text { text: "✓"; font.family: "iA Writer Mono" }
Text { text: "≡" }
Text { text: "✕" }   // cerrar ventana

// INCORRECTO
Image { source: ":/icons/close.png" }  // PROHIBIDO para iconos de UI
```

### 13.5 Eliminación de Tareas: Sin Diálogos Modales

La confirmación de eliminación (icono 🗑) DEBE ocurrir dentro del InputBar, no en un diálogo.

```qml
// CORRECTO — confirmación inline en el InputBar
// Al hacer clic en 🗑:
//   InputBar cambia temporalmente a: "¿Eliminar «Título de tarea»?" [Sí] [No]
//   Timeout de 5s → auto-cancelar
//   Sí → delete → restaurar InputBar normal

// INCORRECTO
Dialog { ... }         // PROHIBIDO
MessageBox { ... }     // PROHIBIDO
QMessageBox { ... }    // PROHIBIDO (también desde C++)
```
