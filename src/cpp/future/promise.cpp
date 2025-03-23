//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <thread>
#include <stdfloat>
#include <random>

using std::string_literals::operator""s;

namespace lib
{
	std::random_device rnd;
	std::mt19937 rng{lib::rnd()};
}

int main()
try
{
	std::promise<std::float16_t> promise;
	std::future<std::float16_t> future = promise.get_future();

	std::jthread thread{
		[] (std::promise<std::float16_t> promise)
		{
			try
			{
				int x = lib::rng();
				if (x%3 == 0)
					promise.set_value(2.32f16);
				else if (x%3 == 1)
					promise.set_value(-7.782f16);
				else
					throw std::runtime_error{"status is test: test exception"};
			}
			catch (...)
			{
				promise.set_exception(std::current_exception());
			}
		},
		std::move(promise)
	};

	std::float16_t result = future.get();

	throw std::runtime_error{"status is OK: I got value "s + std::to_string((float)result)};

	return 0;
}
catch (const std::exception & e)
{
	std::cout << "====> " << e.what() << std::endl;
	return 0;
}
