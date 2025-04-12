//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <vector>
#include <mutex>
#include <numbers>

namespace g
{
	std::mutex mutex;
}

int main()
{
	std::vector<std::future<double>> future_pool;
	for (int i=1; i<=10; ++i)
	{
		future_pool.push_back(
			std::async(
				[] (double x, double y)
				{
					double r = x*x + y*y + x*y;
					return r;
				},
				std::numbers::pi/i,
				std::numbers::phi/i
			)
		);
	}
	auto transfer_futures = [] (decltype(future_pool) & future_pool)
	{
		std::vector<std::shared_future<double>> new_pool;
		for (auto & f: future_pool)
			new_pool.push_back(f.share());
		return new_pool;
	};
	auto futures = transfer_futures(future_pool);
	auto transfer_futures_2 = [] (decltype(futures) futures)
	{
		std::vector<std::shared_future<double>> new_pool;
		for (auto f: futures)
			new_pool.push_back(f);
		return new_pool;
	};
	auto futures_2 = transfer_futures_2(futures);
	std::vector<std::future<void>> thread_pool;
	for (int i=0; i<100; ++i)
	{
		thread_pool.push_back(
			std::async(
				[] (std::vector<std::shared_future<double>> future_pool)
				{
					for (auto f: future_pool)
					{
						double r = f.get();
						std::unique_lock lock{g::mutex};
						std::cout << "Result: " << r << std::endl;
					}
				},
				futures_2
			)
		);
	}
}
