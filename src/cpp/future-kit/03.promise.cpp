//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <mutex>
#include <stdfloat>

namespace g
{
	std::mutex mutex;
}

int main()
try
{
	std::promise<std::float16_t> promise;
	std::future<std::float16_t> future = promise.get_future();
	std::future<void> thread = std::async(
		[&promise]
		{
			try
			{
				throw std::runtime_error{"test error"};
			}
			catch (...)
			{
				promise.set_exception(std::current_exception());
			}
		}
	);
	std::unique_lock<std::mutex> lock{g::mutex};
	std::cout << "===" << std::endl;
	std::cout << ": " << future.get() << std::endl;
	std::cout << "===" << std::endl;
}
catch (const std::exception & e)
{
	std::unique_lock<std::mutex> lock{g::mutex};
	std::cout << "\n\n=> " << e.what();
	std::cout << "<=\n\n" << std::endl;
}
