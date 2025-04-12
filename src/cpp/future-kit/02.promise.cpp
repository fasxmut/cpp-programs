//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <stdfloat>
#include <chrono>
#include <thread>

int main()
{
	std::promise<std::float16_t> promise;
	std::future<std::float16_t> future = promise.get_future();
	std::future<void> thread = std::async(
		[] (auto promise)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			promise.set_value(321.0f16);
		},
		std::move(promise)
	);
	std::cout << "Wait ..." << std::endl;
	std::cout << "Result: " << future.get() << std::endl;
}
