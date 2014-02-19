#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <boost/asio.hpp>

#include "header.h"

namespace pivotal {
namespace http {

struct response
{
    enum status_type
    {
        ok = 200,
        created = 201,
        accepted = 202,
        no_content = 204,
        multiple_choices = 300,
        moved_permanently = 301,
        moved_temporarily = 302,
        not_modified = 304,
        bad_request = 400,
        unauthorized = 401,
        forbidden = 403,
        not_found = 404,
        internal_server_error = 500,
        not_implemented = 501,
        bad_gateway = 502,
        service_unavailable = 503,
		connected = 1001
    } status;

	struct
	{
		unsigned char major;
		unsigned char minor;
	} http_version;

    std::vector<header> headers;

    std::string content;

    std::string to_string(unsigned char http_version_major, unsigned char http_version_minor) const;

    std::vector<boost::asio::const_buffer> to_buffers();

    static response stock_response(status_type status);
};

} // namespace http
} // namespace pivotal
