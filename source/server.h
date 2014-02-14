#pragma once

#include <boost/asio.hpp>
#include <string>

#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"

namespace pivotal {
namespace http {

/// The top-level class of the HTTP server.
class server
{
public:
    server(const server&) = delete;
    server& operator=(const server&) = delete;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    explicit server(const std::string& address, const std::string& port);

private:
    /// Perform an asynchronous accept operation.
    void do_accept();

    /// Wait for a request to stop the server.
    void do_await_stop();

    asio_tls_server(boost::asio::io_service& io_service, unsigned short port) :
    m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	m_session_manager(m_rng),
	m_creds(m_rng)
    {
	session::pointer new_session = make_session();

	m_acceptor.async_accept(
            new_session->get_socket(),
            boost::bind(
		&asio_tls_server::handle_accept,
		this,
		new_session,
		boost::asio::placeholders::error)
            );
    }

private:
    session::pointer make_session()
    {
	return session::create(
            m_acceptor.get_io_service(),
            m_session_manager,
            m_creds,
            m_policy,
            m_rng
            );
    }

    void handle_accept(session::pointer new_session,
		       const boost::system::error_code& error)
    {
	if (!error)
	{
            new_session->start();

            new_session = make_session();

            m_acceptor.async_accept(
		new_session->get_socket(),
		boost::bind(
		    &asio_tls_server::handle_accept,
		    this,
		    new_session,
		    boost::asio::placeholders::error)
		);
	}
    }

    boost::asio::ip::tcp::acceptor m_acceptor;

    Botan::AutoSeeded_RNG m_rng;
    Botan::TLS::Session_Manager_In_Memory m_session_manager;
    InsecurePolicy m_policy;
    Credentials_Manager_Simple m_creds;

    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service io_service_;

    /// The signal_set is used to register for process termination notifications.
    boost::asio::signal_set signals_;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor_;

    /// The connection manager which owns all live connections.
    connection_manager connection_manager_;

    /// The next socket to be accepted.
    boost::asio::ip::tcp::socket socket_;

    /// The handler for all incoming requests.
    request_handler request_handler_;
};

} // namespace http
} // namespace pivotal
