#include <utility>
#include <vector>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace pivotal {
namespace server {

connection::connection(boost::asio::ip::tcp::socket socket, connection_manager &manager, request_handler &handler)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      request_handler_(handler)
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
	bool tunnel = false;

	if(result == request_parser::good)
	{
		tunnel = request_handler_.handle_request(request_, response_);
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

	bool keep_going = request_handler_.tunnel_request(request_buffer_, response_buffer_);

	bytes_transferred = socket_.write_some(boost::asio::buffer(response_buffer_), ec);

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

} // namespace server
} // namespace pivotal

