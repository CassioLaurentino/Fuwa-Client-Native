cmake_minimum_required(VERSION 3.21)
project(FuwaClientNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Prefixos de libs nativas pesadas (libwebrtc/libmediasoupclient/Qt/sioclient)
# sao fornecidos via -DCMAKE_PREFIX_PATH, nao vendorizados neste repo (ver README.md).
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(nlohmann_json REQUIRED)
find_package(mediasoupclient REQUIRED)
find_package(sioclient REQUIRED)

add_executable(fuwa-client-native
    src/main.cpp
    src/AppConfig.cpp
    src/SignalingClient.cpp
    src/MediaController.cpp
    src/ui/MainWindow.cpp
    src/ui/VideoTile.cpp
)

target_include_directories(fuwa-client-native PRIVATE src)

if(WIN32)
    target_sources(fuwa-client-native PRIVATE src/audio/WasapiLoopbackCapturer.cpp)
    target_link_libraries(fuwa-client-native PRIVATE ole32 avrt mmdevapi)
endif()

target_link_libraries(fuwa-client-native PRIVATE
    Qt6::Widgets
    nlohmann_json::nlohmann_json
    mediasoupclient::mediasoupclient
    sioclient::sioclient
)

install(TARGETS fuwa-client-native RUNTIME DESTINATION .)
install(FILES config/fuwa-client.example.json DESTINATION . RENAME fuwa-client.json)
