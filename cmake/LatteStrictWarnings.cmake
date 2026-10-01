function(latte_enable_strict_warnings target)
    if(NOT LATTE_STRICT_WARNINGS)
        return()
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Cannot enable strict warnings for missing target '${target}'")
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        return()
    endif()

    get_target_property(_latte_imported "${target}" IMPORTED)
    get_target_property(_latte_type "${target}" TYPE)
    if(_latte_imported OR _latte_type STREQUAL "INTERFACE_LIBRARY" OR _latte_type STREQUAL "UTILITY")
        return()
    endif()

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        set_property(TARGET "${target}" PROPERTY COMPILE_WARNING_AS_ERROR ON)
    else()
        target_compile_options("${target}" PRIVATE
            "$<$<OR:$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>>:-Werror>"
        )
    endif()
    set_property(GLOBAL APPEND PROPERTY LATTE_STRICT_WARNING_TARGETS "${target}")
endfunction()

# Apply after all project subdirectories have declared their targets so helper,
# plugin and EXCLUDE_FROM_ALL test binaries receive the same first-party gate.
function(latte_apply_strict_warnings_to_directory directory)
    get_property(_latte_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_latte_target IN LISTS _latte_targets)
        latte_enable_strict_warnings("${_latte_target}")
    endforeach()

    get_property(_latte_subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_latte_subdirectory IN LISTS _latte_subdirectories)
        latte_apply_strict_warnings_to_directory("${_latte_subdirectory}")
    endforeach()
endfunction()

function(latte_enable_strict_warnings_for_project)
    if(NOT LATTE_STRICT_WARNINGS)
        message(STATUS "LATTE_STRICT_WARNINGS=OFF: downstream escape hatch; this build does not satisfy the project warning policy")
        return()
    endif()

    latte_apply_strict_warnings_to_directory("${CMAKE_SOURCE_DIR}")
    get_property(_latte_strict_targets GLOBAL PROPERTY LATTE_STRICT_WARNING_TARGETS)
    list(SORT _latte_strict_targets)
    list(LENGTH _latte_strict_targets _latte_strict_target_count)
    file(WRITE "${CMAKE_BINARY_DIR}/latte-strict-warning-targets.txt"
        "${_latte_strict_targets}\n")
    message(STATUS "Strict compiler warnings enabled for ${_latte_strict_target_count} first-party compile targets")
endfunction()
