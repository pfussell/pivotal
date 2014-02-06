#pragma once

#include <array>
#include <stdint.h>
#include <string>
#include <vector>

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

	static void initialize();

	static bool initialized;

	// Not supporting 160, 224 for now
	static const std::array<size_t, 1> valid_block_sizes;

	// Not supporting 160, 224 for now
	static const std::array<size_t, 1> valid_key_sizes;

	static std::array<std::array<uint32_t, 256>, 8> t;

	static std::array<std::array<uint32_t, 256>, 4> u;

	static std::array<uint8_t, 256> s;

	static std::array<uint8_t, 256> si;

	static std::array<uint8_t, 30> rcon;

	static std::array<uint8_t, 256> log;

	static std::array<uint8_t, 256> alog;

	static std::array<std::array<std::pair<int, int>, 4>, 3> shifts;

	size_t key_size_;

	size_t block_size_;

	std::vector<std::vector<uint32_t>> ke_;

	std::vector<std::vector<uint32_t>> kd_;
};