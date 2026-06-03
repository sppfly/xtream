add_library(xtream_core STATIC
    util/log/logger.cpp
    engine/slot.cpp
    engine/slot_manager.cpp
    engine/slot_engine.cpp
)
target_include_directories(xtream_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(xtream main.cpp)
target_link_libraries(xtream PRIVATE xtream_core)

if(ENABLE_ASAN)
  target_compile_options(xtream PRIVATE -fsanitize=address)
  target_link_options(xtream PRIVATE -fsanitize=address)
endif()
if(ENABLE_UBSAN)
  target_compile_options(xtream PRIVATE -fsanitize=undefined)
  target_link_options(xtream PRIVATE -fsanitize=undefined)
endif()

if(ENABLE_ASAN)
  target_compile_options(xtream_core PUBLIC -fsanitize=address)
  target_link_options(xtream_core PUBLIC -fsanitize=address)
endif()
if(ENABLE_UBSAN)
  target_compile_options(xtream_core PUBLIC -fsanitize=undefined)
  target_link_options(xtream_core PUBLIC -fsanitize=undefined)
endif()
