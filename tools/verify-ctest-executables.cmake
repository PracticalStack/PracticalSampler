cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED TEST_PRESET OR TEST_PRESET STREQUAL "")
    message(FATAL_ERROR "TEST_PRESET must name a configured CTest preset")
endif()

get_filename_component(drs_source_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

execute_process(
    COMMAND ctest --preset "${TEST_PRESET}" --show-only=json-v1
    WORKING_DIRECTORY "${drs_source_dir}"
    RESULT_VARIABLE ctest_result
    OUTPUT_VARIABLE ctest_json
    ERROR_VARIABLE ctest_error
)

if(NOT ctest_result EQUAL 0)
    message(FATAL_ERROR
        "Could not inspect CTest preset '${TEST_PRESET}' (exit ${ctest_result}).\n${ctest_error}")
endif()

string(JSON test_count LENGTH "${ctest_json}" tests)
if(test_count EQUAL 0)
    message(FATAL_ERROR "CTest preset '${TEST_PRESET}' did not register any tests")
endif()

set(missing_executables)
math(EXPR last_test_index "${test_count} - 1")

foreach(test_index RANGE 0 ${last_test_index})
    string(JSON test_name GET "${ctest_json}" tests ${test_index} name)
    string(JSON command_type ERROR_VARIABLE command_error
        TYPE "${ctest_json}" tests ${test_index} command)
    if(NOT command_error STREQUAL "NOTFOUND")
        list(APPEND missing_executables "${test_name}: <CTest could not resolve command>")
        continue()
    endif()

    string(JSON command_count LENGTH "${ctest_json}" tests ${test_index} command)
    if(command_count EQUAL 0)
        list(APPEND missing_executables "${test_name}: <empty command>")
        continue()
    endif()

    string(JSON test_executable GET "${ctest_json}" tests ${test_index} command 0)
    set(resolved_executable "")

    if(IS_ABSOLUTE "${test_executable}")
        set(resolved_executable "${test_executable}")
    else()
        unset(resolved_program CACHE)
        unset(resolved_program)
        find_program(resolved_program NAMES "${test_executable}")
        if(resolved_program)
            set(resolved_executable "${resolved_program}")
        endif()
    endif()

    if(resolved_executable STREQUAL ""
       OR NOT EXISTS "${resolved_executable}"
       OR IS_DIRECTORY "${resolved_executable}")
        list(APPEND missing_executables "${test_name}: ${test_executable}")
    endif()
endforeach()

if(missing_executables)
    list(JOIN missing_executables "\n  " missing_report)
    list(LENGTH missing_executables missing_count)
    message(FATAL_ERROR
        "${missing_count} registered CTest command executable(s) are missing after the aggregate build:\n"
        "  ${missing_report}")
endif()

message(STATUS
    "Verified ${test_count} registered tests reference available command executables")
