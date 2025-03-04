//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <testpub/core.hpp>
#include <vector>
#include <filesystem>
#include <queue>

namespace vv
{
	const testp::nub::vector3df ground_center{0,0,0};
	const testp::float32_pub thickness = 20;
	const testp::float32_pub ground_side = 1000;

	const testp::float32_pub tile_size = 300;
	const testp::uint32_pub tile_count_w = ground_side / tile_size;
	const testp::uint32_pub tile_count_h = ground_side / tile_size;
	const testp::float32_pub tile_count_u = tile_count_w * 1.2;
	const testp::float32_pub tile_count_v = tile_count_h * 1.2;

	constexpr int visible = 0;
			//	0: both
			//	1: node
			//	2: skin

	constexpr int map = 4;
			// 0 or other: no normal map
			// 1: normal solid
			// 2: normal transparent
			// 3: parallax solid
			// 4: paralllax transparent

	constexpr testp::float32_pub amplitude = 20000;
	constexpr testp::float32_pub light_height = 500;
	constexpr testp::float32_pub light_x = 500;
}	// namespace vv

int main(int argc, char * argv[])
{
	if (argc == 1)
	{
		std::cout << argv[0] << " tex0 tex1 tex2 ..." << std::endl;
		return 0;
	}

	std::queue<std::filesystem::path> tex;
	for (int i=1; i<argc; ++i)
		tex.push(argv[i]);

	std::cout << "Got textures: " << tex.size() << std::endl;

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
		50.0f,
		0.1f,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);

	testp::gui::ICursorControl * cursor = device->getCursorControl();
	cursor->setVisible(false);

	testp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		nullptr,
		vv::ground_center + testp::nub::vector3df{vv::light_x, vv::light_height, 0},
		testp::video::SColor{0xffa89362},
		1500,
		-1
	);

	const testp::scene::IGeometryCreator * geom = scene->getGeometryCreator();

	testp::scene::IMeshManipulator * manip = scene->getMeshManipulator();

///////////////////////////////////////////////////////////////////////////

	testp::scene::IMesh * mesh = geom->createCubeMesh(
		testp::nub::vector3df{vv::ground_side, vv::thickness, vv::ground_side},
		testp::scene::ECMT_6BUF_4VTX_NP
	);

	testp::scene::IMesh * mesh_lm = nullptr;

	switch (vv::map)
	{
	case 1:
	case 2:
	case 3:
	case 4:
		mesh_lm = manip->createMeshWithTangents(mesh);
		break;
	default:
		mesh_lm = manip->createMeshWith2TCoords(mesh);
		break;
	}
	mesh->drop();
	mesh = nullptr;

	testp::scene::SAnimatedMesh * a_mesh = new testp::scene::SAnimatedMesh;
	a_mesh->addMesh(mesh_lm);
	mesh_lm->drop();
	mesh_lm = nullptr;

	testp::scene::IAnimatedMeshSceneNode * node = scene->addAnimatedMeshSceneNode(
		a_mesh,
		nullptr,
		-1,
		vv::ground_center - testp::nub::vector3df{0, vv::thickness/2, 0},
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{1,1,1},
		false
	);
	a_mesh->drop();
	a_mesh = nullptr;
	node->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	if (! tex.empty())
	{
		node->setMaterialTexture(0, video->getTexture(tex.front().string().data()));
		tex.pop();
	}
	if (! tex.empty())
	{
		auto tex__ = video->getTexture(tex.front().string().data());
		switch (vv::map)
		{
		case 1:
		case 2:
		case 3:
		case 4:
			video->makeNormalMapTexture(tex__, vv::amplitude);
			break;
		default:
			break;
		}
		node->setMaterialTexture(1, tex__);
		tex.pop();
	}

	switch (vv::map)
	{
	case 1:
		node->setMaterialType(testp::video::EMT_NORMAL_MAP_SOLID);
		break;
	case 2:
		node->setMaterialType(testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA);
		break;
	case 3:
		node->setMaterialType(testp::video::EMT_PARALLAX_MAP_SOLID);
		break;
	case 4:
		node->setMaterialType(testp::video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA);
		break;
	default:
		node->setMaterialType(testp::video::EMT_LIGHTMAP_LIGHTING_M2);
		break;
	}

