# 辅助输出信息
message("using FindXanaduJson.cmake find")

# 将头文件路径赋值给 XANADU_JSON_INCLUDE
if (WIN32)
	FIND_PATH(XANADU_JSON_INCLUDE XanaduJson/ C:/Xanadu/include/)
elseif (MINGW)
	FIND_PATH(XANADU_JSON_INCLUDE XanaduJson/ /usr/include/)
elseif (APPLE)
	FIND_PATH(XANADU_JSON_INCLUDE XanaduJson/ /usr/local/include/)
elseif (UNIX)
	FIND_PATH(XANADU_JSON_INCLUDE XanaduJson/ /usr/include/)
endif ()

# 将库文件路径赋值给 XANADU_JSON_LIBRARY
if (WIN32)
	FIND_PATH(XANADU_JSON_LIBRARY XanaduJson.lib C:/Xanadu/lib/)
elseif (MINGW)
	FIND_PATH(XANADU_JSON_LIBRARY XanaduJson.dll.a /usr/lib/)
elseif (APPLE)
	FIND_PATH(XANADU_JSON_LIBRARY libXanaduJson.dylib /usr/local/lib/)
elseif (UNIX)
	FIND_PATH(XANADU_JSON_LIBRARY libXanaduJson.so /usr/lib/)
endif ()

message("XANADU_JSON_INCLUDE: ${XANADU_JSON_INCLUDE}")
message("XANADU_JSON_LIBRARY: ${XANADU_JSON_LIBRARY}")

if(XANADU_JSON_INCLUDE AND XANADU_JSON_LIBRARY)
	# 设置变量结果
	set(XANADU_JSON_FOUND TRUE)
endif(XANADU_JSON_INCLUDE AND XANADU_JSON_LIBRARY)
