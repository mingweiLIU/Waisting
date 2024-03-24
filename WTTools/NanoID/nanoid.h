#pragma once

#include "../settings.h"
#include "nanoid/crypto_random.h"
#include <string>
#include <random>
#include <future>

namespace NANOID_NAMESPACE
{
	class WTTOOLAPI NanoID {
	public:
		static std::string generate();
		static std::string generate(const std::string& alphabet);
		static std::string generate(std::size_t size);
		static std::string generate(const std::string& alphabet, std::size_t size);

		static std::future<std::string> generate_async();
		static std::future<std::string> generate_async(const std::string& alphabet);
		static std::future<std::string> generate_async(std::size_t size);
		static std::future<std::string> generate_async(const std::string& alphabet, std::size_t size);

		static std::string generate(crypto_random_base& random);
		static std::string generate(crypto_random_base& random, const std::string& alphabet);
		static std::string generate(crypto_random_base& random, std::size_t size);
		static std::string generate(crypto_random_base& random, const std::string& alphabet, std::size_t size);

	};
}