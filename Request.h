#include <string>
#include <vector>

typedef unordered_map<std::wstring, std::wstring> Headers;

enum class Port
{
	Ftp,
	Http,
	Https,
	Default
};

enum class Service
{
	Ftp,
	Http
};

struct Request
{
	std::wstring method;
	std::wstring host_name;
	std::wstring target;
	std::wstring username;
	std::wstring password;
	Port port;
	Service service;
	std::wstring referer;
	Headers headers;
	std::wstring content;
};
