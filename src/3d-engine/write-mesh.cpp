//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <vector>
#include <botan/bigint.h>

namespace cpp
{
	class engine
	{
	private:
		testp::TestpubDevice * __device{nullptr};
		testp::scene::ICameraSceneNode * __fps{nullptr};
		testp::gui::ICursorControl * __cursor{nullptr};
	public:
		virtual ~engine()
		{
			if (__device)
			{
				__device->drop();
				__device = nullptr;
				std::cout << "dropped __device :::: by destructor" << std::endl;
			}
			std::cout << "Renderer is closed" << std::endl;
		}
	public:
		engine() = delete;
		engine(testp::uint32_kt width__, testp::uint32_kt height__):
			__device{
				testp::createDevice(
					testp::video::EDT_OPENGL,
					testp::nub::dimension2du{width__, height__},
					32,
					false,
					true,
					false,
					nullptr
				)
			}
		{
			if (! __device)
				throw std::runtime_error{"Can not open render window"};
			this->Init();
		}
	public:
		auto device()
		{
			return __device;
		}
	public:
		auto fs()
		{
			return this->device()->getFileSystem();
		}
	public:
		auto scene()
		{
			return this->device()->getSceneManager();
		}
	public:
		auto video()
		{
			return this->device()->getVideoDriver();
		}
	public:
		auto fps()
		{
			if (__fps)
				return __fps;
			// else
			__fps = this->scene()->addCameraSceneNodeFPS(
				nullptr,
				20.0f,
				0.08f,
				-1,
				nullptr,
				0,
				false,
				15.0f,
				false,
				true
			);
			__fps->setPosition(testp::nub::vector3df{5,2,-40});
			__fps->setTarget(testp::nub::vector3df{0,5,0});
			return __fps;
		}
	public:
		auto cursor()
		{
			if (__cursor)
				return __cursor;
			__cursor = this->device()->getCursorControl();
			return __cursor;
		}
	private:
		void Init()
		{
			this->fps();
			this->cursor()->setVisible(false);
		}
	public:
		void run()
		{
			while (this->device()->run())
			{
				if (this->device()->isWindowActive())
				{
					this->video()->beginScene(
						testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL,
						testp::video::SColor{0xff123456}
					);
					this->scene()->drawAll();
					this->video()->endScene();
				}
				else
				{
					this->device()->yield();
				}
			}
		}
	public:
		void drop_device()
		{
			if (__device)
			{
				__device->drop();
				__device = nullptr;
				std::cout << "dropped __device :::: by drop_devie" << std::endl;
			}
		}
	};

	class my_mesh:
		virtual public testp::scene::SMesh
	{
	public:
		my_mesh()
		{
			this->Init();
		}
	private:
		void Init()
		{
			std::vector<testp::video::S3DVertex> vertices{
				{0,10,-10,		0,-1,-1,		0xffff0000,		0.5,0.5},	// 0
				{20,0,0,		0,-1,-1,		0xffff0000,		1,1},		// 1
				{-20,0,0,		0,-1,-1,		0xffff0000,		0,1},		// 2

				{0,10,-10,		1,1,-1,		0xff00ff00,		0.5,0.5},	// 3
				{0,20,0,		1,1,-1,		0xff00ff00,		0,0},	// 4
				{20,0,0,		1,1,-1,		0xff00ff00,		1,0},	// 5

				{0,10,-10,		-1,1,-1,		0xff0000ff,		0.5,0.5},	// 6
				{-20,0,0,		-1,1,-1,		0xff0000ff,		0,1},		// 7
				{0,20,0,		-1,1,-1,		0xff0000ff,		0,0},	// 8

				{-20,0,0,		0,0,1,		0xffffff00,		0.5,0.5},		// 9
				{20,0,0,		0,0,1,		0xffffff00,		1,1},	// 10
				{0,20,0,		0,0,1,		0xffffff00,		1,0},	// 11
			};

			std::vector<testp::uint16_kt> indices{
				0,1,2,
				3,4,5,
				6,7,8,
				9,10,11
			};

			testp::scene::SMeshBuffer * buffer = new testp::scene::SMeshBuffer;
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
	if (argc != 2)
	{
		std::cerr << "usage:\n\n" << "write-mesh <texture image>\n\n";
		throw std::runtime_error{"arguments error"};
	}

	cpp::engine engine{1925, 1085};
	auto my_mesh = new cpp::my_mesh;
	testp::scene::IMeshSceneNode * node =engine.scene()->addMeshSceneNode(
		my_mesh,
		nullptr,
		-1,
		testp::nub::vector3df{0},
		testp::nub::vector3df{0},
		testp::nub::vector3df{1},
		false
	);
	my_mesh->drop();
	if (node)
	{
		node->setMaterialFlag(testp::video::EMF_LIGHTING, true);
		node->setMaterialTexture(0, engine.video()->getTexture(argv[1]));
	}
	engine.scene()->addLightSceneNode(
		engine.fps(),
		testp::nub::vector3df{0},
		testp::video::SColor{0xffffffff},
		10000,
		-1
	);

	const std::string output = "output.b3d";
	bool write_status = false;
	{
		testp::scene::IMeshWriter * mesh_writer = engine.scene()->createMeshWriter(
			testp::scene::EMWT_B3D
		);
		testp::io::IWriteFile * output_file = engine.fs()->createAndWriteFile(
			output.data(),
			false
		);
		write_status = mesh_writer->writeMesh(
			output_file,
			node->getMesh(),
			testp::scene::EMWF_WRITE_BINARY | testp::scene::EMWF_WRITE_COMPRESSED
		);
		if (write_status)
			std::cout << "Mesh has been written to " << output << " successful!" << std::endl;
		else
			std::cout << "Write mesh failed" << std::endl;
	}

	engine.run();
	engine.drop_device();

	if (write_status)
	{
		std::cout << "Mesh has been written to " << output
			<< ", do you want to load and render it now? (Y/n) " << std::flush;
		std::string answer;
		std::getline(std::cin, answer);
		if (answer != "y" && answer != "Y")
			return 0;
	}

///////////////////////////////////////////////////////////////////////////

	cpp::engine engine2{1925, 1085};

	testp::scene::IAnimatedMeshSceneNode * node2 = engine2.scene()->addAnimatedMeshSceneNode(
		engine2.scene()->getMesh(output.data()),
		nullptr,
		-1,
		testp::nub::vector3df{0},
		testp::nub::vector3df{0},
		testp::nub::vector3df{1},
		true
	);

	if (node2)
	{
		node2->setMaterialTexture(0, engine2.video()->getTexture(argv[1]));
		node2->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	}
	engine2.scene()->addLightSceneNode(
		engine2.fps(),
		testp::nub::vector3df{0},
		testp::video::SColor{0xff0000ff},
		10000,
		-1
	);

	engine2.run();

	return 0;
}
catch (const std::exception & e)
{
	std::cerr << "\n\n" << "c++ std::exception:\n" << e.what() << std::endl << std::endl;
	return 1;
}

