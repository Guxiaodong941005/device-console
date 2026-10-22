cmake_minimum_required(VERSION 3.16)
project(DeviceConsole VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Optional: Qt MQTT (not packaged on many distros). Enable with:
#   cmake -B build -DDEVICE_CONSOLE_USE_QT_MQTT=ON
option(DEVICE_CONSOLE_USE_QT_MQTT "Link Qt6::Mqtt if available" OFF)

set(DEVICE_CONSOLE_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/ConnectionStateMachine.cpp
    src/MockMqttClient.cpp
    include/MainWindow.h
    include/ConnectionStateMachine.h
    include/MqttClientInterface.h
    include/MockMqttClient.h
)

if(DEVICE_CONSOLE_USE_QT_MQTT)
    find_package(Qt6 COMPONENTS Mqtt)
    if(Qt6Mqtt_FOUND)
        list(APPEND DEVICE_CONSOLE_SOURCES src/QtMqttClient.cpp include/QtMqttClient.h)
        add_compile_definitions(DEVICE_CONSOLE_HAS_QT_MQTT=1)
        message(STATUS "Qt6 Mqtt found — building real client wrapper")
    else()
        message(WARNING "DEVICE_CONSOLE_USE_QT_MQTT=ON but Qt6 Mqtt not found; using Mock only")
    endif()
endif()

add_executable(device_console ${DEVICE_CONSOLE_SOURCES})

target_include_directories(device_console PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(device_console PRIVATE Qt6::Widgets Qt6::Network)

if(DEVICE_CONSOLE_USE_QT_MQTT AND Qt6Mqtt_FOUND)
    target_link_libraries(device_console PRIVATE Qt6::Mqtt)
endif()

install(TARGETS device_console RUNTIME DESTINATION bin)
