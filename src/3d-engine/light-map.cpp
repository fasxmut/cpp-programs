//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <lyra/lyra.hpp>
#include <filesystem>

int main(int argc, char * argv[])
{
	std::filesystem::path tex0;
	std::filesystem::path tex1;

	bool help = false;

	auto cli = lyra::help(help)
		| lyra::opt(tex0, "tex0")["-0"]("tex0 for both left wallet texture 0 and right wallet texture 0")
		| lyra::opt(tex1, "tex1")["-1"]("tex1 for right wallet texture 1")
	;
	if (help || argc == 1 || ! cli.parse({argc, argv}))
	{
		std::cout << cli << std::endl;
		return 0;
	}

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{2560, 1440},
		32,
		false,
		true,
		false,
		nullptr
	);
	testp::video::IVideoDriver * video = device->getVideoDriver();
	testp::scene::ISceneManager * scene = device->getSceneManager();
	testp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
		nullptr,
		75,
		0.12,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);
	camera->setPosition(testp::nub::vector3df{500,0,0});
	camera->setTarget(testp::nub::vector3df{-500,0,0});
	testp::gui::ICursorControl * cursor = device->getCursorControl();
	cursor->setVisible(false);

	const testp::scene::IMeshManipulator * const manip = scene->getMeshManipulator();

	const testp::scene::IGeometryCreator * const geo = scene->getGeometryCreator();

///////////////////////////////////////////////////////////////////////////

	testp::scene::IMesh * left_mesh_frame = geo->createCubeMesh(
		testp::nub::vector3df{1000, 500, 10},
		testp::scene::ECMT_6BUF_4VTX_NP
	);
	testp::scene::SAnimatedMesh * left_mesh = new testp::scene::SAnimatedMesh;
	left_mesh->addMesh(left_mesh_frame);
	left_mesh_frame->drop();
	testp::scene::IAnimatedMeshSceneNode * left = scene->addAnimatedMeshSceneNode(
		left_mesh,
		nullptr,
		-1,
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{1,1,1},
		false
	);
	left_mesh->drop();
	left->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	left->setMaterialTexture(0, video->getTexture(tex0.string().data()));
	left->setPosition({0, 0, -200});

///////////////////////////////////////////////////////////////////////////

	testp::scene::IMesh * right_mesh_frame = geo->createCubeMesh(
		testp::nub::vector3df{1000, 500, 10},
		testp::scene::ECMT_6BUF_4VTX_NP
	);
	testp::scene::IMesh * right_mesh_frame_td = manip->createMeshWith2TCoords(right_mesh_frame);
	right_mesh_frame->drop();
	testp::scene::SAnimatedMesh * right_mesh = new testp::scene::SAnimatedMesh;
	right_mesh->addMesh(right_mesh_frame_td);
	right_mesh_frame_td->drop();
	testp::scene::IAnimatedMeshSceneNode * right = scene->addAnimatedMeshSceneNode(
		right_mesh,
		nullptr,
		-1,
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{1,1,1},
		false
	);
	right_mesh->drop();
	right->setPosition(testp::nub::vector3df{0, 0, 200});
	right->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	right->setMaterialTexture(0, video->getTexture(tex0.string().data()));
	right->setMaterialTexture(1, video->getTexture(tex1.string().data()));
	right->setMaterialType(testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA);

	testp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{0, 0, 0},
		testp::video::SColor{0xffffffff},
		3000,
		-1
	);

	testp::scene::ISceneNodeAnimator * ani = scene->createFlyCircleAnimator(
		testp::nub::vector3df{0, 0, 0},
		1000,
		0.003,
		testp::nub::vector3df{0, 1, 0},
		0,
		0
	);
	light->addAnimator(ani);
	ani->drop();

///////////////////////////////////////////////////////////////////////////

	light->setLightType(testp::video::ELT_POINT);

	while (device->run())
	{
		if (! device->isWindowActive())
		{
			device->yield();
			continue;
		}
		video->beginScene(
			testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL,
			testp::video::SColor{0xff123456}
		);
		scene->drawAll();
		video->endScene();
	}

	device->drop();
}

