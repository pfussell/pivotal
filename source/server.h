#pragma once

#include <boost/asio.hpp>
#include <string>

#include "credentials.h"
#include "connection.h"
#include "connection_manager.h"
#include "request_forwarder.h"
#include "session.h"
#include "standard_policy.h"

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
    explicit server(boost::asio::io_service& io_service, const std::string& address = "0.0.0.0", const std::string& port = "4040");

private:
    session::pointer make_session();

    void handle_accept(session::pointer new_session, const boost::system::error_code& error);

    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service &io_service_;

    /// The signal_set is used to register for process termination notifications.
    boost::asio::signal_set signals_;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor_;

    Botan::AutoSeeded_RNG rng_;

    Botan::TLS::Session_Manager_In_Memory session_manager_;

    standard_policy policy_;

    simple_credentials_manager credentials_;

    request_forwarder forwarder_;
};

} // namespace http
} // namespace pivotal
