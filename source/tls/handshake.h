#pragma once

#include <array>
#include <stdint.h>

#include "serializable.h"

namespace pivotal {
namespace tls {

enum class handshake_type
{
	hello_request = 0,
	client_hello = 1,
	server_hello = 2,
	certificate = 11,
	server_key_exchange = 12,
	certficate_request = 13,
	server_hello_done = 14,
	certficate_verify = 15,
	client_key_exchange = 16,
	finished = 20,
	invalid = 255
};

struct random
{
	uint32_t gmt_unix_time;
	std::array<uint8_t, 28> random_numbers;
};

struct hello_request 
{
	random random;
	uint8_t session_id;
};

struct client_hello {};
struct server_hello {};
struct certificate {};
struct server_key_exchange {};
struct certficate_request {};
struct server_hello_done {};
struct certificate_verify {};
struct client_key_exchange {};
struct finished {};

class handshake : public serializable
{
public:
	size_t from_bytes(const buffer &source);
	size_t to_bytes(buffer &dest);

	handshake_type get_type() { return type_; }
	int get_length() { return length_; }
	hello_request &get_hello_request() { return hello_request_; }

private:
	handshake_type type_;
	int length_;
	hello_request hello_request_;
	client_hello client_hello_;
	server_hello server_hello_;
	certificate certificate_;
	server_key_exchange server_key_exchange_;
	certficate_request certficate_request_;
	server_hello_done server_hello_done_;
	certificate_verify certificate_verify_;
	client_key_exchange client_key_exchange_;
	finished finished_;
};

} // namespace tls
} // namespace pivotal
