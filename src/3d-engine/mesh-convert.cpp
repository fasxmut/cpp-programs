//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <jimcpp/core.hpp>
#include <iostream>

using std::string_literals::operator""s;

jpp::JimcppDevice * device = nullptr;

int main(int argc, char * argv[])
try
{
	if (argc != 4)
	{
		std::cerr << "----------------------------------------\n";
		std::cerr << "usage:\n\nmesh-convert <input mesh> <output mesh> <binary?>\n\n";
		std::cerr << "----------------------------------------\n";
		std::cerr << "<input mesh> should be static mesh, otherwise only read first frame\n";
		std::cerr << "<output mesh> supports .b3d mesh only for this program\n";
		std::cerr << "<binary> must be true or false, other values are not allowed\n";
		std::cerr << "----------------------------------------\n";
		std::cerr << "For example:\n\n";
		std::cerr << "mesh-writer cube.ms3d cube.b3d true" << std::endl;
		std::cerr << "mesh-writer cube.ms3d cube.b3d false" << std::endl;
		std::cerr << "----------------------------------------\n";

		throw std::runtime_error{"arguments error"};
	}

	bool binary;
	if ("true"s == argv[3])
		binary = true;
	else if ("false"s == argv[3])
		binary = false;
	else
		throw std::runtime_error{"binary flag must be true or false"};

	const std::string input = argv[1];
	const std::string output = argv[2];

	if (! output.ends_with(".b3d"))
		throw std::runtime_error{"output only supports .b3d extension"};

	std::cout << "\n\n";
	std::cout << "input mesh: " << input << std::endl;
	std::cout << "output mesh: " << output << std::endl;
	std::cout << "binary format: " << std::boolalpha << binary << std::endl;
	std::cout << "\n\n";

	{
		if (device)
			device->drop();
		device = jpp::createDevice(jpp::video::EDT_NULL);
		auto fs = device->getFileSystem();
		auto scene = device->getSceneManager();
		jpp::scene::IAnimatedMesh * mesh = scene->getMesh(input.data());
		if (! mesh)
			throw std::runtime_error{"Mesh Error: Invalid input mesh, is it correct?"};
		if (mesh->getMeshBufferCount() < 1)
			throw std::runtime_error{"Mesh Buffer Error: Invalid input mesh, is it correct?"};
		std::cout << "\n----------------------------------------\n";
		std::cout << "Input mesh is loaded\n";
		std::cout << "----------------------------------------\n\n";

		jpp::scene::IMeshWriter * mesh_writer = scene->createMeshWriter(
			jpp::scene::EMWT_B3D
		);
		jpp::io::IWriteFile * out_file = fs->createAndWriteFile(output.data(), false);
		bool status = false;
		if (binary)
		{
			status = mesh_writer->writeMesh(
				out_file,
				mesh->getMesh(0),
				jpp::scene::EMWF_WRITE_BINARY | jpp::scene::EMWF_WRITE_COMPRESSED
			);
		}
		else
		{
			status = mesh_writer->writeMesh(
				out_file,
				mesh->getMesh(0),
				jpp::scene::EMWF_WRITE_COMPRESSED
			);
		}

		mesh_writer->drop();
		out_file->drop();
		if (device)
		{
			device->drop();
			device = nullptr;
		}

		if (status)
		{
		std::cout << "\n----------------------------------------\n";
		std::cout << "Output mesh is written\n";
		std::cout << "----------------------------------------\n\n";
		}
		else
		{
			throw std::runtime_error{"Write Mesh Error"};
		}
	}

	{
		std::cout << "Do you want to render the output mesh? (Y/n) " << std::flush;
		std::string answer;
		std::getline(std::cin, answer);
		if (answer != "y" && answer != "Y")
			return 0;
	}
	// else
	{
		if (device)
			device->drop();
		device = jpp::createDevice(
			jpp::video::EDT_OPENGL,
			jpp::core::dimension2du{1925, 1085},
			32,
			false,
			true,
			false,
			nullptr
		);
		jpp::scene::ISceneManager * scene = device->getSceneManager();
		jpp::video::IVideoDriver * video = device->getVideoDriver();

		jpp::scene::IAnimatedMesh * mesh = scene->getMesh(output.data());
		if (! mesh || mesh->getMesh(0)->getMeshBufferCount() < 1)
			throw std::runtime_error{"Mesh Error: can not open mesh or mesh is empty !"};
		// else
		std::cout << "Found mesh buffer (for first frame) count: "
			<< mesh->getMesh(0)->getMeshBufferCount() << std::endl;
		jpp::scene::IAnimatedMeshSceneNode * node = scene->addAnimatedMeshSceneNode(
			mesh,
			nullptr,
			-1,
			jpp::core::vector3df{0},
			jpp::core::vector3df{0},
			jpp::core::vector3df{1},
			false
		);
		if (node)
		{
			node->setMaterialFlag(jpp::video::EMF_LIGHTING, true);
		}
		jpp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
			nullptr,
			20.0f,
			0.05f,
			-1,
			nullptr,
			0,
			false,
			10.0f,
			false,
			true
		);
		camera->setPosition({80,80,80});
		camera->setTarget({0,0,0});
		device->getCursorControl()->setVisible(false);
		scene->addLightSceneNode(
			camera,
			jpp::core::vector3df{0},
			jpp::video::SColor{0xff00ff00},
			10000,
			-1
		);
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

		if (device)
		{
			device->drop();
			device = nullptr;
		}
	}

	return 0;
}
catch (const std::exception & e)
{
	if (device)
	{
		device->drop();
		device = nullptr;
	}
	std::cerr << "----------------------------------------\n";
	std::cerr << "ERROR:\n\n";
	std::cerr << "c++ std::exception:\n" << e.what() << std::endl << std::endl;
}

