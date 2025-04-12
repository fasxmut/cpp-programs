//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <numbers>
#include <chrono>

int main()
{
	std::packaged_task<double(double)> task
		{
			[] (const double & x)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				return x*x*x;
			}
		};
	std::future<double> future = task.get_future();
	std::future<void> thread_future = std::async(
		std::move(task),
		std::numbers::pi
	);
	std::cout << "Wait ..." << std::endl;
	std::cout << "Got: " << future.get() << std::endl;
}
