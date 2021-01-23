# Explain
message("using FindXanaduJson.cmake find")

# Find XANADU_JSON_INCLUDE
if (WIN32)
	find_path(XANADU_JSON_INCLUDE XanaduJson/ C:/Xanadu/include/)
elseif (MINGW)
	find_path(XANADU_JSON_INCLUDE XanaduJson/ /usr/include/)
elseif (APPLE)
	find_path(XANADU_JSON_INCLUDE XanaduJson/ /usr/local/include/)
elseif (UNIX)
	find_path(XANADU_JSON_INCLUDE XanaduJson/ /usr/include/)
endif ()

# Find XANADU_JSON_LIBRARY
if (WIN32)
	find_path(XANADU_JSON_LIBRARY XanaduJson.lib C:/Xanadu/lib/)
elseif (MINGW)
	find_path(XANADU_JSON_LIBRARY XanaduJson.dll.a /usr/lib/)
elseif (APPLE)
	find_path(XANADU_JSON_LIBRARY libXanaduJson.dylib /usr/local/lib/)
elseif (UNIX)
	find_path(XANADU_JSON_LIBRARY libXanaduJson.so /usr/lib/)
endif ()

message("XANADU_JSON_INCLUDE: ${XANADU_JSON_INCLUDE}")
message("XANADU_JSON_LIBRARY: ${XANADU_JSON_LIBRARY}")

# Setting
if(XANADU_JSON_INCLUDE AND XANADU_JSON_LIBRARY)
	set(XANADU_JSON_FOUND TRUE)
endif()
