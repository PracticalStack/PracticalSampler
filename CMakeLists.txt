cmake_minimum_required(VERSION 3.28)

project(DecentRhapsodyStudio VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

set(DRS_PRODUCT_DISPLAY_NAME "Practical Sampler")
set(DRS_COMPANY_DISPLAY_NAME "Practical Sampler Project")

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRS_ENABLE_THREAD_SANITIZER "Instrument supported builds with ThreadSanitizer" OFF)

# The package content-protection path uses a maintained, upstream crypto
# implementation.  The release archive is hash-pinned because package
# encryption is part of the product trust boundary; no crypto primitive is
# implemented in product code.
include(FetchContent)
FetchContent_Declare(
    drs_libsodium
    URL https://github.com/jedisct1/libsodium/archive/refs/tags/1.0.20-RELEASE.tar.gz
    URL_HASH SHA256=8E5AECA07A723A27BBECC3BEEF14B0068D37E7FC0E97F51B3F1C82D2A58005C1
)
FetchContent_GetProperties(drs_libsodium)
if(NOT drs_libsodium_POPULATED)
    FetchContent_Populate(drs_libsodium)
endif()
file(GLOB_RECURSE DRS_LIBSODIUM_SOURCES CONFIGURE_DEPENDS
    "${drs_libsodium_SOURCE_DIR}/src/libsodium/*.c"
)
file(GLOB_RECURSE DRS_LIBSODIUM_INCLUDE_DIRS LIST_DIRECTORIES true
    "${drs_libsodium_SOURCE_DIR}/src/libsodium/*"
)
set(DRS_LIBSODIUM_INCLUDE_PATHS
    "${drs_libsodium_SOURCE_DIR}/src/libsodium/include"
    "${drs_libsodium_SOURCE_DIR}/src/libsodium/include/sodium"
    "${drs_libsodium_SOURCE_DIR}/builds/msvc"
    "${drs_libsodium_SOURCE_DIR}/src/libsodium"
)
foreach(DRS_LIBSODIUM_INCLUDE_DIR IN LISTS DRS_LIBSODIUM_INCLUDE_DIRS)
    if(IS_DIRECTORY "${DRS_LIBSODIUM_INCLUDE_DIR}")
        list(APPEND DRS_LIBSODIUM_INCLUDE_PATHS "${DRS_LIBSODIUM_INCLUDE_DIR}")
    endif()
endforeach()
add_library(drs_libsodium STATIC ${DRS_LIBSODIUM_SOURCES})
target_include_directories(drs_libsodium
    PUBLIC
        ${DRS_LIBSODIUM_INCLUDE_PATHS}
    PRIVATE
        ${DRS_LIBSODIUM_INCLUDE_PATHS}
)
target_compile_definitions(drs_libsodium PRIVATE SODIUM_STATIC=1)
if(WIN32)
    target_compile_definitions(drs_libsodium PRIVATE WINAPI_DESKTOP=1)
endif()
set_target_properties(drs_libsodium PROPERTIES FOLDER "Third Party")

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

message(STATUS "${DRS_PRODUCT_DISPLAY_NAME} Phase 0 configuration")
message(STATUS "  JUCE source: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/juce")
message(STATUS "  Runtime content root: ${CMAKE_CURRENT_SOURCE_DIR}/content/runtime")

add_subdirectory(engine_adapter)
add_subdirectory(app)
add_subdirectory(tools/package_signer)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
