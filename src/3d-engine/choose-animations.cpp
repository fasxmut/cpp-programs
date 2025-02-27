//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <lyra/lyra.hpp>
#include <iostream>
#include <vector>
#include <map>

using std::string_literals::operator""s;

testp::TestpubDevice * device = nullptr;

int main(int argc, char * argv[])
try
{
	std::string choice = "first";
	std::string mesh_path;
	std::string texture_path;
	bool help = false;
	auto cli = lyra::help(help)
		| lyra::opt(mesh_path, "Mesh Path")["-m"]("The path to the md2 mesh name")
		| lyra::opt(texture_path, "Texture Path")["-t"]("The path to the texture")
		| lyra::opt(choice, "Choice: first or second")["-c"]("Choose first animation or second animation")
	;
	auto result = cli.parse({argc, argv});
	if (help || argc == 1 || ! result)
	{
		std::cout << cli << std::endl;
		return 0;
	}
	if (choice != "first" && choice != "second")
		throw std::runtime_error{"choice must be first or second"};
	if (mesh_path == "" || ! mesh_path.ends_with("md2"))
		throw std::runtime_error{"Please provide correct md2 mesh path"};

	device = testp::createPub(
		testp::video::EDT_OPENGL,
		testp::nub::dimension2du{2560, 1600},
		32,
		false,
		true,
		false,
		nullptr
	);
	if (! device)
		throw std::runtime_error{"Device creation error"};
	auto scene = device->getSceneManager();
	auto video = device->getVideoDriver();

	auto mesh = scene->getMesh(mesh_path.data());
	if (! mesh)
		throw std::runtime_error{"You might provided an incorret md2 mesh"};

	if (mesh->getMeshType() != testp::scene::EAMT_MD2)
		throw std::runtime_error{"The mesh you provided is not md2 format"};

	auto md2 = static_cast<testp::scene::IAnimatedMeshMD2 *>(mesh);

	testp::scene::SAnimatedMesh * smesh = new testp::scene::SAnimatedMesh;

	std::vector<testp::int32_pub> count;

	for (
		testp::scene::EMD2_ANIMATION_TYPE type:
			{
				testp::scene::EMAT_ATTACK,
				testp::scene::EMAT_JUMP
			}
	)
	{
		testp::int32_pub start, end, fps;
		md2->getFrameLoop(type, start, end, fps);
		count.push_back(end-start+1);
		for (testp::int32_pub i=start; i<=end; ++i)
		{
			auto frame = md2->getMesh(i);
			testp::int32_pub mesh_buffer_count = frame->getMeshBufferCount();
			auto smesh_frame = new testp::scene::SMesh;
			for (testp::int32_pub j=0; j<mesh_buffer_count; ++j)
			{
				auto mesh_buffer = frame->getMeshBuffer(j);
				auto new_mesh_buffer = mesh_buffer->createClone();
				smesh_frame->addMeshBuffer(new_mesh_buffer);
				new_mesh_buffer->drop();
			}
			smesh->addMesh(smesh_frame);
			smesh_frame->drop();
		}
	}

	auto node = scene->addAnimatedMeshSceneNode(
		smesh,
		nullptr,
		-1,
		{},
		{},
		{1,1,1},
		false
	);
	smesh->drop();
	if (! node)
		throw std::runtime_error{"Making node has error"};

	node->setMaterialFlag(testp::video::EMF_LIGHTING, false);
	node->setMaterialTexture(0, video->getTexture(texture_path.data()));

	if (choice == "first")
		node->setFrameLoop(0, count[0]-1);
	else if (choice == "second")
		node->setFrameLoop(count[0], count[0] + count[1] - 1);

	auto cmr = scene->addCameraSceneNodeFPS();
	cmr->setPosition({0, 15, -70});
	cmr->setTarget({0, 10, 0});
	device->getCursorControl()->setVisible(false);

	while (device->run())
	{
		if (! device->isWindowActive())
		{
			device->yield();
			continue;
		}
		video->beginScene(
			testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL,
			testp::video::SColor{0xff456789}
		);
		scene->drawAll();
		video->endScene();
	}

	throw std::runtime_error{"Just end program, which is not an error"};
}
catch (std::exception & e)
{
	std::cout << "==========================================================================="
		<< std::endl;
	std::cout << "Error:\n";
	std::cout << e.what() << std::endl;
	if (device)
	{
		device->drop();
		device = nullptr;
		std::cout << "device is dropped" << std::endl;
	}
	else
	{
		std::cout << "device is already dropped" << std::endl;
	}
}

