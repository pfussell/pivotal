#include <tuple>

#include "session.h"
#include "response.h"
#include "request_parser.h"

namespace pivotal {
namespace http {

session::pointer session::create(boost::asio::io_service &io_service,
	Botan::TLS::Session_Manager &session_manager,
	Botan::Credentials_Manager &credentials,
	Botan::TLS::Policy &policy,
	Botan::RandomNumberGenerator &rng)
{
	auto s = new session(io_service, session_manager, credentials, policy, rng);
	return pointer(s);
}

void session::start()
{
	m_socket.async_read_some(
		boost::asio::buffer(m_read_buf, sizeof(m_read_buf)),
		m_strand.wrap(
		boost::bind(&session::handle_read, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred)));
}

void session::stop()
{
	m_socket.close();
}

session::session(boost::asio::io_service& io_service,
	Botan::TLS::Session_Manager& session_manager,
	Botan::Credentials_Manager& credentials,
	Botan::TLS::Policy& policy,
	Botan::RandomNumberGenerator& rng) :
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
	tunnel_established(false)
{
}

void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if(!error)
	{
		try
		{
			if(!tunnel_established)
			{
				auto parse_result = request_parser_.parse(request_, m_read_buf.begin(), m_read_buf.begin() + bytes_transferred);

				switch(std::get<0>(parse_result))
				{
				case request_parser::good:
					std::cout << "HTTP " << request_.method << " " << request_.server << " " << request_.port << " " << request_.object << std::endl;
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
					std::cout << "bad plaintext request" << std::endl;
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
			}
		}
		catch(std::exception& e)
		{
			std::cout << "Read failed: " << e.what() << "\n";
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

void session::handle_write(const boost::system::error_code& error)
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

void session::tls_output_wanted(const Botan::byte buf[], size_t buf_len)
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

void session::tls_alert_cb(Botan::TLS::Alert alert, const Botan::byte[], size_t)
{
	std::cout << "Alert:" << alert.type() << " " << alert.type_string() << std::endl;
	if(alert.type() == Botan::TLS::Alert::CLOSE_NOTIFY)
	{
		m_tls.close();
		return;
	}
}

void session::tls_data_recv(const Botan::byte buf[], size_t buf_len)
{
	auto parse_result = request_parser_.parse(request_, buf, buf + buf_len);
	request_.server = m_hostname;
	request_.port = 443;
	request_.secure = true;

	switch(std::get<0>(parse_result))
	{
	case request_parser::good:
		std::cout << "HTTPS " << request_.method << " " << request_.server << ":" << request_.port << " " << request_.object << std::endl;
		write_response_encrypted(request_forwarder_.forward_request(request_));
		request_parser_.reset();
		request_ = request();
		break;
	case request_parser::bad:
		std::cout << "bad encrypted request" << std::endl;
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

void session::handle_connection_established(const boost::system::error_code& error)
{
	tunnel_established = !error;
	m_write_buf.clear();

	if(tunnel_established)
	{
		std::cout << "initiating tls handshake (local client->proxy)" << std::endl;
		handle_write(error);
	}
	else
	{
		throw std::runtime_error("error during tls handshake initiation (local client->proxy)");
	}
}

void session::write_connection_established_response()
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

void session::write_response_plaintext(const response &response)
{
	m_write_buf.clear();
	auto response_string = response.to_string(1, 1);
	m_write_buf.insert(m_write_buf.end(), response_string.begin(), response_string.end());

	boost::asio::async_write(m_socket,
		boost::asio::buffer(&m_write_buf[0], m_write_buf.size()),
		m_strand.wrap(
			boost::bind(&session::handle_write,
			shared_from_this(),
			boost::asio::placeholders::error)));
}

void session::write_response_encrypted(const response &response)
{
	std::string response_string = response.to_string(1, 1);
	m_tls.send(response_string);
}

bool session::tls_handshake_complete(const Botan::TLS::Session& session)
{
	m_hostname = session.server_info().hostname();
	std::cout << "tls handshake completed (local client->proxy)" << std::endl;
	std::cout << "now intercepting all encrypted http traffic from local client to " << m_hostname << " and routing through WinINet" << std::endl;
	return true;
}

} // namespace http
} // namespace pivotal
