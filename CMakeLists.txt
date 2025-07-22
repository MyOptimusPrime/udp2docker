cmake_minimum_required(VERSION 3.15)
project(udp2docker VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译器选项
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 and later
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    if(WIN32)
        # MinGW特定选项
        add_definitions(-D_WIN32_WINNT=0x0601)
        add_definitions(-DWINVER=0x0601)
        # 解决MinGW的一些兼容性问题
        add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
    endif()
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(include)

# 查找Windows Socket库
if(WIN32)
    if(MSVC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
        find_library(WSOCK32_LIBRARY wsock32)
        find_library(WS2_32_LIBRARY ws2_32)
    else()
        # MinGW/GCC需要直接链接系统库
        set(WSOCK32_LIBRARY wsock32)
        set(WS2_32_LIBRARY ws2_32)
    endif()
endif()

# 源文件
set(SOURCES
    src/udp_client.cpp
    src/message_protocol.cpp
    src/config_manager.cpp
    src/logger.cpp
)

# 头文件
set(HEADERS
    include/udp2docker/udp_client.h
    include/udp2docker/message_protocol.h
    include/udp2docker/config_manager.h
    include/udp2docker/logger.h
    include/udp2docker/common.h
)

# 创建静态库
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES} ${HEADERS})

# 链接库
if(WIN32)
    target_link_libraries(${PROJECT_NAME}_lib ${WS2_32_LIBRARY} ${WSOCK32_LIBRARY})
    if(NOT MSVC)
        # MinGW需要额外链接文件系统库和线程库
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
            target_link_libraries(${PROJECT_NAME}_lib stdc++fs)
        endif()
        target_link_libraries(${PROJECT_NAME}_lib pthread)
    endif()
else()
    # Linux下可能需要链接文件系统库和线程库
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        target_link_libraries(${PROJECT_NAME}_lib stdc++fs)
    endif()
    target_link_libraries(${PROJECT_NAME}_lib pthread)
endif()

# 创建主程序
add_executable(${PROJECT_NAME} examples/main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)

# 创建测试程序
add_executable(${PROJECT_NAME}_test tests/test_main.cpp)
target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME}_lib)

# 安装规则
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(DIRECTORY include/ DESTINATION include)

# 打包配置
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack) 