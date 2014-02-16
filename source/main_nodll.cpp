#include <thread>
#include <vector>
#include <boost/asio.hpp>

#include "server.h"

size_t choose_thread_count()
{
    size_t result = std::thread::hardware_concurrency();
    
    if(result)
    {
		return result;
    }
    
    return 1;
}

int main()
{
    try
    {
		boost::asio::io_service io_service;
		
		pivotal::http::server server(io_service, "0.0.0.0", "4040");
		
        bool threaded = false;
        
        if(threaded)
        {
            size_t num_threads = 1;//choose_thread_count();
            std::cout << "Starting pivotal proxy server using " << num_threads << " threads" << std::endl;
            
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
        else
        {
            std::cout << "Starting pivotal proxy server" << std::endl;
            io_service.run();
        }

		std::cout << "server stopped" << std::endl;
    }
    catch (std::exception& e)
    {
		std::cerr << "Server error: " << e.what() << std::endl;
    }
    
    return 0;
}