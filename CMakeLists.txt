cmake_minimum_required(VERSION 3.10)
project(DR_CursorTracker)

# 設定版本號
set(MAJOR_VERSION 1)
set(MINOR_VERSION 0)
set(PATCH_VERSION 2)

set(DR_CURSOR_TRACKER_SOURCES
    dr_cursor_tracker.c
)

# .rc 檔案處理
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/obs-module.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/obs-module.rc
    @ONLY
)

# 新增 resource file 到 source
list(APPEND DR_CURSOR_TRACKER_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/obs-module.rc
)

add_library(DR_CursorTracker MODULE ${DR_CURSOR_TRACKER_SOURCES})

target_include_directories(DR_CursorTracker PRIVATE
    $ENV{OBS_SRC}/libobs
    $ENV{OBS_SRC}/deps
)

target_link_libraries(DR_CursorTracker
    libobs
)

install(TARGETS DR_CursorTracker DESTINATION ${CMAKE_INSTALL_PREFIX}/obs-plugins/${OBS_PLUGIN_DESTINATION})