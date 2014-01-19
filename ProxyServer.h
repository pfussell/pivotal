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
	static ProxyServer ConstructProxy(int port);

	void Start();

private:
	ProxyServer(int port);

	std::string GetPassword() const;

	void StartAccept();

	void HandleAccept(session *new_session, const boost::system::error_code &error);

	Response SendRequest(const Request &request);

private:
	boost::asio::io_service io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ssl::context context_;
};