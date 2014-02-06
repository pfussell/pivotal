#pragma once

#include <array>
#include <stdint.h>

#include "../constants.h"

namespace pivotal {
namespace tls {

typedef std::array<uint8_t, buffer_size> buffer;

class serializable
{
public:
	virtual size_t from_bytes(const buffer &source) = 0;
	virtual size_t to_bytes(buffer &dest) = 0;

	static uint16_t read2(const buffer &source, size_t &offset);
	static uint32_t read3(const buffer &source, size_t &offset);
	static uint32_t read4(const buffer &source, size_t &offset);

	static void write2(uint16_t value, buffer &dest, size_t &offset);
	static void write3(uint32_t value, buffer &dest, size_t &offset);
	static void write4(uint32_t value, buffer &dest, size_t &offset);
};

} // namespace tls
} // namespace pivotal
