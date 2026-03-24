cmake_minimum_required(VERSION 3.20)

project(__HONTO_PROJECT_NAME__ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(HonToEngine CONFIG REQUIRED)

get_filename_component(HONTO_SDK_ROOT "${HonToEngine_DIR}/../../.." ABSOLUTE)

set(HONTO_SIGN_SCRIPT "${HONTO_SDK_ROOT}/bin/Sign-Binary.ps1" CACHE FILEPATH "HonTo signing script")
set(HONTO_SIGNTOOL_PATH "$ENV{HONTO_SIGNTOOL_EXE}" CACHE FILEPATH "Path to signtool.exe")
set(HONTO_SIGN_PFX_PATH "$ENV{HONTO_SIGN_PFX_PATH}" CACHE FILEPATH "Path to a PFX certificate")
set(HONTO_SIGN_PFX_PASSWORD "$ENV{HONTO_SIGN_PFX_PASSWORD}" CACHE STRING "Password for the PFX certificate")
set(HONTO_SIGN_CERT_SHA1 "$ENV{HONTO_SIGN_CERT_SHA1}" CACHE STRING "Certificate thumbprint from the Windows certificate store")
set(HONTO_SIGN_CERT_SUBJECT "$ENV{HONTO_SIGN_CERT_SUBJECT}" CACHE STRING "Certificate subject name from the Windows certificate store")
set(HONTO_SIGN_MACHINE_STORE "$ENV{HONTO_SIGN_MACHINE_STORE}" CACHE STRING "Use the machine store when set to 1")
set(HONTO_SIGN_TIMESTAMP_URL "$ENV{HONTO_SIGN_TIMESTAMP_URL}" CACHE STRING "RFC3161 timestamp server")

add_executable(__HONTO_PROJECT_NAME__
    src/main.cpp
)

target_link_libraries(__HONTO_PROJECT_NAME__ PRIVATE HonTo::Engine)

if(MSVC)
    target_compile_options(__HONTO_PROJECT_NAME__ PRIVATE /W4 /permissive-)
else()
    target_compile_options(__HONTO_PROJECT_NAME__ PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(__HONTO_PROJECT_NAME__ PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(EXISTS "${HONTO_SIGN_SCRIPT}")
    if(HONTO_SIGN_PFX_PATH OR HONTO_SIGN_CERT_SHA1 OR HONTO_SIGN_CERT_SUBJECT)
        add_custom_command(TARGET __HONTO_PROJECT_NAME__ POST_BUILD
            COMMAND powershell -ExecutionPolicy Bypass -File "${HONTO_SIGN_SCRIPT}"
                -TargetPath "$<TARGET_FILE:__HONTO_PROJECT_NAME__>"
                -SignToolPath "${HONTO_SIGNTOOL_PATH}"
                -PfxPath "${HONTO_SIGN_PFX_PATH}"
                -PfxPassword "${HONTO_SIGN_PFX_PASSWORD}"
                -CertSha1 "${HONTO_SIGN_CERT_SHA1}"
                -CertSubject "${HONTO_SIGN_CERT_SUBJECT}"
                -MachineStore "${HONTO_SIGN_MACHINE_STORE}"
                -TimestampUrl "${HONTO_SIGN_TIMESTAMP_URL}"
            VERBATIM
        )
    endif()
endif()
