#pragma once

#include <string>

namespace pivotal {

struct response;
struct request;

class request_handler
{
public:
    request_handler();

    void handle_request(const request &req, response &rep);

private:
    static bool url_decode(const std::string &in, std::string &out);
};

} // namespace pivotal
