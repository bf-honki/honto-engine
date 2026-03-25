cmake_minimum_required(VERSION 3.20)

project(HonToEngine VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(HONTO_BUILD_SANDBOX "Build the bundled sandbox application" ON)
option(HONTO_BUILD_EXAMPLES "Build standalone example applications" ON)

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

set(HONTO_EXAMPLE_QUICKSTART_SOURCES
    examples/quickstart/src/main.cpp
)

add_library(honto_engine STATIC
    ${HONTO_ENGINE_HEADERS}
    ${HONTO_ENGINE_SOURCES}
)

add_library(HonTo::Engine ALIAS honto_engine)

target_include_directories(honto_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
    target_compile_options(honto_engine PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(honto_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(honto_engine PROPERTIES
    FOLDER "Engine"
    EXPORT_NAME Engine
)

if(HONTO_BUILD_SANDBOX)
    add_executable(honto_sandbox WIN32 ${HONTO_SANDBOX_SOURCES})
    target_link_libraries(honto_sandbox PRIVATE honto_engine)
    set_target_properties(honto_sandbox PROPERTIES
        FOLDER "Apps"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

if(HONTO_BUILD_EXAMPLES)
    add_executable(honto_example_quickstart ${HONTO_EXAMPLE_QUICKSTART_SOURCES})
    target_link_libraries(honto_example_quickstart PRIVATE honto_engine)
    set_target_properties(honto_example_quickstart PROPERTIES
        FOLDER "Examples"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/engine FILES
    ${HONTO_ENGINE_HEADERS}
    ${HONTO_ENGINE_SOURCES}
)

if(HONTO_BUILD_SANDBOX)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/sandbox FILES
        ${HONTO_SANDBOX_SOURCES}
    )
endif()

if(HONTO_BUILD_EXAMPLES)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/examples FILES
        ${HONTO_EXAMPLE_QUICKSTART_SOURCES}
    )
endif()

if(HONTO_BUILD_SANDBOX)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT honto_sandbox)
elseif(HONTO_BUILD_EXAMPLES)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT honto_example_quickstart)
endif()

configure_package_config_file(
    cmake/HonToEngineConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/HonToEngineConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/HonToEngine"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HonToEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS honto_engine
    EXPORT HonToEngineTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY engine/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT HonToEngineTargets
    FILE HonToEngineTargets.cmake
    NAMESPACE HonTo::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/HonToEngine"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HonToEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HonToEngineConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/HonToEngine"
)

install(FILES
    README.md
    docs/HONTO_ENGINE_GUIDE_KO.md
    docs/HONTO_ENGINE_SOURCE_GUIDE_KO.md
    docs/HONTO_GETTING_STARTED_KO.md
    docs/HONTO_INSTALLER_GUIDE_KO.md
    DESTINATION "${CMAKE_INSTALL_DATADIR}/HonToEngine"
)

install(DIRECTORY templates/ DESTINATION "${CMAKE_INSTALL_DATADIR}/HonToEngine/templates")
install(DIRECTORY examples/ DESTINATION "${CMAKE_INSTALL_DATADIR}/HonToEngine/examples")

install(PROGRAMS
    scripts/Sign-Binary.ps1
    scripts/Setup-LocalDevSigning.ps1
    scripts/Remove-LocalDevSigning.ps1
    scripts/New-HonToProject.ps1
    scripts/Build-HonToSdk.ps1
    scripts/Install-HonToSdk.ps1
    scripts/Build-HonToInstaller.ps1
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

set(CPACK_PACKAGE_NAME "HonToEngine SDK")
set(CPACK_PACKAGE_VENDOR "HonTo")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HonTo Engine SDK")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "HonToSDK")
set(CPACK_PACKAGE_FILE_NAME "HonToEngine-SDK-${PROJECT_VERSION}-win64")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "ZIP")

include(CPack)
