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

namespace vv
{
	constexpr testp::int32_pub type = 4;
		// 0: or other: default: light map
		// 1: normal map solid
		// 2: normal map transparent
		// 3: parallax map solid
		// 4: parallax map transparent
	constexpr testp::float32_pub amplitude = 15;	// normal map texture amplitude

	constexpr testp::float32_pub side = 1000;
	const testp::nub::vector3df world_center{0, 0, 0};
	constexpr testp::float32_pub thickness = 30;

	constexpr testp::float32_pub tile_size = 128;
	constexpr testp::uint32_pub tile_w_count = vv::side / vv::tile_size + 1;
	constexpr testp::uint32_pub tile_h_count = vv::side / vv::tile_size + 1;
	constexpr testp::float32_pub tile_u_count = vv::tile_w_count * 1.2f;
	constexpr testp::float32_pub tile_v_count = vv::tile_h_count * 1.2f;
}

int main(int argc, char * argv[])
{
	std::vector<std::filesystem::path> tex;
	for (int i=1; i<argc; ++i)
		tex.push_back(argv[i]);

	for (const auto & p: tex)
		std::cout << "tex: " << p << std::endl;

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{2560, 1440},
		32,
		false,
		true,
		false
	);
	testp::video::IVideoDriver * video = device->getVideoDriver();
	testp::scene::ISceneManager * scene = device->getSceneManager();

	testp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
		nullptr,
		45,
		0.1,
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

	testp::scene::IMeshManipulator * manip = scene->getMeshManipulator();
	const testp::scene::IGeometryCreator * geom = scene->getGeometryCreator();

///////////////////////////////////////////////////////////////////////////
// light

	testp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		nullptr,
		vv::world_center + testp::nub::vector3df{0, 500, 0},
		testp::video::SColor{0xfffa9f32},
		1000,
		-1
	);

///////////////////////////////////////////////////////////////////////////
// world base

	testp::scene::IMesh * base_mesh = geom->createCubeMesh(
		testp::nub::vector3df{vv::side, vv::thickness, vv::side},
		testp::scene::ECMT_6BUF_4VTX_NP
	);
	testp::scene::SAnimatedMesh * base_mesh_set = new testp::scene::SAnimatedMesh;
	base_mesh_set->addMesh(base_mesh);
	base_mesh->drop();
	base_mesh = nullptr;
	testp::scene::IAnimatedMeshSceneNode * base = scene->addAnimatedMeshSceneNode(
		base_mesh_set,
		nullptr,
		-1,
		vv::world_center + testp::nub::vector3df{0, -vv::thickness/2, 0},
		testp::nub::vector3df{0, 0, 0},
		testp::nub::vector3df{1, 1, 1},
		false
	);
	base_mesh_set->drop();
	base_mesh_set = nullptr;
	base->setMaterialType(testp::video::EMT_TRANSPARENT_ADD_COLOR);
	base->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	base->setMaterialFlag(testp::video::EMF_BACK_FACE_CULLING, false);

	if (! tex.empty())
	{
		base->setMaterialTexture(0, video->getTexture(tex.back().string().data()));
		tex.pop_back();
	}
	if (! tex.empty())
	{
		base->setMaterialTexture(1, video->getTexture(tex.back().string().data()));
		tex.pop_back();
	}

