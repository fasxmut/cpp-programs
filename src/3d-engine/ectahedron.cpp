//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <vector>
#include <lyra/lyra.hpp>

// ectahedron
// Radius: 10

namespace p
{
	using vertex = testp::video::S3DVertex;
	using vector = std::vector<p::vertex>;
	using array = testp::nub::array<p::vertex>;
}

int main(int argc, char ** argv)
{
	bool help = false;
	std::string texture0;
	std::string texture1;
	auto cli = lyra::help(help)
		| lyra::opt(texture0, "texture0")["-0"]("Texture 0: the master texture layer")
		| lyra::opt(texture1, "texture1")["-1"]("Texture 1: the secondary texture layer")
	;
	if (help || argc == 1 || ! cli.parse({argc, argv}))
	{
		std::cout << cli << std::endl;
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
	auto video = device->getVideoDriver();
	auto scene = device->getSceneManager();
	auto camera = scene->addCameraSceneNodeFPS(
		nullptr,45,0.07,-1,nullptr,0,false,30,false,true
	);
	device->getCursorControl()->setVisible(false);
	p::vector vertices{
	///////////////////////////////////////////////////////////////////////////
	// Right
	// face
		p::vertex{10,0,0,		1,1,1,		0xffff0000,		1,0},		// 0
		p::vertex{0,10,0,		1,1,1,		0xffff0000,		0,0},		// 1
		p::vertex{0,0,10,		1,1,1,		0xffff0000,		0.5,1},		// 2
	// face
		p::vertex{10,0,0,		1,1,-1,		0xff00ff00,		1,0},		// 3
		p::vertex{0,10,0,		1,1,-1,		0xff00ff00,		0,0},		// 4
		p::vertex{0,0,-10,		1,1,-1,		0xff00ff00,		0.5,1},		// 5
	// face
		p::vertex{10,0,0,		1,-1,1,		0xff0000ff,		1,0},		// 6
		p::vertex{0,0,10,		1,-1,1,		0xff0000ff,		0.5,1},		// 7
		p::vertex{0,-10,0,		1,-1,1,		0xff0000ff,		0,0},		// 8
	// face
		p::vertex{10,0,0,		1,-1,-1,		0xff00ffff,		1,0},		// 9
		p::vertex{0,0,-10,		1,-1,-1,		0xff00ffff,		0.5,1},		// 10
		p::vertex{0,-10,0,		1,-1,-1,		0xff00ffff,		0,0},		// 11
	///////////////////////////////////////////////////////////////////////////
	// Left
	// face
		p::vertex{-10,0,0,		-1,1,1,		0xffff00ff,		1,0},		// 12
		p::vertex{0,10,0,		-1,1,1,		0xffff00ff,		0,0},		// 13
		p::vertex{0,0,10,		-1,1,1,		0xffff00ff,		0.5,1},		// 14
	// face
		p::vertex{-10,0,0,		-1,-1,1,		0xffffff00,		1,0},		// 15
		p::vertex{0,0,10,		-1,-1,1,		0xffffff00,		0.5,1},		// 16
		p::vertex{0,-10,0,		-1,-1,1,		0xffffff00,		0,0},		// 17
	// face
		p::vertex{-10,0,0,		-1,-1,-1,		0xff123456,		1,0},		// 18
		p::vertex{0,0,-10,		-1,-1,-1,		0xff123456,		0.5,1},		// 19
		p::vertex{0,-10,0,		-1,-1,-1,		0xff123456,		0,0},		// 20
	// face
		p::vertex{-10,0,0,		-1,1,-1,		0xff654321,		1,0},		// 21
		p::vertex{0,0,-10,		-1,1,-1,		0xff654321,		0.5,1},		// 22
		p::vertex{0,10,0,		-1,1,-1,		0xff654321,		0,0},		// 23
	};
	std::vector<testp::uint16_pub> indices{
	///////////////////////////////////////////////////////////////////////////
		0,1,2,
		5,4,3,
		6,7,8,
		11,10,9,
	///////////////////////////////////////////////////////////////////////////
		14,13,12,
		17,16,15,
		18,19,20,
		23,22,21
	};
	std::cout << std::endl;

	auto buffer = new testp::scene::SMeshBuffer;
	buffer->append(vertices.data(), vertices.size(), indices.data(), indices.size());
	vertices.clear();
	indices.clear();
	buffer->setDirty();
	buffer->recalculateBoundingBox();
	auto s_mesh = new testp::scene::SMesh;
	s_mesh->addMeshBuffer(buffer);
	buffer->drop();
	auto a_mesh = new testp::scene::SAnimatedMesh;
	s_mesh->setDirty();
	s_mesh->recalculateBoundingBox();
	a_mesh->addMesh(s_mesh);
	a_mesh->recalculateBoundingBox();
	s_mesh->drop();
	auto node = scene->addAnimatedMeshSceneNode(a_mesh,nullptr,-1,{},{},{1,1,1},false);
	a_mesh->drop();
	node->setMaterialFlag(testp::video::EMF_BACK_FACE_CULLING, true);
	{
		testp::scene::ILightSceneNode * light0 = scene->addLightSceneNode(
			nullptr,testp::nub::vector3df{50,50,10},testp::video::SColor{0xffff1234},1000,-1);
		light0->setLightType(testp::video::ELT_POINT);
		testp::scene::ILightSceneNode * light1 = scene->addLightSceneNode(
			nullptr,testp::nub::vector3df{-50,-50,10},testp::video::SColor{0xff21ff43},1000,-1);
		light1->setLightType(testp::video::ELT_POINT);
	}
	auto animator = scene->createRotationAnimator(testp::nub::vector3df{-0.13,0.2,0.1});
	node->addAnimator(animator);
	node->setMaterialTexture(0, video->getTexture(texture0.data()));
	node->setMaterialTexture(1, video->getTexture(texture1.data()));
	animator->drop();
	camera->setPosition(node->getBoundingBox().MaxEdge*1.5);
	camera->setTarget(node->getBoundingBox().getCenter());
	while (device->run())
	{
		if (! device->isWindowActive())
		{
			device->yield();
			continue;
		}
		video->beginScene(
			testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL,
			testp::video::SColor{0x789abc}
		);
		scene->drawAll();
		video->endScene();
	}
	device->drop();
}

