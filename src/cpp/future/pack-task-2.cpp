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

using std::string_literals::operator""s;
using std::placeholders::_1;
using std::placeholders::_2;

namespace mk
{

class a_task
{
private:
protected:
	std::future<void> thread;
	std::future<std::float64_t> future;
public:
	virtual ~a_task()
	{
		std::cout << "OK: task *****************************************\n";
	}
public:
	a_task()
	{
		std::cout << "======================constructor\n";
	}
public:
	std::float64_t task(std::float64_t value)
	{
		if (value < 0)
			throw std::runtime_error{"value must be positive!"};
		return value*value;
	}
public:
	mk::a_task & run(std::float64_t value)
	{
		std::packaged_task<std::float64_t(mk::a_task *, std::float64_t)> task{
			std::bind(
				&mk::a_task::task,
				_1,
				_2
			)
		};
		future = task.get_future();
		thread = std::async(
			std::move(task),
			this,
			value
		);
		return *this;
	}
public:
	std::float64_t get()
	{
		return future.get();
	}
};

}

int main()
try
{
	mk::a_task task1;
	mk::a_task task2;
	task1.run(7.9);
	task2.run(4.8);
	std::cout << "task3 returns:" << mk::a_task{}.run(3.3).get() << std::endl;
	std::cout << "task1 returns: " << task1.get() << std::endl;
	std::cout << "task2 returns: " << task2.get() << std::endl;
	std::cout << "task4 returns: " << mk::a_task{}.run(-3.5).get() << std::endl;
}
catch (const std::exception & e)
{
	std::cout << "=> " << e.what() << std::endl;
}
