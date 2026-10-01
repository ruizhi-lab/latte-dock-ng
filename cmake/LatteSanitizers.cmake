function(latte_enable_sanitizers target)
    if(NOT LATTE_ENABLE_ASAN_UBSAN)
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        message(FATAL_ERROR "LATTE_ENABLE_ASAN_UBSAN requires GCC or Clang")
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Cannot enable sanitizers for missing target '${target}'")
    endif()

    get_target_property(_latte_imported "${target}" IMPORTED)
    get_target_property(_latte_type "${target}" TYPE)
    if(_latte_imported OR _latte_type STREQUAL "INTERFACE_LIBRARY" OR _latte_type STREQUAL "UTILITY")
        return()
    endif()

    target_compile_options("${target}" PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address,undefined;-fno-omit-frame-pointer>"
    )

    if(_latte_type STREQUAL "EXECUTABLE" OR _latte_type STREQUAL "SHARED_LIBRARY" OR _latte_type STREQUAL "MODULE_LIBRARY")
        target_link_options("${target}" PRIVATE -fsanitize=address,undefined)
    endif()

    set_property(GLOBAL APPEND PROPERTY LATTE_SANITIZER_TARGETS "${target}")
endfunction()

function(latte_apply_sanitizers_to_directory directory)
    get_property(_latte_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_latte_target IN LISTS _latte_targets)
        latte_enable_sanitizers("${_latte_target}")
    endforeach()

    get_property(_latte_subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_latte_subdirectory IN LISTS _latte_subdirectories)
        latte_apply_sanitizers_to_directory("${_latte_subdirectory}")
    endforeach()
endfunction()

function(latte_enable_sanitizers_for_project)
    if(NOT LATTE_ENABLE_ASAN_UBSAN)
        return()
    endif()

    latte_apply_sanitizers_to_directory("${CMAKE_SOURCE_DIR}")
    get_property(_latte_sanitizer_targets GLOBAL PROPERTY LATTE_SANITIZER_TARGETS)
    list(SORT _latte_sanitizer_targets)
    list(REMOVE_DUPLICATES _latte_sanitizer_targets)
    list(LENGTH _latte_sanitizer_targets _latte_sanitizer_target_count)
    file(WRITE "${CMAKE_BINARY_DIR}/latte-sanitizer-targets.txt"
        "${_latte_sanitizer_targets}\n")
    message(STATUS "ASan/UBSan enabled for ${_latte_sanitizer_target_count} first-party compile targets")
endfunction()
