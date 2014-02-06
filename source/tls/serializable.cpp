#include "serializable.h"

namespace pivotal {
namespace tls {

uint16_t serializable::read2(const buffer &source, size_t &offset)
{
	uint16_t value = source[offset++];
	value <<= 8;
	value = source[offset++];
	return value;
}

uint32_t serializable::read3(const buffer &source, size_t &offset)
{
	uint16_t value = source[offset++];
	value <<= 8;
	value = source[offset++];
	value <<= 8;
	value = source[offset++];
	return value;
}

uint32_t serializable::read4(const buffer &source, size_t &offset)
{
	uint16_t value = source[offset++];
	value <<= 8;
	value = source[offset++];
	value <<= 8;
	value = source[offset++];
	value <<= 8;
	value = source[offset++];
	return value;
}

void serializable::write2(uint16_t value, buffer &dest, size_t &offset)
{
	dest[offset++] = static_cast<uint8_t>(value >> 8);
	dest[offset++] = static_cast<uint8_t>(value);
}

void serializable::write3(uint32_t value, buffer &dest, size_t &offset)
{
	dest[offset++] = static_cast<uint8_t>(value >> 16);
	dest[offset++] = static_cast<uint8_t>(value >> 8);
	dest[offset++] = static_cast<uint8_t>(value);
}

void serializable::write4(uint32_t value, buffer &dest, size_t &offset)
{
	dest[offset++] = static_cast<uint8_t>(value >> 24);
	dest[offset++] = static_cast<uint8_t>(value >> 16);
	dest[offset++] = static_cast<uint8_t>(value >> 8);
	dest[offset++] = static_cast<uint8_t>(value);
}

} // namespace tls
} // namespace pivotal
