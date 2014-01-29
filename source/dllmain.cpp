#include <iostream>
#include <Windows.h>

#include "proxy.H"

DWORD ThreadProc(LPVOID whatever)
{
	Sleep(5000);

	proxy p("0.0.0.0", 4040);
	p.run();

	return 0;
}

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{
	HANDLE hThread = NULL;

	switch (nReason) 
	{
	case DLL_PROCESS_ATTACH:
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadProc, (LPVOID)NULL, 0, NULL);
		break;
	}

	return hThread != NULL;
}