cmake_minimum_required(VERSION 3.20)

project(HonToEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(HONTO_ENGINE_HEADERS
    engine/include/honto/Application.h
    engine/include/honto/Audio.h
    engine/include/honto/Color.h
    engine/include/honto/Components.h
    engine/include/honto/Easy.h
    engine/include/honto/Entity.h
    engine/include/honto/HonTo.h
    engine/include/honto/Input.h
    engine/include/honto/Level.h
    engine/include/honto/Math.h
    engine/include/honto/Raycast.h
    engine/include/honto/Renderer2D.h
    engine/include/honto/Scene.h
    engine/include/honto/SceneGraph.h
    engine/include/honto/TileMap.h
    engine/include/honto/Texture.h
    engine/include/honto/Transform2D.h
    engine/include/honto/Window.h
)

set(HONTO_ENGINE_SOURCES
    engine/src/Application.cpp
    engine/src/Audio.cpp
    engine/src/Input.cpp
    engine/src/Level.cpp
    engine/src/Raycast.cpp
    engine/src/Renderer2D.cpp
    engine/src/Scene.cpp
    engine/src/SceneGraph.cpp
    engine/src/TileMap.cpp
    engine/src/Texture.cpp
    engine/src/WindowWin32.cpp
)

set(HONTO_SANDBOX_SOURCES
    sandbox/src/main.cpp
)

add_library(honto_engine STATIC
    ${HONTO_ENGINE_HEADERS}
    ${HONTO_ENGINE_SOURCES}
)

target_include_directories(honto_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
)

target_compile_definitions(honto_engine PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(honto_engine PUBLIC
    user32
    gdi32
    gdiplus
    winmm
)

if(MSVC)
    target_compile_options(honto_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(honto_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(honto_sandbox ${HONTO_SANDBOX_SOURCES})

target_link_libraries(honto_sandbox PRIVATE honto_engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/engine FILES
    ${HONTO_ENGINE_HEADERS}
    ${HONTO_ENGINE_SOURCES}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/sandbox FILES
    ${HONTO_SANDBOX_SOURCES}
)

set_target_properties(honto_engine PROPERTIES FOLDER "Engine")
set_target_properties(honto_sandbox PROPERTIES
    FOLDER "Apps"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT honto_sandbox)
