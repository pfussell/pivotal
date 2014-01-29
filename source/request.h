#pragma once

#include <string>
#include <vector>

#include "header.h"

namespace pivotal {

struct request
{
	std::string method;
	std::string uri;
	bool secure;
	int port;
	std::string protocol;
	std::string object;
	int http_version_major;
	int http_version_minor;
	std::vector<header> headers;
};

} // namespace pivotal
