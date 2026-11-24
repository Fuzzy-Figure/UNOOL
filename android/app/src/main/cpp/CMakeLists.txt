cmake_minimum_required(VERSION 3.22.1)
project(unool_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# android/app/src/main/cpp -> 仓库根
get_filename_component(UNOOL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

file(GLOB_RECURSE UNOOL_CORE_SOURCES CONFIGURE_DEPENDS "${UNOOL_ROOT}/source/core/*.cpp")
file(GLOB UNOOL_PROTOCOL_SOURCES CONFIGURE_DEPENDS "${UNOOL_ROOT}/source/protocol/*.cpp")

add_library(unool-server SHARED
    AndroidServer.cpp
    AndroidPlatform.cpp
    ${UNOOL_CORE_SOURCES}
    ${UNOOL_PROTOCOL_SOURCES}
    ${UNOOL_ROOT}/source/utils.cpp
    ${UNOOL_ROOT}/source/shell/ServerNetwork.cpp
    ${UNOOL_ROOT}/source/shell/RoomManager.cpp
    ${UNOOL_ROOT}/source/shell/RoomGame.cpp
    ${UNOOL_ROOT}/source/shell/RoomKeyMapping.cpp
    ${UNOOL_ROOT}/source/shell/UserDB.cpp
    ${UNOOL_ROOT}/source/shell/JsonConfig.cpp
)

# SFML 最小实现需要先于系统头被找到。
target_include_directories(unool-server BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sfmlshim"
)

target_include_directories(unool-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${UNOOL_ROOT}/header"
    "${UNOOL_ROOT}/header/core"
    "${UNOOL_ROOT}/header/shell"
    "${UNOOL_ROOT}/header/protocol"
    "${UNOOL_ROOT}/dep/nlohmann"
)

target_compile_definitions(unool-server PRIVATE UNOOL_ANDROID=1)

# 商城 DTO 使用 std::optional；强制在每个 TU 注入 from_json 重载。
target_compile_options(unool-server PRIVATE
    -include "${CMAKE_CURRENT_SOURCE_DIR}/compat/json_optional.hpp"
)

find_library(ANDROID_LOG_LIBRARY log)
find_library(ANDROID_LIBRARY android)
target_link_libraries(unool-server ${ANDROID_LOG_LIBRARY} ${ANDROID_LIBRARY})
