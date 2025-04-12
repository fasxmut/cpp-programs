//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <chrono>
#include <iostream>
#include <mutex>

namespace g
{
	std::mutex mutex;
}

int main()
try
{
	std::packaged_task<double(double, double)> task
		{
			[] (const double & x, const double & y)
			{
				double result = x*x + x*y + y*y;
				std::unique_lock lock{g::mutex};
				std::cout << "Make result " << result << std::endl;
				lock.unlock();
				std::this_thread::sleep_for(std::chrono::seconds(1));
				throw std::runtime_error{"test error"};
				return result;
			}
		};
	std::future<double> future = task.get_future();
	std::future<void> thread_future = std::async(
		std::move(task),
		1.2,
		2.3
	);
	std::unique_lock lock{g::mutex};
	std::cout << "Wait ..." << std::endl;
	lock.unlock();
	float r = future.get();
	lock.lock();
	std::cout << "r=> " << r << std::endl;
	lock.unlock();
}
catch (const std::exception & e)
{
	std::unique_lock lock{g::mutex};
	std::cout << "=> " << e.what() << std::endl;
	lock.unlock();
}