///////////////////////////////////////////////////////////////////////////

	testp::scene::IMesh * skin_mesh = geom->createPlaneMesh(
		testp::nub::dimension2df{vv::tile_size, vv::tile_size},
		testp::nub::dimension2du{vv::tile_count_w, vv::tile_count_h},
		nullptr,
		testp::nub::dimension2df{vv::tile_count_u, vv::tile_count_v}
	);

	testp::scene::SAnimatedMesh * ani_skin_mesh = new testp::scene::SAnimatedMesh;
	ani_skin_mesh->addMesh(skin_mesh);
	skin_mesh->drop();
	skin_mesh = nullptr;
	testp::scene::IAnimatedMeshSceneNode * skin = scene->addAnimatedMeshSceneNode(
		ani_skin_mesh,
		nullptr,
		-1,
		vv::ground_center + testp::nub::vector3df{0, 0.01, 0},
		testp::nub::vector3df{0, 0, 0},
		testp::nub::vector3df{1,1,1},
		false
	);

	skin->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	skin->setMaterialType(testp::video::EMT_TRANSPARENT_ADD_COLOR);

	if (! tex.empty())
	{
		skin->setMaterialTexture(0, video->getTexture(tex.front().string().data()));
		tex.pop();
		std::cout << "Added skin texture 0" << std::endl;
	}

	if (! tex.empty())
	{
		skin->setMaterialTexture(1, video->getTexture(tex.front().string().data()));
		tex.pop();
		std::cout << "Added skin texture 1" << std::endl;
	}
	else
		std::cout << "No texture for skin texture 1" << std::endl;

///////////////////////////////////////////////////////////////////////////

	switch (vv::visible)
	{
	case 0:
		node->setVisible(true);
		skin->setVisible(true);
		break;
	case 1:
		node->setVisible(true);
		skin->setVisible(false);
		break;
	case 2:
		node->setVisible(false);
		skin->setVisible(true);
		break;
	default:
		break;
	}

///////////////////////////////////////////////////////////////////////////

	testp::scene::ITriangleSelector * selector = scene->createOctreeTriangleSelector(
		node->getMesh(),
		node,
		128
	);
	node->setTriangleSelector(selector);

	testp::scene::ISceneNodeAnimator * collision = scene->createCollisionResponseAnimator(
		selector,
		camera,
		testp::nub::vector3df{30, 50, 30},
		testp::nub::vector3df{0, 0, 0},
		testp::nub::vector3df{0, 0, 0},
		0.001f
	);
	selector->drop();
	selector = nullptr;

	camera->addAnimator(collision);
	collision->drop();
	collision = nullptr;

///////////////////////////////////////////////////////////////////////////

	testp::scene::ISceneNodeAnimator * ani = scene->createFlyCircleAnimator(
		vv::ground_center + testp::nub::vector3df{0, vv::light_height, 0},
		vv::light_x,
		0.0007,
		vv::ground_center + testp::nub::vector3df{0, vv::light_height*2, 0},
		0,
		0
	);
	light->addAnimator(ani);
	ani->drop();
	ani = nullptr;

///////////////////////////////////////////////////////////////////////////

	camera->setPosition(
		testp::nub::vector3df{vv::ground_side/2-300, 200, vv::ground_side/2-100}
	);
	camera->setTarget(
		testp::nub::vector3df{0, 100, 0}
	);

///////////////////////////////////////////////////////////////////////////

	ani = scene->createFlyStraightAnimator(
		testp::nub::vector3df{0, 50 + camera->getPosition().Y, 0},
		vv::ground_center + testp::nub::vector3df{0, 0.01, 0},
		15000,
		false,	// not loop
		false	// not pingpong
	);
	skin->addAnimator(ani);
	ani->drop();
	ani = nullptr;

///////////////////////////////////////////////////////////////////////////

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

