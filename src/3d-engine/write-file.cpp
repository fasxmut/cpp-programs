//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <jimcpp/core.hpp>
#include <iostream>

int main()
{
	auto device =jpp::createDevice();
	auto scene = device->getSceneManager();

	jpp::io::IFileSystem * fs = device->getFileSystem();

	{
		// append: false
		jpp::io::IWriteFile * file = fs->createAndWriteFile("hello.cpp", false);

		const std::string str = R"(//
			#include <iostream>
			int main()
			{
				std::cout << "Hello, c++!" << std::endl;
			}
		)";

		file->write(str.data(), str.size());
	}
}

