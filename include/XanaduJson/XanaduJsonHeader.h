#ifndef			_XANADU_JSON_HEADER_H_
#define			_XANADU_JSON_HEADER_H_

#include <XanaduRuntime/XanaduRuntime.h>
#include <XanaduCore/XanaduCore.h>

#ifndef			XANADU_JSON_BUILD_STATIC
#ifdef			XANADU_JSON_LIB
#ifdef XANADU_SYSTEM_WINDOWS
#define			XANADU_JSON_EXPORT					__declspec(dllexport)
#else
#define			XANADU_JSON_EXPORT					__attribute__((visibility("default")))
#endif // XANADU_SYSTEM_WINDOWS
#else
#ifdef XANADU_SYSTEM_WINDOWS
#define			XANADU_JSON_EXPORT					__declspec(dllimport)
#else
#define			XANADU_JSON_EXPORT					__attribute__((visibility("default")))
#endif // XANADU_SYSTEM_WINDOWS
#endif // XANADU_JSON_LIB
#else
#define			XANADU_JSON_EXPORT
#endif // XANADU_JSON_BUILD_STATIC
#define			XANADU_JSON_LOCAL

#endif // _XANADU_JSON_HEADER_H_
