#pragma once

#include <array>
#include <memory>
#include <boost/asio.hpp>

#include "response.h"
#include "request.h"
#include "request_handler.h"
#include "request_parser.h"

namespace pivotal {
namespace server {

class connection_manager;

/// Represents a single connection from a client.
class connection : public std::enable_shared_from_this<connection>
{
public:
    explicit connection(boost::asio::ip::tcp::socket socket, connection_manager &manager, request_handler &handler);

    void start();

    void stop();

private:
    void read_request();

    void write_response();

    boost::asio::ip::tcp::socket socket_;

    connection_manager &connection_manager_;

    request_handler &request_handler_;

	std::array<char, buffer_size> request_buffer_;

	std::array<char, buffer_size> response_buffer_;

    request request_;

    request_parser request_parser_;

    response response_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace pivotal
