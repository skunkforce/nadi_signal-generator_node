# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.27)

set(PROJECT_NAME "nadi_signal-generator_node")

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler-specific extensions

include(FetchContent)

if(NOT TARGET CLI11::CLI11)
    set(CLI11_BUILD_TESTS OFF)
    set(CLI11_BUILD_EXAMPLES OFF)
    # CLI11
    FetchContent_Declare(
        cli11_proj
        QUIET
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.6.1
    )

    FetchContent_MakeAvailable(cli11_proj)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(json)
endif()

if(NOT TARGET nadicpp::nadicpp)
    add_subdirectory(nadicpp)
endif()

# get latest git tag
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Fallback if no git tag exists
if (NOT GIT_VERSION)
    set(GIT_VERSION "0.0.0") # standard version
endif()

# Set the version in the project
message(STATUS "Project version: ${GIT_VERSION}")
add_definitions(-DPROJECT_VERSION="${GIT_VERSION}")

project(${PROJECT_NAME} VERSION 0.0.1)

# Define the DLL target
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)


# Platform-specific settings
if(WIN32)
    # On Windows, ensure DLL exports symbols correctly
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF # We control exports manually
    )
endif()

target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json nadi::nadi nadicpp::nadicpp)


