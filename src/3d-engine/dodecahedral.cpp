//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <complex>
#include <vector>
#include <complex>
#include <random>
#include <boost/assert.hpp>
#include <lyra/lyra.hpp>

class vertex;

class vertex
{
public:
	float x, y, z;
public:
	vertex operator-(const vertex & v) const
	{
		return vertex{x-v.x, y-v.y, z-v.z};
	}
	vertex cross_product(const vertex & v) const
	{
		return vertex{
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x
		};
	}
	vertex get_normalize() const;
};

class polygon5
{
public:
	std::vector<testp::video::S3DVertex> vertices;
	std::vector<testp::uint16_pub> indices;
	std::mt19937 rng{std::random_device{}()};
public:
	// Incorrect parameter order makes incorrect graphics
	polygon5(
		const vertex & a,
		const vertex & b,
		const vertex & c,
		const vertex & d,
		const vertex & e
	)
	{
		auto normal = (b-a).cross_product(c-a).get_normalize();
		testp::uint8_pub R = rng()%256;
		testp::uint8_pub G = rng()%256;
		testp::uint8_pub B = rng()%256;
		testp::uint32_pub color = 0xff000000|(R<<16)|(G<<8)|(B);

		BOOST_ASSERT(vertices.size() == 0);
		for (const auto & x: {a,b,c,d,e})
		{
			vertices.emplace_back(
				x.x, x.y, x.z,
				normal.x, normal.y, normal.z,
				color,
				0,0
			);
		}

		BOOST_ASSERT(vertices.size() == 5);

		// Update UV
		vertices[0].TCoords.X = 0.3f;
		vertices[0].TCoords.Y = 0.1f;

		vertices[1].TCoords.X = 0.7f;
		vertices[1].TCoords.Y = 0.1f;

		vertices[2].TCoords.X = 0.9f;
		vertices[2].TCoords.Y = 0.5f;

		vertices[3].TCoords.X = 0.5f;
		vertices[3].TCoords.Y = 0.9f;

		vertices[4].TCoords.X = 0.1f;
		vertices[4].TCoords.Y = 0.5f;

		BOOST_ASSERT(indices.size() == 0);
		for (
			testp::uint16_pub i:
				{
					0,1,2,
					0,2,3,
					0,3,4
				}
		)
		{
			indices.push_back(i);
		}
	}
};

class dodecahedral
{
public:
	testp::scene::SMesh * mesh = nullptr;
public:
	dodecahedral():
		mesh{new testp::scene::SMesh}
	{
	}
	virtual ~dodecahedral()
	{
		if (mesh)
		{
			mesh->drop();	// down reference-counter of the mesh
			mesh = nullptr;
		}
	}
	void init()
	{
		float w = std::numbers::phi;
		float k = 1/w;

		vertex v0{1,1,-1};
		vertex v1{-1,1,-1};
		vertex v2{-1,-1,-1};
		vertex v3{1,-1,-1};

		vertex v4{1,-1,1};
		vertex v5{1,1,1};
		vertex v6{-1,1,1};
		vertex v7{-1,-1,1};

		vertex v8{k, 0, -w};
		vertex v9{-k, 0, -w};
		vertex v10{k, 0, w};
		vertex v11{-k, 0, w};

		vertex v12{0, w, -k};
		vertex v13{0, w, k};
		vertex v14{0, -w, -k};
		vertex v15{0, -w, k};

		vertex v16{-w, k, 0};
		vertex v17{-w, -k, 0};
		vertex v18{w, k, 0};
		vertex v19{w, -k, 0};

		{
			using p = polygon5;
			for (auto poly: {
				p{v0, v8, v9, v1, v12},
				p{v0, v12, v13, v5, v18},
				p{v0, v18, v19, v3, v8},
				p{v1, v16, v6, v13, v12},
				p{v2, v17, v16, v1, v9},
				p{v2, v14, v15, v7, v17},
				p{v2, v9, v8, v3, v14},
				p{v3, v19, v4, v15, v14},
				p{v4, v19, v18, v5, v10},
				p{v4, v10, v11, v7, v15},
				p{v6, v11, v10, v5, v13},
				p{v6, v16, v17, v7, v11}
			})
			{
				auto mesh_buffer = new testp::scene::SMeshBuffer;
				mesh_buffer->append(
					poly.vertices.data(),
					poly.vertices.size(),
					poly.indices.data(),
					poly.indices.size()
				);
				mesh_buffer->setDirty();
				mesh_buffer->recalculateBoundingBox();
				this->mesh->addMeshBuffer(mesh_buffer);
				mesh_buffer->drop();
			}
			mesh->setDirty();
			mesh->recalculateBoundingBox();
		}
	}
};

vertex vertex::get_normalize() const
{
	auto r = std::sqrt(x*x + y*y + z*z);
	return vertex{
		x/r,
		y/r,
		z/r
	};
}

int main(int argc, char * argv[])
{
	bool help = false;
	std::string texture;
	bool enable_lighting = false;

	auto cli = lyra::help(help)
		| lyra::opt(texture, "texture")["-t"]("texture")
		| lyra::opt(enable_lighting, "enable_lighting")["-l"]("Enable lighting")
	;
	if (help || argc == 1 || ! cli.parse({argc, argv}))
	{
		std::cout << cli << std::endl;
		return 0;
	}

	auto device = testp::createPub();
	auto video = device->getVideoDriver();
	auto scene = device->getSceneManager();
	auto camera = scene->addCameraSceneNodeFPS(
		nullptr,
		30,
		0.01,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);
	camera->setPosition({0,0,-4});
	device->getCursorControl()->setVisible(false);
	testp::scene::IAnimatedMeshSceneNode * node = nullptr;
	device->setWindowCaption(L"c++ program");
	{
		auto mesh = new testp::scene::SAnimatedMesh;
		{
			dodecahedral dode;
			dode.init();
			mesh->addMesh(dode.mesh);
			mesh->setDirty();
			mesh->recalculateBoundingBox();
		}
		std::cout << "mesh count: " << mesh->getFrameCount() << std::endl;
		std::cout << "mb count: " << mesh->getMesh(0)->getMeshBufferCount() << std::endl;
		node = scene->addAnimatedMeshSceneNode(
			mesh,
			nullptr,
			-1,
			{},
			{},
			{1,1,1},
			false
		);
		mesh->drop();
	}
	if (! node)
		throw std::runtime_error{"node is not added"};
	node->setMaterialFlag(testp::video::EMF_LIGHTING, enable_lighting);
	if (enable_lighting)
	{
		auto light = scene->addLightSceneNode(
			nullptr,
			{0,5,-5},
			testp::video::SColor{0xff0000ff},
			1,
			-1
		);
	}
	node->setMaterialTexture(0, video->getTexture(texture.data()));

	auto selector = scene->createTriangleSelector(
		node->getMesh(),
		node,
		false
	);
	auto r_ani = scene->createRotationAnimator({0.1,-0.05,0.21});
	node->addAnimator(r_ani);
	node->setTriangleSelector(selector);
	auto ani = scene->createCollisionResponseAnimator(
		selector,
		camera,
		{3,3,3},
		{0,0,0},
		{0,0,0},
		0.001f
	);
	selector->drop();
	camera->addAnimator(ani);
	ani->drop();
	while (device->run())
	{
		if (! device->isWindowActive())
		{
			device->yield();
			continue;
		}
		video->beginScene(
			testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH,
			testp::video::SColor{0xff123456}
		);
		scene->drawAll();
		video->endScene();
	}
	device->drop();
}

