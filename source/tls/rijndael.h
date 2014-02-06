#pragma once

#include <array>
#include <stdint.h>
#include <string>
#include <vector>

namespace pivotal {
namespace tls {

class invalid_block_size_exception : public std::runtime_error
{
public:
	invalid_block_size_exception(size_t block_size) : std::runtime_error("invalid block size: " + std::to_string(block_size))
	{

	}
};

class invalid_key_size_exception : public std::runtime_error
{
public:
	invalid_key_size_exception(size_t block_size) : std::runtime_error("invalid block size: " + std::to_string(block_size))
	{
		
	}
};

class rijndael
{
public:
	rijndael(const std::string &key, size_t block_size);

	// Some people think it's bad to return a container by value
	// Too bad for them...

	static std::vector<uint8_t> encrypt(const std::string &key, const std::string &plain_text);
	
	static std::string decrypt(const std::string &key, const std::vector<uint8_t> &encrypted);

	static void test();

	std::vector<uint8_t> encrypt(const std::string &plain_text);

	std::string decrypt(const std::vector<uint8_t> &encrypted);

private:
	static size_t calc_num_rounds(size_t block_size, size_t key_size);

	static bool is_valid_block_size(size_t block_size);

	static bool is_valid_key_size(size_t key_size);

	// Not supporting 160, 224 for now
	static uint32_t valid_block_sizes[1];

	// Not supporting 160, 224 for now
	static uint32_t valid_key_sizes[1];

	static uint32_t t[8][256];

	static uint32_t u[4][256];

	static uint8_t s[256];

	static uint8_t si[256];

	static uint8_t rcon[30];

	static uint8_t log[256];

	static uint8_t alog[256];

	static uint8_t e_shifts[3][4];

	static uint8_t d_shifts[3][4];

	size_t key_size_;

	size_t block_size_;

	std::vector<std::vector<uint32_t>> ke_;

	std::vector<std::vector<uint32_t>> kd_;
};

} // namespace tls
} // namespace pivotal
