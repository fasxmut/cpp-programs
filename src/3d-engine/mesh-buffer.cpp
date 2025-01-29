//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <jimcpp/core.hpp>
#include <iostream>
#include <vector>

namespace my_mesh
{
	class cube_mesh:
		virtual public jpp::scene::SMesh
	{
	public:
		cube_mesh()
		{
			this->create();
		}
	public:
		virtual ~cube_mesh()	// destructor is called by reference-counter
		{
			std::cout << "cube mesh is removed" << std::endl;
		}
	protected:
		inline void create()
		{
			std::vector<jpp::video::S3DVertex> vertices{
				{10, -10, -10,    1,-1,-1,    0xffff0000,    0.5,0},	// 0
				{10,10,-10,    1,1,-1,    0xffff9999,    0.5,0.5},	// 1
				{-10,10,-10,    -1,1,-1,    0xff00ff00,    0,0.5},	// 2
				{-10,-10,-10,    -1,-1,-1,    0xff0000ff,    0,0},	// 3
				{10,-10,10,    1,-1,1,    0xffffff00,    1,0.5},	// 4
				{10,10,10,    1,1,1,    0xff00ffff,    1,1},	// 5
				{-10,10,10,    -1,1,1,    0xff00ff00,    0.5,1},	// 6
				{-10,-10,10,    -1,-1,1,    0xff00ffff,    0.5,0.5}	// 7
			};
			std::vector<jpp::u16> indices{
				0,1,2,
				0,2,3,
				0,4,5,
				0,5,1,
				1,5,6,
				1,6,2,
				2,6,7,
				2,7,3,
				4,7,6,
				4,6,5,
				0,3,7,
				0,7,4
			};
			auto buffer = new jpp::scene::SMeshBuffer;
			buffer->append(
				vertices.data(),
				vertices.size(),
				indices.data(),
				indices.size()
			);
			this->addMeshBuffer(buffer);
			buffer->drop();
			this->setDirty();
			this->recalculateBoundingBox();
		}
	};	// class cube_mesh
}	// namespace my_mesh

int main(int argc, char * argv[])
try
{
	if (argc != 2)
	{
		std::cerr << "usage:\nmesh-buffer <texture>\n";
		std::cerr << "A texture mesh-buffer.texture.png can be found at the source code\n";
		throw std::runtime_error{"Arguments error"};
	}
	std::string texture = argv[1];
	jpp::JimcppDevice * device = jpp::createDevice(
		jpp::video::EDT_OPENGL,
		jpp::core::dimension2du{1925, 1085},
		32,
		false,
		true,
		false,
		nullptr
	);
	if (! device)
		throw std::runtime_error{"can not open rendering window"};
	auto video = device->getVideoDriver();
	auto scene = device->getSceneManager();

	auto cube_mesh = new my_mesh::cube_mesh;
	auto node = scene->addMeshSceneNode(
		cube_mesh,
		nullptr,
		-1,
		jpp::core::vector3df{0},
		jpp::core::vector3df{0},
		jpp::core::vector3df{1},
		false
	);
	cube_mesh->drop();
	std::cout << "cube mesh is dropped" << std::endl;

	if (node)
	{
		jpp::u32 mb_count = node->getMesh()->getMeshBufferCount();
		std::cout << "mb_count = " << mb_count << std::endl;
		if (mb_count > 0)
		{
			node->setMaterialFlag(jpp::video::EMF_LIGHTING, true);
			node->setMaterialFlag(jpp::video::EMF_BACK_FACE_CULLING, false);
			bool enable_texture = true;
			if (enable_texture)
				node->setMaterialTexture(0, video->getTexture(texture.data()));
		}
	}
	else
		std::cout << "no node" << std::endl;

	auto camera = scene->addCameraSceneNodeFPS(
		nullptr,
		20.0f,
		0.02f,
		-1,
		nullptr,
		0,
		false,
		10.0f,
		false,
		true
	);
	camera->setPosition(jpp::core::vector3df{25, 15, -25});
	camera->setTarget(jpp::core::vector3df{0, 0, 0});

	jpp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		camera,
		jpp::core::vector3df{0},
		jpp::video::SColor{0xff787878},
		1000,
		-1
	);
	light->setPosition(jpp::core::vector3df{0});
	device->getCursorControl()->setVisible(false);

	while (device->run())
	{
		if (device->isWindowActive())
		{
			video->beginScene(
				jpp::video::ECBF_COLOR | jpp::video::ECBF_DEPTH | jpp::video::ECBF_STENCIL,
				jpp::video::SColor{0xff123456}
			);
			scene->drawAll();
			video->endScene();
		}
		else
		{
			device->yield();
		}
	}
	device->drop();
	return 0;
}
catch (const std::exception & e)
{
	std::cerr << "c++ std::exception:\n" << e.what() << std::endl;
	return 1;
}

