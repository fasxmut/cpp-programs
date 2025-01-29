//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <jimcpp/core.hpp>
#include <vector>

using std::string_literals::operator""s;

namespace my_mesh
{
	class rectangle_mesh: virtual public jpp::scene::SMesh
	{
	private:
		bool __reverse_order;
	public:
		virtual ~rectangle_mesh()
		{
			std::cout << "rectangle mesh is removed" << std::endl;
		}
	public:
		rectangle_mesh(bool reverse_order__):
			__reverse_order{reverse_order__}
		{
			this->create();
		}
	protected:
		inline void create()
		{
			std::vector<jpp::video::S3DVertex> vertices{
				{10,-10,0,    0,0,-1,    0xffff0000,    1,0},
				{10,10,0,    0,0,-1,    0xff00ff00,    1,1},
				{-10,10,0,    0,0,-1,    0xff0000ff,    0,1},
				{-10,-10,0,    0,0,-1,    0xffffffff,    0,0}
			};
			std::vector<jpp::u16> indices;

			if (__reverse_order)
				indices = std::vector<jpp::u16>{
					0,1,2,
					0,2,3,
				};
			else
				indices = std::vector<jpp::u16>{
					0,2,1,
					0,3,2
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
	};
}

int main(int argc, char * argv[])
try
{
	bool reverse_order = false;
	bool enable_back_face_culling = true;

	if (argc != 3)
	{
		std::cout << "usage:\n\n";
		std::cout << "mesh-buffer-rectangle <enable_back_face_culling> <reverse_order>\n\n"
			<< "For example:\nmesh-buffer-rectangle true false\n";
		std::cout << std::endl;
		throw std::runtime_error{"arguements error"};
	}
	if ("true"s == argv[1])
		enable_back_face_culling = true;
	else if ("false"s == argv[1])
		enable_back_face_culling = false;
	else
		throw std::runtime_error{"arguments error"};

	if ("true"s == argv[2])
		reverse_order = true;
	else if ("false"s == argv[2])
		reverse_order = false;
	else
		throw std::runtime_error{"arguments error"};

	jpp::JimcppDevice * device = jpp::createDevice(
		jpp::video::EDT_OPENGL,
		jpp::core::dimension2du{1925, 1085},
		32,
		false,
		true,
		false,
		nullptr
	);

	auto scene = device->getSceneManager();
	auto video = device->getVideoDriver();

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
	camera->setPosition({0,2,-25});
	camera->setTarget({0,0,0});
	device->getCursorControl()->setVisible(false);

	auto mesh = new my_mesh::rectangle_mesh{reverse_order};

	auto node = scene->addMeshSceneNode(
		mesh,
		nullptr,
		-1,
		{0,0,0},
		{0,0,0},
		{1,1,1},
		false
	);

	if (node)
	{
		jpp::s32 mb_count = node->getMesh()->getMeshBufferCount();
		std::cout << "mb_count: " << mb_count << std::endl;
		if (mb_count > 0)
		{
			node->setMaterialFlag(jpp::video::EMF_LIGHTING, false);
			node->setMaterialFlag(jpp::video::EMF_BACK_FACE_CULLING, enable_back_face_culling);
		}
	}
	else
		std::cout << "no node" << std::endl;

	mesh->drop();

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

