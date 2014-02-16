#include "request_forwarder.h"

namespace pivotal {
namespace http {

struct request_forwarder_impl
{

};

request_forwarder::request_forwarder() : impl_(new request_forwarder_impl())
{
}

request_forwarder::~request_forwarder()
{
	delete impl_;
}

response request_forwarder::forward_request(const request &request)
{
	std::string request_string = requestt.to_string();
	read_buffer[0] = '\0';
	return response::stock_response(response::status_type::ok);
}

} // namespace http
} // namespace pivotal
