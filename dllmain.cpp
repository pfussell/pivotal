#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <Windows.h>
#include <WinInet.h>

#include "request.hpp"
#include "request_parser.hpp"

using boost::asio::ip::tcp;

boost::asio::io_service io_service;

LPCTSTR *build_accept_types(const std::wstring &s)
{
	std::vector<std::wstring> accept_types_split;
	std::wstringstream ss(s);
    std::wstring accept_type;

	while(std::getline(ss, accept_type, L','))
	{
		accept_types_split.push_back(accept_type);
    }

	LPCTSTR *accept_types_array = new LPCTSTR[accept_types_split.size() + 1];
	for(int i = 0; i < accept_types_split.size(); i++)
	{
		accept_types_array[i] = accept_types_split[i].c_str();
	}
	accept_types_array[accept_types_split.size()] = NULL;

	return accept_types_array;
}

std::tuple<std::wstring, std::wstring, int, std::wstring> parse_uri(const std::string &uri)
{
	std::wstring scheme;
	std::wstring domain;
	int port = 80;
	std::wstring resource = L"/";

	auto scheme_separator = std::find(uri.begin(), uri.end(), ':');
	scheme = std::wstring(uri.begin(), scheme_separator);

	if(!(scheme == L"http" || scheme == L"https" || scheme == L"ftp"))
	{
		scheme = L"guess";
		auto domain_separator = std::find(uri.begin(), uri.end(), '/');
		domain = std::wstring(uri.begin(), domain_separator);

		if(domain_separator != uri.end())
		{
			resource = std::wstring(domain_separator, uri.end());
		}
	}
	else
	{
		auto domain_start = scheme_separator + 3;
		auto domain_separator = std::find(domain_start, uri.end(), '/');
		domain = std::wstring(domain_start, domain_separator);

		if(domain_separator != uri.end())
		{
			resource = std::wstring(domain_separator, uri.end());
		}
	}

	auto port_separator = std::find(domain.begin(), domain.end(), ':');
	if(port_separator != domain.end())
	{
		std::wstring port_string = std::wstring(port_separator + 1, domain.end());
		domain = std::wstring(domain.begin(), port_separator);
		if(!port_string.empty() && std::find_if(port_string.begin(), port_string.end(), [](char c) { return !std::isdigit(c); }) == port_string.end())
		{
			port = std::stoi(port_string);
		}
	}

	if(scheme == L"guess")
	{
		scheme = port == 80 ? L"http" : L"https";
	}

	return std::make_tuple(scheme, domain, port, resource);
}

void handle_request(boost::shared_ptr<tcp::socket> local_socket, http::server::request &request)
{
	try
	{
		boost::asio::streambuf remote_response;
		std::ostream response_stream(&remote_response);

		std::wstring user_agent = request.get_header_wide("User-Agent");
		if(user_agent == L"")
		{
			user_agent = L"Mozilla/4.0";
		}
		HINTERNET hInternet = InternetOpen(user_agent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		if(hInternet == NULL)
		{
			std::cout << "InternetOpen failed with error code " << GetLastError() << std::endl;
			return;
		}

		auto uri = parse_uri(request.uri);
		std::cout << request.method << " " << std::string(std::get<0>(uri).begin(), std::get<0>(uri).end()) << "://" << std::string(std::get<1>(uri).begin(), std::get<1>(uri).end()) << ":" << std::get<2>(uri) << std::string(std::get<3>(uri).begin(), std::get<3>(uri).end()) << std::endl;
		if(std::get<0>(uri) == L"https")
		{
			std::cout << "Error: https isn't supported yet, aborting connection" << std::endl;
			return;
		}
		HINTERNET hConnect = InternetConnect(hInternet, std::get<1>(uri).c_str(), std::get<2>(uri), NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
		if(hConnect == NULL)
		{
			std::cout << "InternetConnect failed with error code " << GetLastError() << std::endl;
			InternetCloseHandle(hInternet);
			return;
		}

		std::wstring accept_types_string = request.get_header_wide("Accept");
		LPCTSTR *accept_types_array = build_accept_types(accept_types_string);
		HINTERNET hRequest = HttpOpenRequest(hConnect, NULL, std::get<3>(uri).c_str(), NULL, NULL, accept_types_array, 0, 0);

		if(hRequest==NULL)
		{
			std::cout << "HttpOpenRequest failed with error code " << GetLastError() << std::endl;
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			return;
		}

		BOOL bRequestSent = HttpSendRequest(hRequest, NULL, 0, NULL, 0);

		if(!bRequestSent)
		{
			std::cout << "HttpSendRequest failed with error code " << GetLastError() << std::endl;
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			return;
		}

		const int nBuffSize = 1024;
		char buff[nBuffSize];

		BOOL bKeepReading = true;
		DWORD dwBytesRead = -1;

		while(bKeepReading && dwBytesRead!=0)
		{
			bKeepReading = InternetReadFile(hRequest, buff, nBuffSize, &dwBytesRead);
			response_stream << std::string(buff, dwBytesRead);
			boost::asio::write(*local_socket,  remote_response, boost::asio::transfer_all());
		}

		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
	}
	catch(std::exception &e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
}

void start_proxy(int port)
{
	std::cout << "pivotal started on port " << port << std::endl;

	try
	{
		auto ip = boost::asio::ip::address_v4(2130706433); //127.0.0.1
		tcp::acceptor acceptor(io_service, tcp::endpoint(ip, port));

		while(true)
		{
			boost::shared_ptr< tcp::socket > socket( new tcp::socket(io_service) );

			acceptor.accept(*socket);

			std::string Request, Host;
			boost::asio::streambuf response;

			std::istream is( &response );

			boost::asio::read( *socket, response, boost::asio::transfer_at_least(1) );

			std::string line;

			while(is.good())
			{
				getline(is, line);
				Request += line + "\n";
			}

			http::server::request_parser request_parser;
			http::server::request request;
			request_parser.parse(request, Request.begin(), Request.end());

			handle_request(socket, request); 
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
		std::cout << "restarting pivotal" << std::endl;
	}
}

DWORD ThreadProc(LPVOID whatever)
{
	Sleep(5000);

	while(true)
	{
		start_proxy(4040);
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