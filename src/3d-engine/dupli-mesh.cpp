//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <lyra/lyra.hpp>
#include <string>
#include <stdexcept>
#include <vector>
#include <boost/assert.hpp>

constexpr int duplicated = 4;
static_assert(duplicated >= 4);

using std::string_literals::operator""s;

namespace my
{

class quit_a_class
{
public:
	void operator()(
		bool condition,
		testp::TestpubDevice * device,
		const std::string & msg = "c++ std::exception"
	) const
	{
		if (! condition)
			return;
		if (device)
		{
			device->drop();
			device = nullptr;
			std::cout << "device is dropped" << std::endl;
		}
		else
			std::cout << "device is already dropped" << std::endl;
		throw std::runtime_error{"MSG: "s + msg};
	}
};

inline my::quit_a_class quit_a{};

}

int main(int argc, char * argv[])
try
{
	bool help = false;
	std::string mesh_name;
	std::vector<std::string> textures(duplicated);
	auto cli = lyra::help(help)
		| lyra::opt(mesh_name, "Mesh Name")["-m"]("The mesh name path")
		| lyra::opt(textures[0], "Texture 0 Name")["-0"]("The texture 0 name path")
		| lyra::opt(textures[1], "Texture 0 Name")["-1"]("The texture 1 name path")
		| lyra::opt(textures[2], "Texture 0 Name")["-2"]("The texture 2 name path")
		| lyra::opt(textures[3], "Texture 0 Name")["-3"]("The texture 3 name path")
	;
	auto result = cli.parse({argc, argv});
	if (help || argc == 1 || ! result)
	{
		std::cout << cli << std::endl;	// print Help
		return 0;
	}

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{1280, 720},
		32,
		false,
		true,
		false,
		nullptr
	);
	device->setWindowCaption(L"c++ window");

	my::quit_a(!device, device, "device creation error");

	testp::scene::ISceneManager * scene = device->getSceneManager();
	testp::video::IVideoDriver * video = device->getVideoDriver();

	testp::scene::IAnimatedMesh * mesh = scene->getMesh(mesh_name.data());
	my::quit_a(! mesh, device, "mesh is not loaded");

	if (true)
	{
		my::quit_a(mesh->getMeshType() != testp::scene::EAMT_MD2,
			device, "only md2 model is supported");

		std::vector<testp::scene::IAnimatedMeshSceneNode *> nodes;

		{
			auto md2 = static_cast<testp::scene::IAnimatedMeshMD2 *>(mesh);
			for (
				testp::scene::EMD2_ANIMATION_TYPE ani:
					{
						testp::scene::EMAT_STAND,
						testp::scene::EMAT_ATTACK,
						testp::scene::EMAT_SALUTE,
						testp::scene::EMAT_BOOM
					}
			)
			{
				testp::int32_pub start, end, fps;
				md2->getFrameLoop(ani, start, end, fps);
				std::cout << "::" << start << "::" << end << "::" << fps << "::" << std::endl;
				auto s_ani_mesh = new testp::scene::SAnimatedMesh;
				for (testp::int32_pub i=start; i<=end; ++i)
				{
					auto s_mesh = new testp::scene::SMesh;
					for (testp::int32_pub j=0; j<md2->getMesh(i)->getMeshBufferCount(); ++j)
					{
						auto mesh_buffer = md2->getMesh(i)->getMeshBuffer(j)->createClone();
						s_mesh->addMeshBuffer(mesh_buffer);
						mesh_buffer->drop();
					}
					s_ani_mesh->addMesh(s_mesh);
					s_mesh->drop();
				}
				s_ani_mesh->setAnimationSpeed(fps);
				auto node = scene->addAnimatedMeshSceneNode(
					s_ani_mesh,
					nullptr,
					-1,
					{},
					{},
					{1,1,1},
					false
				);
				if (node)
					nodes.push_back(node);
				s_ani_mesh->drop();
			}
		}

		my::quit_a(nodes.size() < duplicated, device, "ERROR: one or more node is not added, requires at least "s + std::to_string(duplicated));
	
		for (auto node: nodes)
		{
			node->setMaterialFlag(testp::video::EMF_LIGHTING, false);
			node->setLoopMode(true);
		}
		nodes[0]->setPosition({-50, 0, 0});
		nodes[0]->setMaterialTexture(0, video->getTexture(textures[0].data()));

		nodes[1]->setPosition({50, 0, 0});
		nodes[1]->setMaterialTexture(0, video->getTexture(textures[1].data()));

		nodes[2]->setPosition({0, 0, -70});
		nodes[2]->setMaterialTexture(0, video->getTexture(textures[2].data()));

		nodes[3]->setPosition({100, 0, -70});
		nodes[3]->setMaterialTexture(0, video->getTexture(textures[3].data()));
	}

	auto camera = scene->addCameraSceneNodeFPS();
	camera->setPosition({0,20,-150});
	camera->setTarget({30,0,0});
	device->getCursorControl()->setVisible(false);

	while (device->run())
	{
		if (! device->isWindowActive())
		{
			device->yield();
			continue;
		}
		video->beginScene(
			testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_DEPTH,
			testp::video::SColor{0xff123456}
		);
		scene->drawAll();
		video->endScene();
	}

	my::quit_a(true, device, "Quit normally");
}
catch (std::exception & e)
{
	std::cout << "---------------------------------------------------------------------------"
		<< std::endl;
	std::cout << "c++: " << e.what() << std::endl;
}
