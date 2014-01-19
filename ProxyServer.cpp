#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "Request.h"
#include "Response.h"
#include "Session.h"

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

class ProxyServer
{
public:
	ProxyServer(boost::asio::io_service& io_service, int port)
	: io_service_(io_service), acceptor_(io_service,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	context_(boost::asio::ssl::context::sslv23)
	{
		context_.set_options(
			boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::no_sslv2
			| boost::asio::ssl::context::single_dh_use);
		context_.set_password_callback(boost::bind(&server::get_password, this));
		context_.use_certificate_chain_file("server.pem");
		context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);
		context_.use_tmp_dh_file("dh512.pem");

		start_accept();
	}

	std::string get_password() const
	{
		return "test";
	}

	void start_accept()
	{
		session* new_session = new session(io_service_, context_);
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	}

	void handle_accept(session* new_session, const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}
		else
		{
			delete new_session;
		}

		start_accept();
	}

	Response SendRequest(const Request &request)
	{
		Response response;

		HINTERNET hInternet = InternetOpen(L"pivotal", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if(NULL != hInternet)
		{
			HINTERNET hConnection = InternetConnect(hInternet, request.host_name.c_str(), INTERNET_DEFAULT_HTTPS_PORT, L"", L"", INTERNET_SERVICE_HTTP, 0, NULL);
			if(NULL != hConnection)
			{
				const wchar_t *lplpszAcceptTypes[] = new wchar_t *[request.headers[L"Accept"].size() + 1];
				for(int i = 0; i < request.headers[L"Accept"].size())
				{
					lplpszAcceptTypes[i] = i == request.headers[L"Accept"].size() ? NULL : request.headers[L"Accept"][i].c_str();
				}

				HINTERNET hRequest = HttpOpenRequest(hConnection, request.method.c_str(), request.target.c_str(), NULL, request.referer.c_str(),
					lplpszAcceptTypes, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
					INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS | INTERNET_FLAG_KEEP_CONNECTION |
					INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_UI | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
				if(NULL != hRequest)
				{
					BOOL rc = HttpSendRequest(hRequest, request.headers.c_str(), request.headers.size(), (void*)request.data.c_str(), request.data.size());

	                if(rc) // here rc = 0 and GetLastError() returns 12152
	                {
	                	DWORD availDataLen;
	                	char buffer[4096];
	                	DWORD readCount = ERROR_INTERNET_CONNECTION_RESET;

	                	InternetQueryDataAvailable(hInternet, &availDataLen, 0, 0);
	                	while(0 < availDataLen)
	                	{
	                		InternetReadFile(hInternet, buffer, std::min(sizeof(buffer), availDataLen), &readCount);
	                		availDataLen -= readCount;
	                		retVal += std::string(buffer, readCount);
	                	}
	                }
	                InternetCloseHandle(hRequest);
	            }
	            delete[] lplpszAcceptTypes;

	            InternetCloseHandle(hConnection);
	        }
	        InternetCloseHandle(hInternet);
	    }
	    return retVal;
	}

private:
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ssl::context context_;
};