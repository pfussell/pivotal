#include <signal.h>
#include <utility>

#include "server.h"

namespace pivotal {
namespace http {

server::server(boost::asio::io_service& io_service, const std::string& address, const std::string& port)
	: io_service_(io_service),
	  signals_(io_service),
	  acceptor_(io_service),
	  session_manager_(rng_),
	  credentials_(rng_)
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif
    
    signals_.async_wait(
	[this](boost::system::error_code /*ec*/, int /*signo*/)
	{
	    // The server is stopped by cancelling all outstanding asynchronous
	    // operations. Once all operations have finished the io_service::run()
	    // call will exit.
	    acceptor_.close();
	});
    
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({address, port});
    
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    session::pointer new_session = make_session();
    
    acceptor_.async_accept(new_session->get_socket(),
    	boost::bind(&server::handle_accept,
    		this,
    		new_session,
    		boost::asio::placeholders::error));
}

session::pointer server::make_session()
{
	return session::create(acceptor_.get_io_service(), session_manager_, credentials_, policy_, rng_, forwarder_);
}

void server::handle_accept(session::pointer new_session, const boost::system::error_code& error)
{
	if (!error)
	{
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
            return;
        }

        new_session->start();
        new_session = make_session();

        auto accept_callback = boost::bind(&server::handle_accept, this, new_session, boost::asio::placeholders::error);
        acceptor_.async_accept(new_session->get_socket(), accept_callback);
	}
}

} // namespace http
} // namespace pivotal
