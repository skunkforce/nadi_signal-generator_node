# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.20)

# Define the project name and version
project(NADIIotaProject VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler-specific extensions

# Define the DLL target
add_library(NADIIota SHARED
    src/main.cpp
    include/nadi.h
)

# Set properties for DLL to ensure C-ABI and export symbols
target_compile_definitions(NADIIota
    PRIVATE NADIIOTA_EXPORTS # Used to control export/import in header
)

# Specify include directories for the DLL
target_include_directories(NADIIota
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Ensure position-independent code for shared libraries (Linux/macOS)
set_target_properties(NADIIota PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)


# Platform-specific settings
if(WIN32)
    # On Windows, ensure DLL exports symbols correctly
    set_target_properties(NADIIota PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF # We control exports manually
    )
endif()

