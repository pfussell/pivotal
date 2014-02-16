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

namespace pivotal {
namespace http {

class session : public boost::enable_shared_from_this<session>
{
public:
    typedef boost::shared_ptr<session> pointer;

    static pointer create(boost::asio::io_service &io_service,
			  Botan::TLS::Session_Manager &session_manager,
			  Botan::Credentials_Manager &credentials,
			  Botan::TLS::Policy &policy,
			  Botan::RandomNumberGenerator &rng,
			  request_forwarder &forwarder)
    {
		auto s = new session(io_service, session_manager, credentials, policy, rng, forwarder);
		return pointer(s);
    }

    boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

    void start()
    {
		m_socket.async_read_some(
            boost::asio::buffer(m_read_buf, sizeof(m_read_buf)),
            m_strand.wrap(
				boost::bind(&session::handle_read, shared_from_this(),
					    boost::asio::placeholders::error,
					    boost::asio::placeholders::bytes_transferred)));
    }

    void stop()
    {
		m_socket.close();
    }

private:
    session(boost::asio::io_service& io_service,
		       Botan::TLS::Session_Manager& session_manager,
		       Botan::Credentials_Manager& credentials,
		       Botan::TLS::Policy& policy,
		       Botan::RandomNumberGenerator& rng,
		       request_forwarder &forwarder) :
	m_strand(io_service),
	m_socket(io_service),
	m_tls(boost::bind(&session::tls_output_wanted, this, _1, _2),
	      boost::bind(&session::tls_data_recv, this, _1, _2),
	      boost::bind(&session::tls_alert_cb, this, _1, _2, _3),
	      boost::bind(&session::tls_handshake_complete, this, _1),
	      session_manager,
	      credentials,
	      policy,
	      rng),
	tunnel_established(false),
	request_forwarder_(forwarder)
    {
    }

    void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
    {
		if(!error)
		{
	        try
		    {
				if(!tunnel_established)
				{
				    auto parse_result = request_parser_.parse(request_, m_read_buf.begin(), m_read_buf.end());
				    m_read_buf.fill('\0');

			    	switch(std::get<0>(parse_result))
					{
					case request_parser::good:
						std::cout << "HTTP : " << request_.method << " " << request_.server << " " << request_.object << std::endl;
					    if(request_.method == "CONNECT")
					    {
							write_connection_established_response();
					    }
					    else
					    {
							write_response_plaintext(request_forwarder_.forward_request(request_));
					    }
						request_parser_.reset();
						request_ = request();
						break;
					case request_parser::bad:
						write_response_plaintext(response::stock_response(response::status_type::bad_request));
						request_parser_.reset();
						request_ = request();
						break;
					case request_parser::indeterminate:
						// Maintain parsing state until next packet is received
					default:
						// How did this happen?
						std::runtime_error("unexpected");
					}
				}
				else
				{
				    m_tls.received_data(m_read_buf.data(), bytes_transferred);
				    m_read_buf.fill('\0');
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
					m_strand.wrap(boost::bind(&session::handle_read, shared_from_this(),
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
					 boost::bind(&session::handle_write,
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
		auto parse_result = request_parser_.parse(request_, buf, buf + buf_len);

		switch(std::get<0>(parse_result))
		{
		case request_parser::good:
			request_.server = m_hostname;
			std::cout << "HTTPS : " << request_.method << " " << request_.server << " " << request_.object << std::endl;
			write_response_encrypted(request_forwarder_.forward_request(request_));
			request_parser_.reset();
			request_ = request();
			break;
		case request_parser::bad:
			write_response_encrypted(response::stock_response(response::status_type::bad_request));
			request_parser_.reset();
			request_ = request();
			break;
		case request_parser::indeterminate:
			// Maintain parsing state until next packet is received
		default:
			// How did this happen?
			std::runtime_error("unexpected");
		}
    }

    void handle_connection_established(const boost::system::error_code& error)
    {
		tunnel_established = !error;
		m_write_buf.clear();

		if(tunnel_established)
		{
			std::cout << "initiating tls handshake" << std::endl;
			handle_write(error);
		}
		else
		{
			throw std::runtime_error("error establishing secure tunnel");
		}
    }

    void write_connection_established_response()
    {
		std::string response_string = "HTTP/1.1 200 Connection established\r\n\r\n";
		m_write_buf.insert(m_write_buf.end(), response_string.begin(), response_string.end());

		boost::asio::async_write(m_socket,
					 boost::asio::buffer(&m_write_buf[0], m_write_buf.size()),
					 m_strand.wrap(
					     boost::bind(&session::handle_connection_established,
							 shared_from_this(),
							 boost::asio::placeholders::error)));
    }

    void write_response_plaintext(const response &response)
    {
    	m_write_buf.clear();
    	auto response_string = response.to_string();
    	m_write_buf.insert(m_write_buf.end(), response_string.begin(), response_string.end());
    	boost::asio::write(m_socket, boost::asio::buffer(&m_write_buf[0], m_write_buf.size()));
    }

    void write_response_encrypted(const response &response)
    {
		std::string response_string = response.to_string();

		m_tls.send(response_string);
		m_tls.close();
    }

    bool tls_handshake_complete(const Botan::TLS::Session& session)
    {
		m_hostname = session.server_info().hostname();
		std::cout << "secure tunnel established to " << m_hostname << std::endl;
		return true;
    }

    boost::asio::io_service::strand m_strand; // serialization

    boost::asio::ip::tcp::socket m_socket;
    Botan::TLS::Server m_tls;
    std::string m_hostname;

    std::array<unsigned char, 1024> m_read_buf;;

    // used to hold the data currently being written by the system
    std::vector<Botan::byte> m_write_buf;

    // used to hold data queued for writing
    std::vector<Botan::byte> m_outbox;

    bool tunnel_established;

    request_parser request_parser_;

    request_forwarder &request_forwarder_;

    request request_;
};

} // namespace http
} // namespace pivotal
