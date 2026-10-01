cmake_minimum_required(VERSION 3.20)
project(LatteStrictWarningFixture LANGUAGES CXX)

if(NOT DEFINED LATTE_STRICT_WARNINGS)
    set(LATTE_STRICT_WARNINGS ON)
endif()

include("${LATTE_STRICT_WARNINGS_MODULE}")

option(FIXTURE_ENABLE_WARNING "Build the source with an intentional unused-variable warning." ON)
if(FIXTURE_ENABLE_WARNING)
    add_executable(strict-warning-fixture warning.cpp)
else()
    add_executable(strict-warning-fixture clean.cpp)
endif()
target_compile_options(strict-warning-fixture PRIVATE -Wall -Wextra)
latte_enable_strict_warnings(strict-warning-fixture)
