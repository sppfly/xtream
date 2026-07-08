include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(xtream_tests
    core/types/int_test.cpp
    core/types/uint_test.cpp
    core/types/float_test.cpp
    core/event_test.cpp
    core/watermark_test.cpp
    core/stream_schema_test.cpp
    core/record_test.cpp
    core/id_test.cpp
    graph/dataflow_graph_test.cpp
    graph/graph_print_test.cpp
    util/log/logger_test.cpp
    functions/function_concepts_test.cpp
    operators/logical_operator_test.cpp
    operators/physical_operator_test.cpp
    operators/source_watermark_test.cpp
    operators/time_window_test.cpp
    operators/join_fluent_api_test.cpp
    engine/compiler_test.cpp
    engine/pipeline_test.cpp
    engine/execution_graph_print_test.cpp
    engine/slot_test.cpp
    engine/slot_manager_test.cpp
    engine/slot_engine_test.cpp
    runtime/event_buffer_test.cpp
)
target_link_libraries(xtream_tests PRIVATE xtream_core GTest::gtest GTest::gtest_main)

if(ENABLE_ASAN)
  target_compile_options(xtream_tests PRIVATE -fsanitize=address)
  target_link_options(xtream_tests PRIVATE -fsanitize=address)
endif()
if(ENABLE_UBSAN)
  target_compile_options(xtream_tests PRIVATE -fsanitize=undefined)
  target_link_options(xtream_tests PRIVATE -fsanitize=undefined)
endif()

include(GoogleTest)
gtest_discover_tests(xtream_tests)
