# CMake最低版本号要求
cmake_minimum_required(VERSION 3.8)

# 项目名称
project(XanaduJson VERSION 0.2.0)

# 设置安装路径
message(STATUS "Operation System : ${CMAKE_SYSTEM}")
if (WIN32)
	message(STATUS "Current Platform : WIN32")
	set(CMAKE_INSTALL_PREFIX C:/Xanadu)
elseif (MINGW)
	message(STATUS "Current Platform : MINGW")
elseif (APPLE)
	message(STATUS "Current Platform : APPLE")
elseif (UNIX)
	message(STATUS "Current Platform : UNIX")
endif ()

# 配置查找包
set(XANADU_INSTALL_CMAKE "${CMAKE_SOURCE_DIR}/config/Find${PROJECT_NAME}.cmake")
set(XANADU_INSTALL_PKG_CONFIG "${CMAKE_SOURCE_DIR}/config/${PROJECT_NAME}.pc")
configure_file("${XANADU_INSTALL_CMAKE}.in" "${XANADU_INSTALL_CMAKE}")
configure_file("${XANADU_INSTALL_PKG_CONFIG}.in" "${XANADU_INSTALL_PKG_CONFIG}")

# 设置模块搜索路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX}/lib/cmake)

# 项目依赖
find_package(XanaduRuntime REQUIRED)
find_package(XanaduCore REQUIRED)

# C++支持
set(CMAKE_CXX_STANDARD 17)

# 头文件搜索目录
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${XANADU_RUNTIME_INCLUDE})
include_directories(${XANADU_CORE_INCLUDE})

# 链接库搜索目录
link_directories(${XANADU_RUNTIME_LIBRARY})
link_directories(${XANADU_CORE_LIBRARY})

# 设置静态编译宏
#add_definitions(-DXANADU_JSON_BUILD_STATIC)
# 设置导出宏
add_definitions(-DXANADU_JSON_LIB)

# 包含源文件
aux_source_directory(./source SRC)
if (MSVC)
	set(VERSIONINFO_RC "${CMAKE_BINARY_DIR}/VersionInfo.rc")
	configure_file("${CMAKE_SOURCE_DIR}/config/VersionInfo.rc.in" "${VERSIONINFO_RC}")
	list(APPEND SRC ${VERSIONINFO_RC})
endif ()

# 设置输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 链接库
if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	link_libraries(XanaduRuntimeD)
	link_libraries(XanaduCoreD)
else ()
	link_libraries(XanaduRuntime)
	link_libraries(XanaduCore)
endif ()

# 构建
add_library(XanaduJson SHARED ${SRC})

# 设置项目属性
set_target_properties(XanaduJson PROPERTIES VERSION 0.2.0 SOVERSION 0)
set_target_properties(XanaduJson PROPERTIES DEBUG_POSTFIX D)

# 设置安装文件(静态库)
install(TARGETS XanaduJson ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
# 设置安装文件(动态库)
install(TARGETS XanaduJson LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
# 设置安装文件(可执行文件)
#install(TARGETS XanaduJson RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
# 设置安装文件(与库关联的PUBLIC头文件)
#install(TARGETS XanaduJson PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# 设置安装文件(与库关联的PRIVATE头文件)
#install(TARGETS XanaduJson PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# 安装头文件
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/XanaduJson DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# 安装头文件
#install(FILES ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
# 配置模块文件
install(FILES ${PROJECT_SOURCE_DIR}/config/FindXanaduJson.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake)
install(FILES ${PROJECT_SOURCE_DIR}/config/XanaduJson.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
