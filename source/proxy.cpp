#include <signal.h>
#include <utility>
#include <boost/bind.hpp>

#include "proxy.h"

namespace pivotal {

proxy::proxy(const std::string &address, int port)
	: io_service_(),
      signals_(io_service_),
      acceptor_(io_service_),
      connection_manager_(),
      socket_(io_service_),
      request_handler_()
{
	signals_.add(SIGINT);
	signals_.add(SIGTERM);
#if defined(SIGQUIT)
	signals_.add(SIGQUIT);
#endif

	do_await_stop();

	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, std::to_string(port));
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	do_accept();
}

void proxy::do_accept()
{
	acceptor_.async_accept(socket_,
		[this](boost::system::error_code ec)
		{
			if (!acceptor_.is_open())
			{
				return;
			}

			if (!ec)
			{
				connection_manager_.start(std::make_shared<connection>(
				std::move(socket_), connection_manager_, request_handler_));
			}

			do_accept();
		});
}

void proxy::do_await_stop()
{
	signals_.async_wait(
		[this](boost::system::error_code /*ec*/, int /*signo*/)
		{
			acceptor_.close();
			connection_manager_.stop_all();
		});
}

} // namespace pivotal