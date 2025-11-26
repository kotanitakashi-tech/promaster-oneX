cmake_minimum_required(VERSION 3.16)
project(PromasterOne VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for low CPU usage
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -march=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /fp:fast /arch:AVX2")
endif()

# Find VST3 SDK
# Set this to your VST3 SDK path
set(VST3_SDK_ROOT "${CMAKE_SOURCE_DIR}/../VST_SDK/vst3sdk" CACHE PATH "Path to VST3 SDK")

if(EXISTS "${VST3_SDK_ROOT}")
    message(STATUS "Found VST3 SDK at ${VST3_SDK_ROOT}")
    add_subdirectory(${VST3_SDK_ROOT} ${CMAKE_BINARY_DIR}/vst3sdk)
else()
    message(STATUS "VST3 SDK not found - building standalone library only")
endif()

# Source files
set(SOURCES
    source/DSPCore.cpp
    source/VST3Plugin.cpp
    source/GUIController.cpp
)

set(HEADERS
    include/PromasterOne.h
)

# Build as shared library
add_library(PromasterOne SHARED ${SOURCES} ${HEADERS})

target_include_directories(PromasterOne PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(PromasterOne PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        _USE_MATH_DEFINES
    )
    set_target_properties(PromasterOne PROPERTIES
        SUFFIX ".vst3"
    )
elseif(APPLE)
    set_target_properties(PromasterOne PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "vst3"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist"
    )
else()
    set_target_properties(PromasterOne PROPERTIES
        SUFFIX ".so"
    )
endif()

# Installation paths for Cakewalk Sonar
if(WIN32)
    # Default VST3 path on Windows
    set(VST3_INSTALL_PATH "$ENV{COMMONPROGRAMFILES}/VST3" CACHE PATH "VST3 installation path")
else()
    set(VST3_INSTALL_PATH "$ENV{HOME}/.vst3" CACHE PATH "VST3 installation path")
endif()

install(TARGETS PromasterOne
    LIBRARY DESTINATION "${VST3_INSTALL_PATH}/PromasterOne.vst3/Contents/x86_64-win"
    RUNTIME DESTINATION "${VST3_INSTALL_PATH}/PromasterOne.vst3/Contents/x86_64-win"
)

# Copy presets
install(DIRECTORY presets/
    DESTINATION "${VST3_INSTALL_PATH}/PromasterOne.vst3/Contents/Resources/Presets"
    PATTERN "*.pmo"
)

# Print configuration summary
message(STATUS "")
message(STATUS "=== PROMASTER ONE Build Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Install path: ${VST3_INSTALL_PATH}")
message(STATUS "=========================================")
message(STATUS "")
