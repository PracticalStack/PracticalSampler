cmake_minimum_required(VERSION 3.28)

project(DecentRhapsodyStudio VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRS_ENABLE_HISE_INTEGRATION "Enable experimental HISE-backed integration work" OFF)
option(DRS_ENABLE_THREAD_SANITIZER "Instrument supported builds with ThreadSanitizer" OFF)

if(DRS_ENABLE_THREAD_SANITIZER)
    if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "DRS_ENABLE_THREAD_SANITIZER requires Clang or GCC on a non-MSVC build.")
    endif()

    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_MODULES_ONLY OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set(JUCE_COPY_PLUGIN_AFTER_BUILD OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/juce EXCLUDE_FROM_ALL)

message(STATUS "Decent Rhapsody Studio Phase 0 configuration")
message(STATUS "  JUCE source: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/juce")
message(STATUS "  HISE source: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hise")
message(STATUS "  HISE CMake integration enabled: ${DRS_ENABLE_HISE_INTEGRATION}")

if(NOT DRS_ENABLE_HISE_INTEGRATION)
    message(STATUS "  Note: HISE is vendored but not yet integrated into the CMake build.")
endif()

add_subdirectory(engine_adapter)
add_subdirectory(app)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
