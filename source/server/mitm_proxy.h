#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace pivotal {
namespace server {

class session;

class mitm_proxy
{
public:
	explicit mitm_proxy(const std::string &address, int port);

	void run();

private:
	void do_accept();

	void handle_accept(session *new_session, const boost::system::error_code &error);

	void do_await_stop();

	std::string get_password() const
  {
    return "thomas";
  }

	boost::asio::io_service io_service_;

	boost::asio::signal_set signals_;

	boost::asio::ip::tcp::acceptor acceptor_;

	connection_manager connection_manager_;

	boost::asio::ssl::context context_;
};

} // namespace server
} // namespace pivotal