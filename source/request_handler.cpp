#include <fstream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <WinInet.h>

#include "request_handler.h"
#include "mime_types.h"
#include "response.h"
#include "request.h"

namespace pivotal {

request_handler::request_handler()
{
}

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

void request_handler::handle_request(const request& req, response& rep)
{
// Decode url to path.
    std::string request_path;
    if (!url_decode(req.uri, request_path))
    {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

// Request path must be absolute and not contain "..".
    if (request_path.empty() || request_path[0] != '/'
        || request_path.find("..") != std::string::npos)
    {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

// If path ends in slash (i.e. is a directory) then add "index.html".
    if (request_path[request_path.size() - 1] == '/')
    {
        request_path += "index.html";
    }

// Determine the file extension.
    std::size_t last_slash_pos = request_path.find_last_of("/");
    std::size_t last_dot_pos = request_path.find_last_of(".");
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
    {
        extension = request_path.substr(last_dot_pos + 1);
    }

// Open the file to send back.
    std::string full_path = doc_root_ + request_path;
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is)
    {
        rep = reply::stock_reply(reply::not_found);
        return;
    }

// Fill out the reply to be sent to the client.
    rep.status = reply::ok;
    char buf[512];
    while (is.read(buf, sizeof(buf)).gcount() > 0)
        rep.content.append(buf, is.gcount());
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = std::to_string(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = mime_types::extension_to_type(extension);
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '%')
        {
            if (i + 3 <= in.size())
            {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value)
                {
                    out += static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if (in[i] == '+')
        {
            out += ' ';
        }
        else
        {
            out += in[i];
        }
    }
    return true;
}

} // namespace pivotal
