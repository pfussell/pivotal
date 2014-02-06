#include <signal.h>
#include <utility>
#include <boost/bind.hpp>

#include "mitm_proxy.h"

namespace pivotal {
namespace server {

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

class session
{
public:
  session(boost::asio::io_service& io_service,
      boost::asio::ssl::context& context)
    : socket_(io_service, context)
  {
  }

  ssl_socket::lowest_layer_type& socket()
  {
    return socket_.lowest_layer();
  }

  void start()
  {
    socket_.async_handshake(boost::asio::ssl::stream_base::server,
        boost::bind(&session::handle_handshake, this,
          boost::asio::placeholders::error));
  }

  void handle_handshake(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
		throw error;
      delete this;
    }
  }

  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      boost::asio::async_write(socket_,
          boost::asio::buffer(data_, bytes_transferred),
          boost::bind(&session::handle_write, this,
            boost::asio::placeholders::error));
    }
    else
    {
      delete this;
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      delete this;
    }
  }

private:
  ssl_socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  request_handler request_handler_;
};

mitm_proxy::mitm_proxy(const std::string &address, int port)
	: io_service_(),
      signals_(io_service_),
      acceptor_(io_service_),
      connection_manager_(),
	  context_(boost::asio::ssl::context::sslv3)
{
	context_.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);
    context_.set_password_callback(boost::bind(&mitm_proxy::get_password, this));
    context_.use_certificate_chain_file("C:\\Users\\Thomas\\Downloads\\openssl-0.9.8k_X64\\bin\\certificate.crt");
    context_.use_private_key_file("C:\\Users\\Thomas\\Downloads\\openssl-0.9.8k_X64\\bin\\privateKey.key", boost::asio::ssl::context::pem);
    context_.use_tmp_dh_file("C:\\Users\\Thomas\\Downloads\\openssl-0.9.8k_X64\\bin\\dh512.pem");

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

void mitm_proxy::do_accept()
{
	session* new_session = new session(io_service_, context_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&mitm_proxy::handle_accept, this, new_session,
          boost::asio::placeholders::error));

	/*
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
		*/
}

void mitm_proxy::handle_accept(session* new_session, const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
    }
    else
    {
      delete new_session;
    }

    do_accept();
  }

void mitm_proxy::do_await_stop()
{
	signals_.async_wait(
		[this](boost::system::error_code /*ec*/, int /*signo*/)
		{
			acceptor_.close();
			connection_manager_.stop_all();
		});
}

void mitm_proxy::run()
{
	io_service_.run();
}

} // namespace server
} // namespace pivotal
