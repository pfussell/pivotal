#include <utility>
#include <vector>

#include "connection.h"
#include "connection_manager.h"

namespace pivotal {
namespace http {

connection::connection(boost::asio::ip::tcp::socket socket, connection_manager &manager, request_forwarder &forwarder)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      request_forwarder_(forwarder)
{

}

void connection::start()
{
	read_request();
}

void connection::stop()
{
    socket_.close();
}

void connection::read_request()
{
	boost::system::error_code ec;
	size_t bytes_transferred = socket_.read_some(boost::asio::buffer(request_buffer_), ec);

	if(ec)
    {
        connection_manager_.stop(shared_from_this());
		return;
    }

	request_parser::result_type result;
	std::tie(result, std::ignore) = request_parser_.parse(request_, request_buffer_.data(), request_buffer_.data() + bytes_transferred);

	if(result == request_parser::good)
	{
		response_ = request_forwarder_.forward_request(request_);
	}
	else
	{
		response_ = response::stock_response(response::bad_request);
	}

	write_response();

	if(ec)
	{
		connection_manager_.stop(shared_from_this());
		return;
	}

	boost::system::error_code ignored_ec;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void connection::write_response()
{
	boost::system::error_code ec;
	socket_.write_some(response_.to_buffers(), ec);

	if(ec)
    {
        connection_manager_.stop(shared_from_this());
		return;
    }
}

} // namespace http
} // namespace pivotal