///////////////////////////////////////////////////////////////////////////
// base skin

	testp::scene::IMesh * skin_mesh = geom->createPlaneMesh(
		testp::nub::dimension2df{vv::tile_size, vv::tile_size},
		testp::nub::dimension2du{vv::tile_w_count, vv::tile_h_count},
		nullptr,
		testp::nub::dimension2df{vv::tile_u_count, vv::tile_v_count}
	);
	switch (vv::type)
	{
	case 0:
	default:
		skin_mesh = manip->createMeshWith2TCoords(skin_mesh);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		skin_mesh = manip->createMeshWithTangents(skin_mesh);
		break;
	}

	if (! skin_mesh)
		throw std::runtime_error{"no skin mesh"};

	testp::scene::SAnimatedMesh * skin_mesh_set = new testp::scene::SAnimatedMesh;
	skin_mesh_set->addMesh(skin_mesh);
	skin_mesh->drop();
	skin_mesh = nullptr;
	testp::scene::IAnimatedMeshSceneNode * skin = scene->addAnimatedMeshSceneNode(
		skin_mesh_set,
		nullptr,
		-1,
		{0,0,0},
		{0,0,0},
		{1,1,1},
		false
	);
	skin_mesh_set->drop();
	skin_mesh_set = nullptr;
	if (! skin)
		throw std::runtime_error{"No skin"};

	if (! tex.empty())
	{
		skin->setMaterialTexture(0, video->getTexture(tex.back().string().data()));
		tex.pop_back();
		std::cout << "skin texture 0 is set" << std::endl;
	}

	if (! tex.empty())
	{
		testp::video::ITexture * tex1 = video->getTexture(tex.back().string().data());
		tex.pop_back();
		if (tex1)
		{
			switch (vv::type)
			{
			case 0:
			default:
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				video->makeNormalMapTexture(tex1, vv::amplitude);
				break;
			}
		}
		if (tex1)
		{
			skin->setMaterialTexture(1, tex1);
			std::cout << "skin texture 1 is set" << std::endl;
		}
	}

	switch (vv::type)
	{
	case 0:
	default:
		skin->setMaterialType(testp::video::EMT_LIGHTMAP_LIGHTING_M2);
		break;
	case 1:
		skin->setMaterialType(testp::video::EMT_NORMAL_MAP_SOLID);
		break;
	case 2:
		skin->setMaterialType(testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA);
		break;
	case 3:
		skin->setMaterialType(testp::video::EMT_PARALLAX_MAP_SOLID);
		break;
	case 4:
		skin->setMaterialType(testp::video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA);
		break;
	}

///////////////////////////////////////////////////////////////////////////

	testp::scene::ISceneNodeAnimator * ani = scene->createFlyStraightAnimator(
		vv::world_center + testp::nub::vector3df{0, 90, 0},
		vv::world_center + testp::nub::vector3df{0, -0.01, 0},
		5000,
		false,
		false
	);

	skin->setPosition(
		vv::world_center + testp::nub::vector3df{0, 90, 0}
	);

	skin->addAnimator(ani);
	ani->drop();
	ani = nullptr;

///////////////////////////////////////////////////////////////////////////

	camera->setPosition(
		testp::nub::vector3df{
			vv::world_center + testp::nub::vector3df{vv::side/2+100, 110, vv::side/2-100}
		}
	);
	camera->setTarget(
		testp::nub::vector3df{
			vv::world_center
		}
	);

///////////////////////////////////////////////////////////////////////////

	testp::scene::ITriangleSelector * selector = scene->createOctreeTriangleSelector(
		base->getMesh(),
		base,
		256
	);
	base->setTriangleSelector(selector);
	testp::scene::IMetaTriangleSelector * meta = scene->createMetaTriangleSelector();
	meta->addTriangleSelector(selector);
	selector->drop();
	selector = nullptr;

	ani = scene->createCollisionResponseAnimator(
		meta,
		camera,
		testp::nub::vector3df{30, 50, 30},
		testp::nub::vector3df{0, 0, 0},
		testp::nub::vector3df{0, 0, 0},
		0.001f
	);
	camera->addAnimator(ani);
	ani->drop();
	ani = nullptr;

///////////////////////////////////////////////////////////////////////////

	ani = scene->createFlyCircleAnimator(
		vv::world_center + testp::nub::vector3df{0, 300, 0},
		300,
		0.0003,
		vv::world_center + testp::nub::vector3df{0, 600, 0},
		0,
		0
	);
	light->addAnimator(ani);
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
	device = nullptr;
}

