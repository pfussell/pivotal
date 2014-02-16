#include "request_forwarder.h"
#include "request.h"

#include <Windows.h>
#include <WinInet.h>

namespace {

std::wstring to_wide(const std::string &s)
{
	return std::wstring(s.begin(), s.end());
}

struct wininet_request
{
	bool using_tls = false;
	bool allow_invalid_cert = true;
	bool allow_https_redirect = true;
	bool allow_http_redirect = true;
	std::wstring method;
	std::wstring object;
	std::wstring http_version;
	std::wstring accept_types;
};

DWORD build_request_flags(const wininet_request &request)
{
	DWORD flags = 0;

	if(request.allow_invalid_cert)
	{
		flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
		flags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
	}

	if(request.allow_http_redirect)
	{
		flags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP;
	}

	if(request.allow_https_redirect)
	{
		flags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
	}

	if(request.using_tls)
	{
		flags |= INTERNET_FLAG_SECURE;
	}

	return flags;
}

template<typename T>
static std::vector<T> split_string(const T &string, wchar_t delimiter)
{
    std::vector<T> split;
    std::wstringstream ss(string);
    T accept_type;

    while(std::getline(ss, accept_type, delimiter))
    {
        split.push_back(accept_type);
    }

    return split;
}

} // namespace

namespace pivotal {
namespace http {

struct request_forwarder_impl
{
	request_forwarder_impl() = default;

	~request_forwarder_impl()
	{
		close_all();
	}

   	bool open_internet(const std::string &user_agent, bool prevent_proxy=true)
   	{
		DWORD access_type = prevent_proxy ? INTERNET_OPEN_TYPE_DIRECT : INTERNET_OPEN_TYPE_PRECONFIG;

		h_internet = InternetOpen(to_wide(user_agent).c_str(), access_type, nullptr, nullptr, 0);

		return h_internet != nullptr;
   	}

	bool open_connection(const std::string &server, int port)
	{
		if(h_internet == nullptr)
		{
			return false;
		}

		INTERNET_PORT p;

		switch(port)
		{
		case 80:
			p = INTERNET_DEFAULT_HTTP_PORT;
			break;
		case 443:
			p = INTERNET_DEFAULT_HTTPS_PORT;
			break;
		default:
			throw std::runtime_error("unsupported port: " + std::to_string(port));
		}

		h_connection = InternetConnect(h_internet, to_wide(server).c_str(), p, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)nullptr);

		return h_connection != nullptr;
	}

	bool open_request(const wininet_request &request)
	{
	    auto accept_types = split_string(request.accept_types, L',');
	    std::vector<LPCWSTR> accept_types_pointers(accept_types.size() + 1, nullptr);
	    auto get_string_pointer = [](const std::wstring &s) { return s.c_str(); };
	    std::transform(accept_types.begin(), accept_types.end(), accept_types_pointers.begin(), get_string_pointer);

	    auto flags = build_request_flags(request);

		h_request = HttpOpenRequest(h_connection, request.method.c_str(), request.object.c_str(), request.http_version.c_str(), nullptr, accept_types_pointers.data(), flags, 0);

		return h_request != nullptr;
	}

	bool send_request(const std::vector<header> &headers)
	{
		if(h_request == nullptr)
		{
			return false;
		}

		std::wstring headers_string;
		for(auto header : headers)
		{
			headers_string.append(std::wstring(header.name.begin(), header.name.end()));
			headers_string.append(L": ");
			headers_string.append(std::wstring(header.value.begin(), header.value.end()));
			headers_string.append(L"\r\n");
		}
		headers_string.append(L"\r\n");
		headers_string.append(1, L'\0');

		return HttpSendRequest(h_request, headers_string.c_str(), -1L, nullptr, 0) == TRUE;
	}

	response read_response()
	{
		response response;
		bool keep_reading;
		DWORD bytes_read;

		try
		{
			do
			{
				keep_reading = InternetReadFile(h_request, read_buffer.data(), read_buffer.size(), &bytes_read);
				response.content.insert(response.content.end(), read_buffer.begin(), read_buffer.begin() + bytes_read);
			} while(keep_reading && bytes_read > 0);
			std::cout << "response returned through wininet: " << response.content << std::endl;
		}
		catch(std::exception e)
		{
			std::cout << "read response failed" << std::cout;
			response = response::stock_response(response::status_type::internal_server_error);
		}

		return response;
	}

	void close_all()
	{
		close_request();
		close_connection();
		close_internet();
	}

	void close_request()
	{ 
		if(h_request != nullptr)
		{
			InternetCloseHandle(h_request);
		}

		h_request = nullptr;
	}

	void close_connection()
	{
		if(h_connection != nullptr)
		{
			InternetCloseHandle(h_connection);
		}

		h_connection = nullptr;
	}

	void close_internet()
	{
		if(h_internet != nullptr)
		{
			InternetCloseHandle(h_internet);
		}

		h_internet = nullptr;
	}

	HINTERNET h_request = nullptr;
	HINTERNET h_connection = nullptr;
	HINTERNET h_internet = nullptr;

	static const size_t read_buffer_size = 2048;

	std::array<char, read_buffer_size> read_buffer;
};

request_forwarder::request_forwarder() : impl_(new request_forwarder_impl())
{

}

request_forwarder::~request_forwarder()
{
	delete impl_;
}

response request_forwarder::forward_request(const request& request)
{
	response response;
    response.status = response::ok;

    if(impl_->h_internet == nullptr)
    {
		auto user_agent = request.has_header("User-Agent") ? request.get_header("User-Agent") : "Mozilla/4.0";

	    if(!impl_->open_internet(user_agent, true))
	    {
	        std::cout << "InternetOpen failed with error code " << GetLastError() << std::endl;
	        return response::stock_response(response::status_type::not_found);
	    }
	}

	if(impl_->h_connection == nullptr)
	{
		std::cout << "opening connection: " << request.server << " " << request.port << std::endl;
		if(!impl_->open_connection(request.server, request.port))
	    {
	        std::cout << "InternetConnect failed with error code " << GetLastError() << std::endl;
			impl_->close_internet();
	        return response::stock_response(response::status_type::not_found);
	    }
	}

	if(impl_->h_request == nullptr)
	{
	    wininet_request w_request;
	    w_request.method = std::wstring(request.method.begin(), request.method.end());
		std::string accept = request.has_header("Accept") ? request.get_header("Accept") : "";
		w_request.using_tls = request.secure;

		if(!impl_->open_request(w_request))
	    {
	        std::cout << "HttpOpenRequest failed with error code " << GetLastError() << std::endl;
			impl_->close_connection();
			impl_->close_internet();
	        return response::stock_response(response::status_type::not_found);
	    }
	}

	if(!impl_->send_request(request.headers))
	{
		std::cout << "HttpSendRequest failed with error code " << GetLastError() << std::endl;
		impl_->close_connection();
		impl_->close_internet();
        return response::stock_response(response::status_type::not_found);
	}

	response = impl_->read_response();

	return response;
}

} // namespace http
} // namespace pivotal
