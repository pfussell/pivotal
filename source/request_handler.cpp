#include <array>
#include <fstream>
#include <sstream>
#include <string>

#include "../constants.h"
#include "response.h"
#include "request.h"
#include "request_handler.h"

typedef std::array<uint8_t, buffer_size> buffer;

namespace pivotal {
namespace server {

request_handler::request_handler() :
	h_internet(NULL),
	h_connection(NULL),
	h_request(NULL)
{
}

LPCTSTR *request_handler::parse_accept_types(const std::string &accept)
{
	std::wstring s(accept.begin(), accept.end());
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

bool request_handler::open_internet(const std::string &user_agent, bool prevent_proxy)
{
	std::wstring user_agent_wide(user_agent.begin(), user_agent.end());
	DWORD access_type = prevent_proxy ?  INTERNET_OPEN_TYPE_DIRECT : INTERNET_OPEN_TYPE_PRECONFIG;
	h_internet = InternetOpen(user_agent_wide.c_str(), access_type, NULL, NULL, 0);

	return h_internet != NULL;
}

bool request_handler::open_connection(const std::string &server, int port)
{
	assert(h_internet != NULL);
	std::wstring server_wide(server.begin(), server.end());
	if(port != 80 && port != 443)
	{
		std::cout << "non-standard ports are not supported, continue with caution..." << std::endl;
	}
	h_connection = InternetConnect(h_internet, server_wide.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);

	return h_connection != NULL;
}

bool request_handler::tunnel_request(const std::array<char, buffer_size> &request, std::array<char, buffer_size> &response)
{
	// HTTP1.1 200 Connection established

	// client hello

	// server hello

	// server certificate

	// server key exchange

	// server hello done

	// client key exchange

	// client change cipher spec

	// client finished

	// server change cipher spec

	// server finished

	return false;
}

bool request_handler::open_request(const std::string &method, const std::string &object, http_version http_version, 
				  const std::string &accept, bool secure, bool allow_invalid_cert, bool allow_https_redirect, 
				  bool allow_http_redirect)
{
    LPCTSTR *accept_types_array = parse_accept_types(accept);

	int flags = 0;
	if(allow_invalid_cert)
	{
		flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
		flags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
	}
	if(secure && allow_http_redirect)
	{
		flags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP;
	}
	if(!secure && allow_https_redirect)
	{
		flags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
	}
	if(secure)
	{
		flags |= INTERNET_FLAG_SECURE;
	}

	std::wstring method_wide(method.begin(), method.end());
	std::wstring object_wide(object.begin(), object.end());
	std::wstring http_version_string = http_version == http_version::http11 ? L"HTTP/1.1" : L"HTTP/1.0";

	h_request = HttpOpenRequest(h_connection, method_wide.c_str(), object_wide.c_str(), http_version_string.c_str(), NULL, accept_types_array, flags, 0);

	return h_request != NULL;
}

bool request_handler::send_request(const std::vector<header> &headers)
{
	std::wstring headers_string;
	for(auto header : headers)
	{
		headers_string.append(std::wstring(header.name.begin(), header.name.end()));
		headers_string.append(L": ");
		headers_string.append(std::wstring(header.value.begin(), header.value.end()));
		headers_string.append(L"\r\n");
	}
	headers_string.append(L"\r\n");

	return HttpSendRequest(h_request, headers_string.c_str(), -1L, NULL, 0) == TRUE;
}

bool request_handler::write_response(response &response)
{
	const int nBuffSize = 1024;
	char buff[nBuffSize];

	BOOL bKeepReading = true;
	DWORD dwBytesRead = -1;

	try
	{
		while(bKeepReading && dwBytesRead!=0)
		{
			bKeepReading = InternetReadFile(h_request, buff, nBuffSize, &dwBytesRead);
			response.content.append(std::string(buff, dwBytesRead));
		}
	}
	catch(std::exception e)
	{
		return false;
	}

	return true;
}

bool request_handler::handle_request(const request& request, response& response)
{
    response.status = response::ok;

	auto user_agent = request.has_header("User-Agent") ? request.get_header("User-Agent") : "Mozilla/4.0";

    if(!open_internet(user_agent, true))
    {
        std::cout << "InternetOpen failed with error code " << GetLastError() << std::endl;
        return false;
    }

	if(!open_connection(request.server, request.port))
    {
        std::cout << "InternetConnect failed with error code " << GetLastError() << std::endl;
		close_internet();
        return false;
    }

	http_version http_version = request.http_version_minor == 1 ? http_version::http11 : http_version::http10;
	std::string accept = request.has_header("Accept") ? request.get_header("Accept") : "";

	if(!open_request(request.method, request.object, http_version, accept, request.secure))
    {
        std::cout << "HttpOpenRequest failed with error code " << GetLastError() << std::endl;
		close_connection();
		close_internet();
        return false;
    }

	if(request.method == "CONNECT")
	{
		response.status = response::connected;
		return true;
	}
	else
	{
		if(!send_request(request.headers))
		{
			std::cout << "HttpSendRequest failed with error code " << GetLastError() << std::endl;
			close_connection();
			close_internet();
			return false;
		}

		write_response(response);

		close_request();
		close_connection();
		close_internet();
	}

	return false;
}

} // namespace server
} // namespace pivotal
