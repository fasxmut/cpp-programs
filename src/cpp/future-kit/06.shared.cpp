//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <vector>
#include <numbers>
#include <mutex>

namespace g
{
	std::mutex mutex;
}

int main()
{
	std::vector<std::shared_future<double>> future_pool;
	for (int i=1; i<=10; ++i)
	{
		future_pool.push_back(
			std::async(
				[] (double x, double y)
				{
					double r = x*x + y*y + x*y;
					std::unique_lock lock{g::mutex};
					std::cout << "log: " << x << ',' << y << " => " << r << std::endl;
					return r;
				},
				std::numbers::pi/i,
				std::numbers::e/i
			)
		);
	}
	auto transfer_futures = [] (std::vector<std::shared_future<double>> & future_pool)
	{
		std::vector<std::shared_future<double>> new_pool;
		for (auto f: future_pool)
		{
			new_pool.push_back(f);
		}
		return new_pool;
	};
	std::vector<std::future<void>> thread_future_pool;
	for (int i=0; i<100; ++i)
	{
		thread_future_pool.push_back(
			std::async(
				[] (auto future_pool)
				{
					for (auto & f: future_pool)
					{
						auto r = f.get();
						std::unique_lock lock{g::mutex};
						std::cout << "Got => " << r << std::endl;
					}
				},
				transfer_futures(future_pool)
			)
		);
	}
}
