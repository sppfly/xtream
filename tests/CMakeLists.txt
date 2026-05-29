include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(extream_tests
    core/types/int_test.cpp
    core/types/uint_test.cpp
    core/types/float_test.cpp
    core/event_test.cpp
    core/watermark_test.cpp
    core/stream_schema_test.cpp
    core/record_test.cpp
    core/id_test.cpp
    util/log/logger_test.cpp
)
target_link_libraries(extream_tests PRIVATE extream_core GTest::gtest GTest::gtest_main)

if(ENABLE_ASAN)
    target_compile_options(extream_tests PRIVATE -fsanitize=address)
    target_link_options(extream_tests PRIVATE -fsanitize=address)
endif()
if(ENABLE_UBSAN)
    target_compile_options(extream_tests PRIVATE -fsanitize=undefined)
    target_link_options(extream_tests PRIVATE -fsanitize=undefined)
endif()

include(GoogleTest)
gtest_discover_tests(extream_tests)
