#pragma once

#include <string>

namespace pivotal {
namespace http {

struct header
{
	header(const std::string &name = "", const std::string &value = "")
		: name(name),
		  value(value)
	{
	}

	std::string name;
	std::string value;
};

} // namespace http
} // namespace pivotal
