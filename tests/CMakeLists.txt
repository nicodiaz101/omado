find_package(Qt6 REQUIRED COMPONENTS Core Test Sql DBus)

add_definitions(-DTEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

set(TEST_TARGETS
    tst_ThemeReader
    tst_Database
    tst_LocalRepository
    tst_NotificationService
    tst_ReminderService
    tst_DaemonService
)

foreach(test_name ${TEST_TARGETS})
    qt_add_executable(${test_name} ${test_name}.cpp)
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Sql
        Qt6::DBus
        omado_lib
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
