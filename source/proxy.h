#pragma once

#include <string>
#include <boost/asio.hpp>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace pivotal {

class proxy
{
public:
	explicit proxy(const std::string &address, int port);

	void run();

private:
	void do_accept();

	void do_await_stop();

	boost::asio::io_service io_service_;

	boost::asio::signal_set signals_;

	boost::asio::ip::tcp::acceptor acceptor_;

	connection_manager connection_manager_;

	boost::asio::ip::tcp::socket socket_;

	request_handler request_handler_;
};

} // namespace pivotal