#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "header.h"

namespace pivotal {
namespace http {

enum class http_version
{
	http10,
	http11
};

struct request
{
	std::string method;
	std::string uri;
	std::string protocol;
	std::string server;
	bool secure;
	int port;
	std::string object;
	int http_version_major;
	int http_version_minor;
	std::vector<header> headers;

	bool has_header(const std::string &name) const
	{
		return std::find_if(headers.begin(), headers.end(), [name](const header &h) { return h.name == name; }) != headers.end();
	}

	std::string get_header(const std::string &name) const
	{
		auto match = std::find_if(headers.begin(), headers.end(), [name](const header &h) { return h.name == name; });
		if(match != headers.end())
		{
			return match->value;
		}
		throw std::runtime_error("header not found: " + name);
	}
};

} // namespace http
} // namespace pivotal
