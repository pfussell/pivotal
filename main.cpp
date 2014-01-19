#include "ProxyServer.h"

int main(int argc, char* argv[])
{
try
{
boost::asio::io_service io_service;

using namespace std; // For atoi.
server s(io_service, atoi(argv[1]));

io_service.run();
}
catch (std::exception& e)
{
std::cerr << "Exception: " << e.what() << "\n";
}

return 0;
}
