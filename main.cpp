#include <fstream>
#include <Windows.h>

DWORD ThreadProc(LPVOID whatever)
{
	Sleep(5000);

	std::fstream f;
	f.open("C:\\Users\\Thomas\\Desktop\\test.txt", std::ios::out);
	f << "hacking into your browser sesssionz...";
	f.close();

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