#include <iostream>

#include "server/mitm_proxy.h"

#include <Windows.h>

DWORD ThreadProc(LPVOID whatever)
{
	Sleep(5000);

	pivotal::server::mitm_proxy p("127.0.0.1", 4040);
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