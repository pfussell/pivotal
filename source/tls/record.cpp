#include "record.h"

namespace pivotal {
namespace tls {

size_t record::from_bytes(const buffer &source)
{
	size_t offset = 0;
	type_ = (content_type)source[offset++];
	version_ = (content_version)read2(source, offset);
	length_ = read2(source, offset);
	return offset;
}

size_t record::to_bytes(buffer &dest)
{
	size_t offset = 0;
	dest[offset++] = (uint8_t)type_;
	write2((uint16_t)type_, dest, offset);
	write2(length_, dest, offset);
	return offset;
}

}
}