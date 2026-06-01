add_library(xtream_core STATIC
    util/log/logger.cpp
)
target_include_directories(xtream_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/interfaces
)

if(ENABLE_ASAN)
  target_compile_options(xtream_core PUBLIC -fsanitize=address)
  target_link_options(xtream_core PUBLIC -fsanitize=address)
endif()
if(ENABLE_UBSAN)
  target_compile_options(xtream_core PUBLIC -fsanitize=undefined)
  target_link_options(xtream_core PUBLIC -fsanitize=undefined)
endif()
