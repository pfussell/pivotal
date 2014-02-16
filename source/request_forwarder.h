#pragma once

#include <array>
#include <string>

#include "response.h"

namespace pivotal {
namespace http {

struct request;
struct request_forwarder_impl;

class request_forwarder
{
public:
    request_forwarder();

    ~request_forwarder();

    response forward_request(const request &req);

private:
	static const size_t read_buffer_size = 2048;

	std::array<char, read_buffer_size> read_buffer;

	request_forwarder_impl *impl_;
};

} // namespace http
} // namespace pivotal
