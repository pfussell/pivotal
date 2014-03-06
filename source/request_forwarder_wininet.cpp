#include <sstream>

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
			std::cout << "Warning: attempting to open connection before opening internet" << std::endl;
			return false;
		}

		std::cout << "Opening connection: " << server << ":" << port << std::endl;

		h_connection = InternetConnect(h_internet, to_wide(server).c_str(), port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)nullptr);

		if(h_connection != nullptr)
		{
			BOOL value = true;
			auto success = InternetSetOption(h_connection, 65, &value, sizeof(value)) == TRUE;
			if(success)
			{
				std::cout << "Enabled gzip&deflate decoding" << std::endl;
			}
			else
			{
				std::cout << "Failure when enabling gzip&deflate decoding: " << GetLastError() << std::endl;
			}
		}

		return h_connection != nullptr;
	}

	bool open_request(const wininet_request &request)
	{
		if(h_connection == nullptr)
		{
			std::cout << "Warning: attempting to open request before opening connection" << std::endl;
			return false;
		}

		std::wcout << "Opening request: " << request.method << " " << request.object << " " << request.http_version << std::endl;

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
			std::cout << "Warning: attempting to send request before opening request" << std::endl;
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

		return HttpSendRequest(h_request, headers_string.c_str(), -1L, nullptr, 0) == TRUE;
	}

	response read_response()
	{
		response response;

		DWORD bytes_read = read_buffer_size;
		while (!HttpQueryInfo(h_request, HTTP_QUERY_RAW_HEADERS_CRLF, read_buffer.data(), &bytes_read, NULL))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_INSUFFICIENT_BUFFER)
			{
				throw std::runtime_error("read buffer too small");
			}
			else
			{
				throw std::runtime_error("HttpQueryInfo failed, error = " + std::to_string(GetLastError()));
			}
		}
		std::string headers_string;
		for(int i = 0; i < bytes_read / 2; i++)
		{
			headers_string.append(1, read_buffer[i * 2]);
		}
		headers_string.resize(bytes_read / 2);
		std::stringstream ss(headers_string);
		std::string header_string;
		std::getline(ss, header_string, '/');
		assert(header_string == "HTTP");
		std::getline(ss, header_string, '.');
		response.http_version.major = std::stoi(header_string);
		std::getline(ss, header_string, ' ');
		response.http_version.minor = std::stoi(header_string);
		std::getline(ss, header_string, ' ');
		response.status = static_cast<response::status_type>(std::stoi(header_string));
		std::getline(ss, header_string, '\r');
		while(ss.good())
		{
			std::getline(ss, header_string, '\r');
			assert(header_string[0] = '\n');
			if(header_string.length() > 1)
			{
				auto colon_position = header_string.find(':');
				if(colon_position == std::string::npos)
				{
					throw std::runtime_error("bad header: " + header_string);
				}
				header header;
				header.name = std::string(header_string.begin() + 1, header_string.begin() + colon_position);
				auto header_value_start = header_string.find_first_not_of(' ', colon_position + 1);
				header.value = std::string(header_string.begin() + header_value_start , header_string.end());
				if(header.name != "Transfer-Encoding")
				{
					response.headers.push_back(header);
				}
			}
		}

		bool keep_reading;

		try
		{
			do
			{
				keep_reading = InternetReadFile(h_request, read_buffer.data(), read_buffer.size(), &bytes_read);
				response.content.append(std::string(read_buffer.data(), bytes_read));
			} while(keep_reading && bytes_read > 0);
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
	        return response::stock_response(response::status_type::not_found);
	    }
	}

	if(impl_->h_request == nullptr)
	{
	    wininet_request w_request;
	    w_request.method = std::wstring(request.method.begin(), request.method.end());
	    w_request.object = std::wstring(request.object.begin(), request.object.end());
		std::string accept = request.has_header("Accept") ? request.get_header("Accept") : "";
		w_request.accept_types = std::wstring(accept.begin(), accept.end());
		w_request.http_version = std::wstring(L"HTTP/") + to_wide(std::to_string(request.http_version_major)) + L"." + to_wide(std::to_string(request.http_version_minor));
		w_request.using_tls = request.secure;

		if(!impl_->open_request(w_request))
	    {
	        std::cout << "HttpOpenRequest failed with error code " << GetLastError() << std::endl;
	        return response::stock_response(response::status_type::not_found);
	    }
	}

	if(!impl_->send_request(request.headers))
	{
		std::cout << "HttpSendRequest failed with error code " << GetLastError() << std::endl;
		impl_->close_request();
        return response::stock_response(response::status_type::not_found);
	}

	response = impl_->read_response();

	std::cout << response.to_string(1,1) << std::endl;

	impl_->close_request();

	return response;
}

} // namespace http
} // namespace pivotal
