//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <future>

int main()
{
	std::future<void> f1 = std::async(
		std::launch::async,
		[]
		{
			std::cout << "f1 .\n";
		}
	);
	std::future<float> f2 = std::async(
		std::launch::async,
		[]
		{
			std::cout << "f2 ." << std::endl;
			return 2.3f * 2.3f;
		}
	);
	f1.wait();
	f2.wait();
	f1.get();
	auto r = f2.get();
	std::cout << "r=>" << r << std::endl;
}
