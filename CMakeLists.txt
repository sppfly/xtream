cmake_minimum_required(VERSION 3.21)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_program(CLANGXX_PROGRAM clang++)
if(CLANGXX_PROGRAM)
    set(CMAKE_CXX_COMPILER ${CLANGXX_PROGRAM})
endif()

find_program(MOLD_PROGRAM mold)
if(MOLD_PROGRAM)
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=mold")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=mold")
    set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=mold")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(xtream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable AddressSanitizer" ON)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" ON)

enable_testing()

file(GLOB_RECURSE FORMAT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/*.h
)
add_custom_target(format
    COMMAND clang-format -i ${FORMAT_SOURCES}
    COMMENT "Running clang-format on all source files"
)

add_subdirectory(src)
add_subdirectory(tests)
