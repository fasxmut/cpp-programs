//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <botan/bigint.h>
#include <iostream>

int main(int argc, char * argv[])
try
{
	if (argc != 4)
	{
		std::cerr << std::endl;
		std::cerr << std::endl;
		std::cerr << "Usage:" << std::endl;
		std::cerr << "light <light color> <light radius> <model>" << std::endl;
		std::cerr << "For example:\nlight 0xffff0000 100 man.md2" << std::endl;
		std::cerr << std::endl;
		std::cerr << std::endl;
		throw std::runtime_error{"Arguments Error"};
	}

	const testp::video::SColor light_color = Botan::BigInt{argv[1]}.to_u32bit();
	const testp::uint32_pub light_radius = Botan::BigInt{argv[2]}.to_u32bit();
	const std::string model = argv[3];

	testp::TestpubDevice * device = testp::createDevice(
		testp::video::EDT_OPENGL,
		testp::nub::dimension2du{1925, 1085},
		32,
		false,
		true,
		false,
		nullptr
	);

	if (! device)
		throw std::runtime_error{"can not open rendering window"};

	testp::video::IVideoDriver * video = device->getVideoDriver();
	testp::scene::ISceneManager * scene = device->getSceneManager();

	testp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
		nullptr,
		15.0f,
		0.05f,
		-1,
		nullptr,
		0,
		false,
		10.0f,
		false,
		true
	);
	camera->setPosition(testp::nub::vector3df{20, 20, -70});
	camera->setTarget(testp::nub::vector3df{0});
	device->getCursorControl()->setVisible(false);

	testp::scene::IAnimatedMeshSceneNode * node = scene->addAnimatedMeshSceneNode(
		scene->getMesh(model.data()),
		nullptr,
		-1,
		testp::nub::vector3df{0},
		testp::nub::vector3df{0},
		testp::nub::vector3df{1},
		false
	);
	node->setMaterialFlag(testp::video::EMF_LIGHTING, true);

	scene->addLightSceneNode(
		camera,	// attach light to camera
		testp::nub::vector3df{0, 0, 0},	// position relative to camera
		light_color,
		light_radius,
		-1
	);

	while (device->run())
	{
		if (device->isWindowActive())
		{
			video->beginScene(
				testp::video::ECBF_COLOR
				|
				testp::video::ECBF_DEPTH
				|
				testp::video::ECBF_STENCIL
				,
				testp::video::SColor{0xff123456}
			);
			scene->drawAll();
			video->endScene();
		}
		else
		{
			device->yield();
		}
	}
}
catch (const std::exception & e)
{
	std::cerr << "c++ std::exception:\n" << e.what() << std::endl;
	return 1;
}

