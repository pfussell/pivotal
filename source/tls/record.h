#pragma once

#include <stdint.h>
#include <vector>

#include "handshake.h"
#include "serializable.h"

namespace pivotal {
namespace tls {

enum class content_type
{
	change_cipher_spec = 0x14,
	alert = 0x15,
	handshake = 0x16,
	application = 0x17
};

enum class content_version
{
	sslv30 = 0x0300,
	tlsv10 = 0x0301,
	tlsv11 = 0x0302,
	tlsv12 = 0x0303
};

class record : public serializable
{
public:
	virtual size_t from_bytes(const buffer &source);
	virtual size_t to_bytes(buffer &dest);

	content_type get_type() { return type_; }
	content_version get_version() { return version_; }
	uint16_t get_length() { return length_; }
	handshake &get_handshake() { return handshake_; }

private:
	handshake handshake_;
	content_type type_;
	content_version version_;
	uint16_t length_;
};

} // namespace tls
} // namespace pivotal
