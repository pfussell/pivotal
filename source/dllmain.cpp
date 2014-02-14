#include <iostream>
#include <thread>

#include "server.h"

// Include this last for now
#include <Windows.h>

size_t choose_thread_count()
{
    size_t result = std::thread::hardware_concurrency();
    
    if(result)
    {
	return result;
    }
    
    return 1;
}

DWORD ThreadProc(LPVOID whatever)
{
    // Let some things happen, not sure why, but this seems to be important
    Sleep(5000);

    try
    {
	boost::asio::io_service io_service;
	
	const unsigned short port = 4434;
	pivotal::http::server server(io_service, port);
	
	size_t num_threads = choose_thread_count();
	std::cout << "Starting pivotal proxy server using " << num_threads << " threads\n";
	
	std::vector<std::shared_ptr<std::thread>> threads;
	for(size_t i = 0; i != num_threads; ++i)
	{
	    threads.push_back(std::shared_ptr<std::thread>(new std::thread([&]() { io_service.run(); })));
	}
	
	// Wait for all threads in the pool to exit.
	for (size_t i = 0; i < threads.size(); ++i)
	{
	    threads[i]->join();
	}
    }
    catch (std::exception& e)
    {
	std::cerr << e.what() << std::endl;
    }
    
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
