//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <lyra/lyra.hpp>
#include <iostream>
#include <testpub/core.hpp>

int main(int argc, char ** argv)
try
{
	std::string map_fs;
	std::string map_name;
	std::string model_name;

	bool help = false;
	auto cli = lyra::help(help)
		| lyra::opt(map_fs, "map fs")["-f"]("Map File System")
		| lyra::opt(map_name, "map name")["-m"]("Map Name")
		| lyra::opt(model_name, "model name")["-d"]("Shadered Model Name")
	;

	auto result = cli.parse({argc, argv});

	if (help || argc == 1 || ! result)
	{
		std::cout << cli << std::endl;
		return 0;
	}

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_OPENGL,
		testp::nub::dimension2du{1920, 1080},
		32,
		false,
		true,
		false,
		nullptr
	);
	if (! device)
		throw std::runtime_error{"device error"};
	testp::video::IVideoDriver * video = device->getVideoDriver();
	testp::scene::ISceneManager * scene = device->getSceneManager();

	testp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
		nullptr,
		30,
		0.07,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);
	device->getCursorControl()->setVisible(false);

	testp::io::IFileSystem * fs = device->getFileSystem();

	if (! fs->addFileArchive(map_fs.data()))
	{
		device->drop();
		throw std::runtime_error{"You added an error map file system!"};
	}

	testp::scene::IAnimatedMesh * map_mesh = scene->getMesh(map_name.data());
	if (! map_mesh)
	{
		device->drop();
		throw std::runtime_error{"You added an error map!"};
	}

	testp::scene::IQ3LevelMesh * map_mesh_q3 = static_cast<testp::scene::IQ3LevelMesh *>(map_mesh);

	testp::scene::IAnimatedMesh * model_mesh = scene->getMesh(model_name.data());
	if (! model_mesh)
	{
		device->drop();
		throw std::runtime_error{"You added an error model!"};
	}

	std::cout << "---------------------------------------------------------------------------"
		<< std::endl;

	auto model_mesh_3 = static_cast<testp::scene::IAnimatedMeshMD3 *>(model_mesh);
	testp::scene::SMD3Mesh * mesh_3_o = model_mesh_3->getOriginalMesh();

	testp::nub::array<testp::scene::SMD3MeshBuffer *> & buffers = mesh_3_o->Buffer;
	testp::int32_pub size = buffers.size();
	std::cout << "Buffers Size: " << size << std::endl;
	for (testp::int32_pub i=0; i<size; ++i)
	{
		testp::scene::SMD3MeshBuffer * buffer = buffers[i];
		if (! buffer)
			continue;
		testp::nub::string & shader_name = buffer->Shader;
		if (shader_name.empty())
		{
			std::cout << "buffer " << i << " has no shader name" << std::endl;
			continue;
		}
		// else
		std::cout << "buffer " << i << " has shader name: " << shader_name.data() << std::endl;
		const testp::scene::quake3::IShader * shader = map_mesh_q3->getShader(
			shader_name.data(),
			true
		);

		testp::scene::IMeshBuffer * i_mesh_buffer = model_mesh_3->getMesh(0)->getMeshBuffer(i);
		testp::scene::IMeshSceneNode * node;
		if (shader)
		{
			std::cout << "\tGot shader" << std::endl;
			node = scene->addQuake3SceneNode(
				i_mesh_buffer,
				shader,
				nullptr,
				-1
			);
		}
		else
		{
			std::cout << "\tcan not get shader" << std::endl;
			testp::scene::SMesh * s_mesh = new testp::scene::SMesh;
			s_mesh->addMeshBuffer(i_mesh_buffer);
			node = scene->addMeshSceneNode(
				s_mesh,
				nullptr,
				-1,
				{0,0,0},
				{0,0,0},
				{1,1,1},
				false
			);
			{
				testp::nub::string texture_name = shader_name;
				testp::video::ITexture * texture = nullptr;
				for (const char * ext: {".tga", ".png", ".jpg"})
				{
					texture_name = shader_name + ext;
					texture = video->getTexture(texture_name);
					if (texture)
						break;
				}
				if (texture)
				{
					std::cout << "Found Texture: " << texture_name.data() << std::endl;
					node->setMaterialTexture(0, texture);
				}
				else
				{
					std::cout << "Texture is not found" << std::endl;
				}
			}
			//node->setMaterialFlag(testp::video::EMF_LIGHTING, false);
		}
		if (! node)
		{
			std::cout << "\tnode is not added" << std::endl;
			continue;
		}
		// else
		std::cout << "\tnode is added" << std::endl;
	}

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
}
catch (const std::exception & e)
{
	std::cerr
		<< "===========================================================================\n"
		<< "c++ std::exception:\n"
		 << e.what() << std::endl;
	return 1;
}

