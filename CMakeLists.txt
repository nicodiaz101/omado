cmake_minimum_required(VERSION 3.21)
project(omado VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Sql DBus Concurrent)
find_package(Qt6Keychain REQUIRED)

# Librería estática interna para compartir con tests
add_library(omado_lib STATIC
    src/core/ThemeReader.cpp
    src/core/Database.cpp
    src/core/NotificationService.cpp
    src/core/ReminderService.cpp
    src/core/DaemonService.cpp
    src/core/KeychainStore.cpp
    src/core/AuthManager.cpp
    src/models/GraphClient.cpp
    src/core/SyncEngine.cpp
    src/models/LocalRepository.cpp
    src/models/TaskListModel.cpp
    src/models/TaskModel.cpp
)
target_include_directories(omado_lib PUBLIC src/)
target_link_libraries(omado_lib PUBLIC
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Network Qt6::Sql Qt6::DBus Qt6::Concurrent
    Qt6Keychain::Qt6Keychain
)

qt_add_executable(omado src/main.cpp)

qt_add_qml_module(omado
    URI "OmaDo"
    VERSION "1.0"
    QML_FILES
        src/qml/Main.qml
        src/qml/SidePanel.qml
        src/qml/TaskView.qml
        src/qml/TaskDetail.qml
        src/qml/InputBar.qml
        src/qml/TaskDelegate.qml
        src/qml/components/AppIcon.qml
        src/qml/components/CheckCircle.qml
        src/qml/components/SectionSeparator.qml
        src/qml/components/DatePickerPopup.qml
        src/qml/components/TimePickerPopup.qml
        src/qml/components/DraftStepsPopup.qml
        src/qml/components/RecurrencePopup.qml
    RESOURCES
        fonts/iAWriterMono-Regular.ttf
        fonts/iAWriterMono-Bold.ttf
        icons/alarm.svg
        icons/calendar.svg
        icons/close.svg
        icons/list.svg
        icons/plus.svg
        icons/repeat.svg
        icons/star.svg
        icons/steps.svg
        icons/sun.svg
        icons/tasks.svg
        icons/trash.svg
)

target_link_libraries(omado PRIVATE omado_lib Qt6::QuickControls2)

option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
