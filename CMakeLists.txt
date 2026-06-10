# ============================================================
#  Namemark CMake Configuration
# ============================================================
cmake_minimum_required(VERSION 3.16)
project(namemark VERSION 1.4.0 LANGUAGES CXX)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Include directories ---
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
)

# --- Source files (auto-discovered via glob) ---
file(GLOB_RECURSE SOURCES
    src/*.cpp
    lib/customio23.cpp
)

# --- Executable target ---
add_executable(${PROJECT_NAME} ${SOURCES})

# --- Platform & compiler specific options ---
if(MSVC)
    # MSVC specific flags
    target_compile_options(${PROJECT_NAME} PRIVATE
        /utf-8                          # UTF-8 source and execution encoding
        /W4                             # Warning level 4
        /wd4100                         # Disable "unreferenced formal parameter"
        /wd4189                         # Disable "local variable is initialized but not referenced"
        /wd4505                         # Disable "unreferenced local function has been removed"
        /D_CRT_SECURE_NO_WARNINGS       # Disable secure CRT deprecation warnings
        $<$<CONFIG:Debug>:/Zi /Od /DDEBUG>
        $<$<CONFIG:Release>:/O2 /DNDEBUG>
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/DEBUG>
    )
else()
    # GCC/Clang flags (Linux / macOS)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
        -Wno-unused-variable
        $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
        $<$<CONFIG:Release>:-O2 -DNDEBUG>
    )
endif()

# --- Output name ---
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
        SUFFIX ".exe"
    )
endif()

# --- Export compile commands (for tools like clangd) ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type:   ${CMAKE_BUILD_TYPE}")