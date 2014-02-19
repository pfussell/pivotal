#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <botan/tls_server.h>
#include <botan/x509cert.h>
#include <botan/pkcs8.h>
#include <botan/auto_rng.h>

#include "request_parser.h"
#include "request_forwarder.h"

namespace pivotal {
namespace http {

struct response;

class session : public boost::enable_shared_from_this<session>
{
public:
    typedef boost::shared_ptr<session> pointer;

    static pointer create(boost::asio::io_service &io_service,
			  Botan::TLS::Session_Manager &session_manager,
			  Botan::Credentials_Manager &credentials,
			  Botan::TLS::Policy &policy,
			  Botan::RandomNumberGenerator &rng);

    boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

    void start();

    void stop();

private:
    session(boost::asio::io_service& io_service,
		       Botan::TLS::Session_Manager& session_manager,
		       Botan::Credentials_Manager& credentials,
		       Botan::TLS::Policy& policy,
		       Botan::RandomNumberGenerator& rng);

    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

    void handle_write(const boost::system::error_code& error);

    void tls_output_wanted(const Botan::byte buf[], size_t buf_len);

    void tls_alert_cb(Botan::TLS::Alert alert, const Botan::byte[], size_t);

    void tls_data_recv(const Botan::byte buf[], size_t buf_len);

    void handle_connection_established(const boost::system::error_code& error);

    void write_connection_established_response();

    void write_response_plaintext(const response &response);

    void write_response_encrypted(const response &response);

    bool tls_handshake_complete(const Botan::TLS::Session& session);

    boost::asio::io_service::strand m_strand; // serialization

    boost::asio::ip::tcp::socket m_socket;

    Botan::TLS::Server m_tls;
    
	std::string m_hostname;

    std::array<unsigned char, 4096> m_read_buf;;

    // used to hold the data currently being written by the system
    std::vector<Botan::byte> m_write_buf;

    // used to hold data queued for writing
    std::vector<Botan::byte> m_outbox;

    bool tunnel_established;

    request_parser request_parser_;

    request_forwarder request_forwarder_;

    request request_;
};

} // namespace http
} // namespace pivotal
