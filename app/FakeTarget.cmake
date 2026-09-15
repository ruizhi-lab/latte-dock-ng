execute_process(COMMAND find ../shell ../containment ../plasmoid -name "*.qml" -o -name "*.js"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE QML_SRCS_STRING)

string(REPLACE "\n" ";" QML_SRCS ${QML_SRCS_STRING})

# fake target for QtCreator project
add_custom_target(fake-target
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    SOURCES ${QML_SRCS})

# qmllint: qml static syntax checker
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT DEFINED ECM_ENABLE_SANITIZERS)
    find_program(QMLLINT NAMES qmllint-qt6 qmllint HINTS /usr/lib64/qt6/bin /usr/lib/qt6/bin)

    if(EXISTS "${QMLLINT}")
        message(STATUS "Found qmllint: ${QMLLINT}")
        add_custom_command(TARGET latte-dock-ng PRE_BUILD
            COMMAND ${QMLLINT} ${QML_SRCS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running qmllint")
    else()
        message(STATUS "qmllint: QML Syntax verifier not found")
    endif()

    message(STATUS "Enabling QML debugging and profiling")
    target_compile_definitions(latte-dock-ng PRIVATE QT_QML_DEBUG QT_FATAL_WARNINGS)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Disabling debug info")
    target_compile_definitions(latte-dock-ng PRIVATE QT_NO_DEBUG)

endif()
