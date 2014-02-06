#pragma once

#include <string>

#include "request.h"

#include <Windows.h>
#include <WinInet.h>

namespace pivotal {
namespace server {

struct response;

class request_handler
{
public:
    request_handler();

    bool handle_request(const request &req, response &rep);

	bool tunnel_request(const std::array<char, buffer_size> &request, std::array<char, buffer_size> &response);

private:
	bool open_internet(const std::string &user_agent, bool prevent_proxy=true);

	bool open_connection(const std::string &server, int port);

	bool open_request(const std::string &method, const std::string &object, http_version http_version, 
		const std::string &accept, bool secure, bool allow_invalid_cert=true, bool allow_https_redirect=true, 
		bool allow_http_redirect=true);

	bool send_request(const std::vector<header> &headers);

	bool write_response(response &response);

	void close_request() { InternetCloseHandle(h_request); }

	void close_connection() { InternetCloseHandle(h_connection); }

	void close_internet() { InternetCloseHandle(h_internet); }

	static LPCTSTR *parse_accept_types(const std::string &accept_types_string);

    HINTERNET h_request, h_connection, h_internet;
};

} // namespace server
} // namespace pivotal
