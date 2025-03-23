//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <future>
#include <iostream>
#include <stdfloat>
#include <functional>
#include <random>

using std::placeholders::_1;
using std::placeholders::_2;
using std::string_literals::operator""s;

namespace mk
{

std::random_device rnd;
std::mt19937 rng{rnd()};

class a_task
{
private:
protected:
	std::future<std::bfloat16_t> future;
	std::future<void> thread;
public:
	virtual ~a_task()
	{
		std::cout << "stauts is OK: destructor" << std::endl;
	}
protected:
	std::bfloat16_t task(std::bfloat16_t x, std::bfloat16_t y)
	{
		switch (rng() % 3)
		{
		case 0:
			x += 0.1bf16;
			break;
		case 1:
			y -= 0.2bf16;
			break;
		case 2:
			throw std::runtime_error{"status is OK: test"};
		default:
			break;
		}
		return x*y;
	}
public:
	void run(std::bfloat16_t x, std::bfloat16_t y)
	{
		std::packaged_task<std::bfloat16_t(std::bfloat16_t, std::bfloat16_t)> task{
			std::bind(
				&mk::a_task::task,
				this,
				_1,
				_2
			)
		};
		future = task.get_future();
		thread = std::async(
			std::move(task),
			x,
			y
		);
	}
public:
	std::bfloat16_t get()
	{
		return future.get();
	}
};

}

int main()
try
{
	mk::a_task task;
	task.run(2.332bf16, -7.2bf16);
	std::cout << "I got value: " << static_cast<float>(task.get()) << std::endl;
	throw std::runtime_error{"status is OK"};
}
catch (const std::exception & e)
{
	std::cout << "=> " << e.what() << std::endl;
}
