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

class tls_server_session : public boost::enable_shared_from_this<tls_server_session>
{
public:
    typedef boost::shared_ptr<tls_server_session> pointer;

    static pointer create(boost::asio::io_service& io_service,
			  Botan::TLS::Session_Manager& session_manager,
			  Botan::Credentials_Manager& credentials,
			  Botan::TLS::Policy& policy,
			  Botan::RandomNumberGenerator& rng)
    {
	auto session = new tls_server_session(io_service, session_manager, credentials, policy, rng);
	return pointer(session);
    }

    boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

    void start()
    {
	m_socket.async_read_some(
            boost::asio::buffer(m_read_buf, sizeof(m_read_buf)),
            m_strand.wrap(
		boost::bind(&tls_server_session::handle_read, shared_from_this(),
			    boost::asio::placeholders::error,
			    boost::asio::placeholders::bytes_transferred)));
    }

    void stop()
    {
	m_socket.close();
    }

private:
    tls_server_session(boost::asio::io_service& io_service,
		       Botan::TLS::Session_Manager& session_manager,
		       Botan::Credentials_Manager& credentials,
		       Botan::TLS::Policy& policy,
		       Botan::RandomNumberGenerator& rng) :
	m_strand(io_service),
	m_socket(io_service),
	m_tls(boost::bind(&tls_server_session::tls_output_wanted, this, _1, _2),
	      boost::bind(&tls_server_session::tls_data_recv, this, _1, _2),
	      boost::bind(&tls_server_session::tls_alert_cb, this, _1, _2, _3),
	      boost::bind(&tls_server_session::tls_handshake_complete, this, _1),
	      session_manager,
	      credentials,
	      policy,
	      rng),
	tunnel_established(false)
    {
    }

    http::response fulfill_request(const http::request &request)
    {
	return http::response();
    }

    void handle_read(const boost::system::error_code& error,
		     size_t bytes_transferred)
    {
	if(!error)
	{
            try
	    {
		if(!tunnel_established)
		{
		    auto request = http::request_parser::parse();//m_read_buf, bytes_transferred);
		    if(request.method == http::request::method::connect)
		    {
			write_connection_established_response();
		    }
		    else
		    {
			auto response = fulfill_request(request);
			write_response_plaintext(response);
		    }
		}
		else
		{
		    m_tls.received_data(m_read_buf, bytes_transferred);
		}
	    }
            catch(std::exception& e)
	    {
		std::cout << "Read failed " << e.what() << "\n";
		stop();
		return;
	    }

            m_socket.async_read_some(
		boost::asio::buffer(m_read_buf, sizeof(m_read_buf)),
		m_strand.wrap(boost::bind(&tls_server_session::handle_read, shared_from_this(),
					  boost::asio::placeholders::error,
					  boost::asio::placeholders::bytes_transferred)));
	}
	else
	{
            stop();
	}
    }

    void handle_write(const boost::system::error_code& error)
    {
	if(!error)
	{
            m_write_buf.clear();

            // initiate another write if needed
            tls_output_wanted(nullptr, 0);
	}
	else
	{
            stop();
	}
    }

    void tls_output_wanted(const Botan::byte buf[], size_t buf_len)
    {
	if(buf_len > 0)
	{
            m_outbox.insert(m_outbox.end(), buf, buf + buf_len);
	}

	// no write pending and have output pending
	if(m_write_buf.empty() && !m_outbox.empty())
	{
            std::swap(m_outbox, m_write_buf);

            boost::asio::async_write(m_socket,
				     boost::asio::buffer(&m_write_buf[0], m_write_buf.size()),
				     m_strand.wrap(
					 boost::bind(&tls_server_session::handle_write,
						     shared_from_this(),
						     boost::asio::placeholders::error)));
	}
    }

    void tls_alert_cb(Botan::TLS::Alert alert, const Botan::byte[], size_t)
    {
	std::cout << "Alert:" << alert.type() << " " << alert.type_string() << std::endl;
	if(alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
	{
            m_tls.close();
            return;
	}
    }

    void tls_data_recv(const Botan::byte buf[], size_t buf_len)
    {
	m_client_data.insert(m_client_data.end(), buf, buf + buf_len);
	auto request = http::request_parser::parse();//m_client_data.begin(), m_client_data.end());
	auto response = fulfill_request(request);
	write_response_encrypted(response);
    }

    void handle_connection_established(const boost::system::error_code& error)
    {
	tunnel_established = !error;
	handle_write(error);
    }

    void write_connection_established_response()
    {
	std::string out;
	out += "HTTP/1.0 200 Connection established\r\n\r\n";

	m_write_buf.clear();
	for(auto c : out)
	{
	    m_write_buf.push_back(c);
	}
	
	boost::asio::async_write(m_socket,
				 boost::asio::buffer(&m_write_buf[0], m_write_buf.size()),
				 m_strand.wrap(
				     boost::bind(&tls_server_session::handle_connection_established,
						 shared_from_this(),
						 boost::asio::placeholders::error)));
    }

    void write_response_plaintext(const http::response &response)
    {

    }

    void write_response_encrypted(const http::response &response)
    {
	std::string response_string = response.to_string();

	m_tls.send(response_string);
	m_tls.close();
    }

    bool tls_handshake_complete(const Botan::TLS::Session& session)
    {
	m_hostname = session.server_info().hostname();
	return true;
    }

    boost::asio::io_service::strand m_strand; // serialization

    boost::asio::ip::tcp::socket m_socket;
    Botan::TLS::Server m_tls;
    std::string m_hostname;

    unsigned char m_read_buf[1024];

    // used to hold the data currently being written by the system
    std::vector<Botan::byte> m_write_buf;

    // used to hold data queued for writing
    std::vector<Botan::byte> m_outbox;

    std::vector<Botan::byte> m_client_data;

    bool tunnel_established;
};
