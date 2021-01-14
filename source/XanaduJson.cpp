//
// Created by Administrator on 2021/1/14.
//

#include <XanaduJson/XanaduJson.h>

bool XANADUAPI Xanadu_Json_Initialize() XANADU_NOTHROW
{
return true;
};

void XANADUAPI Xanadu_Json_Release() XANADU_NOTHROW
{
};


#ifdef XANADU_SYSTEM_WINDOWS
extern "C" BOOL WINAPI DllMain(HANDLE _HDllHandle, DWORD _Reason, LPVOID _Reserved)
{
	XANADU_UNPARAMETER(_HDllHandle);
	XANADU_UNPARAMETER(_Reserved);

	switch(_Reason)
	{
		case DLL_PROCESS_ATTACH:
			Xanadu_Json_Initialize();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			Xanadu_Json_Release();
			break;
		default:
			break;
	}
	return TRUE;
}
#else
__attribute((constructor)) void Xanadu_Json_Library_Init(void)
{
	Xanadu_Json_Initialize();
};

__attribute((destructor)) void Xanadu_Json_Library_Fini(void)
{
	Xanadu_Json_Release();
};
#endif//XANADU_SYSTEM_WINDOWS

