#include <utility>
#include <vector>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace pivotal {

connection::connection(boost::asio::ip::tcp::socket socket, connection_manager &manager, request_handler &handler)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      request_handler_(handler)
{

}

void connection::start()
{
    do_read();
}

void connection::stop()
{
    socket_.close();
}

void connection::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(buffer_),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
        {
            if(!ec)
            {
                request_parser::result_type result;
                std::tie(result, std::ignore) = request_parser_.parse(
                request_, buffer_.data(), buffer_.data() + bytes_transferred);

                if(result == request_parser::good)
                {
                    request_handler_.handle_request(request_, response_);
                    do_write();
                }
                else if (result == request_parser::bad)
                {
                    response_ = response::stock_response(response::bad_request);
                    do_write();
                }
                else
                {
                    do_read();
                }
            }
            else if (ec != boost::asio::error::operation_aborted)
            {
                connection_manager_.stop(shared_from_this());
            }
        });
}

void connection::do_write()
{
    auto self(shared_from_this());
    boost::asio::async_write(socket_, response_.to_buffers(),
        [this, self](boost::system::error_code ec, std::size_t)
        {
            if (!ec)
            {
                boost::system::error_code ignored_ec;
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                ignored_ec);
            }

            if (ec != boost::asio::error::operation_aborted)
            {
                connection_manager_.stop(shared_from_this());
            }
        });
}

} // namespace pivotal
