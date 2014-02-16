#include <string>

#include "response.h"

namespace pivotal {
namespace http {

namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string connected =
  "HTTP/1.0 200 Connection established\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(response::status_type status)
{
  switch (status)
  {
  case response::ok:
    return boost::asio::buffer(ok);
  case response::connected:
	  return boost::asio::buffer(connected);
  case response::created:
    return boost::asio::buffer(created);
  case response::accepted:
    return boost::asio::buffer(accepted);
  case response::no_content:
    return boost::asio::buffer(no_content);
  case response::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case response::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case response::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case response::not_modified:
    return boost::asio::buffer(not_modified);
  case response::bad_request:
    return boost::asio::buffer(bad_request);
  case response::unauthorized:
    return boost::asio::buffer(unauthorized);
  case response::forbidden:
    return boost::asio::buffer(forbidden);
  case response::not_found:
    return boost::asio::buffer(not_found);
  case response::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case response::not_implemented:
    return boost::asio::buffer(not_implemented);
  case response::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case response::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> response::to_buffers()
{
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(status_strings::to_buffer(status));
  for (std::size_t i = 0; i < headers.size(); ++i)
  {
    header& h = headers[i];
    buffers.push_back(boost::asio::buffer(h.name));
    buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
    buffers.push_back(boost::asio::buffer(h.value));
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  }
  buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  buffers.push_back(boost::asio::buffer(content));
  return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(response::status_type status)
{
  switch (status)
  {
  case response::ok:
    return ok;
  case response::created:
    return created;
  case response::accepted:
    return accepted;
  case response::no_content:
    return no_content;
  case response::multiple_choices:
    return multiple_choices;
  case response::moved_permanently:
    return moved_permanently;
  case response::moved_temporarily:
    return moved_temporarily;
  case response::not_modified:
    return not_modified;
  case response::bad_request:
    return bad_request;
  case response::unauthorized:
    return unauthorized;
  case response::forbidden:
    return forbidden;
  case response::not_found:
    return not_found;
  case response::internal_server_error:
    return internal_server_error;
  case response::not_implemented:
    return not_implemented;
  case response::bad_gateway:
    return bad_gateway;
  case response::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

response response::stock_response(response::status_type status)
{
  response rep;
  rep.status = status;
  rep.content = stock_replies::to_string(status);
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "text/html";
  return rep;
}

std::string response::to_string() const
{
    std::stringstream ss;
    switch (status)
    {
    case response::ok:
        ss << status_strings::ok;
        break;
    case response::connected:
        ss << status_strings::connected;
        break;
    case response::created:
        ss << status_strings::created;
        break;
    case response::accepted:
        ss << status_strings::accepted;
        break;
    case response::no_content:
        ss << status_strings::no_content;
        break;
    case response::multiple_choices:
        ss << status_strings::multiple_choices;
        break;
    case response::moved_permanently:
        ss << status_strings::moved_permanently;
        break;
    case response::moved_temporarily:
        ss << status_strings::moved_temporarily;
        break;
    case response::not_modified:
        ss << status_strings::not_modified;
        break;
    case response::bad_request:
        ss << status_strings::bad_request;
        break;
    case response::unauthorized:
        ss << status_strings::unauthorized;
        break;
    case response::forbidden:
        ss << status_strings::forbidden;
        break;
    case response::not_found:
        ss << status_strings::not_found;
        break;
    case response::internal_server_error:
        ss << status_strings::internal_server_error;
        break;
    case response::not_implemented:
        ss << status_strings::not_implemented;
        break;
    case response::bad_gateway:
        ss << status_strings::bad_gateway;
        break;
    case response::service_unavailable:
        ss << status_strings::service_unavailable;
        break;
    default:
        ss << status_strings::internal_server_error;
    }
    for(auto header : headers)
    {
        ss << header.name << ": " << header.value << "\r\n";
    }
    ss << "\r\n";
    ss << content;
    return ss.str();
}

} // namespace http
} // namespace pivotal
