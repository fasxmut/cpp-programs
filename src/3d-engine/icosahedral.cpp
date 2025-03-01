//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <complex>
#include <lyra/lyra.hpp>

// icosahedral

class vertex_type
{
public:
	float x, y, z;
public:
	auto operator-(const vertex_type & other) const
	{
		return vertex_type{x - other.x, y - other.y, z - other.z};
	}
	auto cross_product(const vertex_type & v2) const
	{
		return
			vertex_type{
				y * v2.z - z * v2.y,
				z * v2.x - x * v2.z,
				x * v2.y - y * v2.x
			};
	}
};
using v = vertex_type;	// vertex
using n = vertex_type;	// normal

using std::numbers::phi;

constexpr inline float b = 10;
constexpr inline float a = 10*phi;

class uv_type
{
public:
	float u, v;
};

using uv = uv_type;

class my_s3d_vertex_array
{
public:
	std::vector<testp::video::S3DVertex> vertices;
	std::vector<testp::uint16_pub> indices;

	std::mt19937 rng{std::random_device{}()};

	int face_count = 0;
public:
	void push_vertex(
		const vertex_type & v,	// vertex
		const vertex_type & n,	// normal
		testp::uint32_pub c,		// color
		const uv_type & uv_		// uv
	)
	{
		vertices.emplace_back(
			v.x, v.y, v.z,
			n.x, n.y, n.z,
			c,
			uv_.u, uv_.v
		);
	}
	void normalize(vertex_type & v)
	{
		float r = std::sqrt(
			v.x*v.x + v.y*v.y + v.z*v.z
		);
		v.x = v.x/r;
		v.y = v.y/r;
		v.z = v.z/r;
	}
	auto v_size() const {return vertices.size();}
	auto i_size() const {return indices.size();}
	// The order is v1,v2,v3; different order makes different normal
	void push_face(
		const vertex_type & v1,
		const vertex_type & v2,
		const vertex_type & v3
	)
	{
		++face_count;
		auto normal = (v2-v1).cross_product(v3-v1);
		this->normalize(normal);
		testp::uint8_pub r = rng() % 255;
		testp::uint8_pub g = rng() % 255;
		testp::uint8_pub b = rng() % 255;
		testp::uint32_pub color = ((255u << 24)|(r<<16)|(g<<8)|b);
		for (const auto & v: {v1,v2,v3})
		{
			this->push_vertex(
				v,
				normal,
				color,
				{0,0}
			);
		}
		const auto end = this->v_size() - 1;
		const auto start = end - 2;
	// Update UV coords
		vertices[start].TCoords.X = 0;
		vertices[start].TCoords.Y = 0;
		vertices[start+1].TCoords.X = 1;
		vertices[start+1].TCoords.Y = 0;
		vertices[start+2].TCoords.X = 1;
		vertices[start+2].TCoords.Y = 1;
	// Update indices
		for (auto i=start; i<=end; ++i)
			indices.push_back(i);
	}
	void dump() const
	{
		std::cout << "================================================= dump vertices: "
			<< std::endl << std::endl;
		for (int i=0; i<vertices.size(); ++i)
		{
			const auto & x = vertices[i];
			std::cout << i << "\t";
			std::cout << x.Pos.X << ',' << x.Pos.Y << ',' << x.Pos.Z << "\t\t"
				<< x.Normal.X << "," << x.Normal.Y << ',' << x.Normal.Z << "\t\t"
				<< x.Color.getRed() << ',' << x.Color.getGreen()
						<< ',' << x.Color.getBlue() << "\t\t"
				<< x.TCoords.X << ',' << x.TCoords.Y << std::endl;
			if ((i+1)%3 == 0)
				std::cout << std::endl;
		}
		std::cout << "================================================= dump indices: "
			<< std::endl << std::endl;
		for (int i=0; i<indices.size(); ++i)
		{
			std::cout << indices[i] << "\t";
			if ((i+1)%3 == 0)
				std::cout << std::endl;
		}
		std::cout << "\n\nface count: " << face_count << std::endl;
	}
};

int main(int argc, char * argv[])
{
	bool help = false;
	std::string texture;
	auto cli = lyra::help(help)
		| lyra::opt(texture, "texture")["-t"]("Add Texture")
	;
	if (help || argc == 1 || ! cli.parse({argc, argv}))
	{
		std::cout << cli << std::endl;
		return 0;
	}
	auto device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{1280, 720},
		32,
		false,
		true,
		false,
		nullptr
	);
	auto video = device->getVideoDriver();
	auto scene = device->getSceneManager();
	testp::scene::IAnimatedMeshSceneNode * node = nullptr;
	scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{0, a*10, b},
		testp::video::SColor{0x72a83215},
		a*50,
		-1
	)->setLightType(testp::video::ELT_POINT);
	scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{0, -a*10, b},
		testp::video::SColor{0x722172ba},
		a*50,
		-1
	)->setLightType(testp::video::ELT_POINT);
	device->setWindowCaption(L"c++ window");
	{
		auto data = new my_s3d_vertex_array;
		std::vector<testp::video::S3DVertex> vertices;
		std::vector<testp::uint16_pub> indices;
		{
			auto v0 = v{a, b, 0};
			auto v1 = v{-a, b, 0};
			auto v2 = v{-a, -b, 0};
			auto v3 = v{a, -b, 0};

			auto v4 = v{b, 0, a};
			auto v5 = v{-b, 0, a};
			auto v6 = v{-b, 0, -a};
			auto v7 = v{b, 0, -a};

			auto v8 = v{0, a, -b};
			auto v9 = v{0, a, b};
			auto v10 = v{0, -a, b};
			auto v11 = v{0, -a, -b};

			data->push_face(v0, v3, v7);
			data->push_face(v0, v7, v8);
			data->push_face(v0, v8, v9);
			data->push_face(v0, v9, v4);
			data->push_face(v0, v4, v3);

			data->push_face(v1, v8, v6);
			data->push_face(v1, v6, v2);
			data->push_face(v1, v2, v5);
			data->push_face(v1, v5, v9);
			data->push_face(v1, v9, v8);

			data->push_face(v2, v10, v5);
			//data->push_face(v2, v5, v1);
			//data->push_face(v2, v1, v6);
			data->push_face(v2, v6, v11);
			data->push_face(v2, v11, v10);

			//data->push_face(v3, v7, v0);
			//data->push_face(v3, v0, v4);
			data->push_face(v3, v4, v10);
			data->push_face(v3, v10, v11);
			data->push_face(v3, v11, v7);

			//data->push_face(v4, v3, v0),
			//data->push_face(v4, v0, v9);
			data->push_face(v4, v9, v5);
			data->push_face(v4, v5, v10);
			//data->push_face(v4, v10, v3);

			//data->push_face(v5, v4, v9);
			//data->push_face(v5, v9, v1);
			//data->push_face(v5, v1, v2);
			//data->push_face(v5, v2, v10);
			//data->push_face(v5, v10, v4);

			//data->push_face(v6, v2, v1);
			//data->push_face(v6, v1, v8);
			data->push_face(v6, v8, v7);
			data->push_face(v6, v7, v11);
			//data->push_face(v6, v11, v2);
		}
		data->dump();
		{
			auto buffer = new testp::scene::SMeshBuffer;
			buffer->append(
				data->vertices.data(),
				data->vertices.size(),
				data->indices.data(),
				data->indices.size()
			);
			buffer->setDirty();
			buffer->recalculateBoundingBox();
			auto smesh = new testp::scene::SMesh;
			smesh->addMeshBuffer(buffer);
			buffer->drop();
			smesh->setDirty();
			smesh->recalculateBoundingBox();
			auto ani_mesh = new testp::scene::SAnimatedMesh;
			ani_mesh->addMesh(smesh);
			smesh->drop();
			ani_mesh->recalculateBoundingBox();
			node = scene->addAnimatedMeshSceneNode(
				ani_mesh,
				nullptr,
				-1,
				{},
				{},
				{1,1,1},
				false
			);
			ani_mesh->drop();
		}
		delete data;
	}
	if (! node)
		throw std::runtime_error{"Node is not added"};
	node->setMaterialFlag(testp::video::EMF_LIGHTING, true);
	auto tex = video->getTexture(texture.data());
	if (tex)
		node->setMaterialTexture(0, tex);
	auto ani = scene->createRotationAnimator(
		testp::nub::vector3df{0.1, -0.03, 0.22}
	);
	node->addAnimator(ani);
	ani->drop();
	auto camera = scene->addCameraSceneNodeFPS(
		nullptr,
		40,
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
	camera->setPosition({2, 2, a*2});
	camera->setTarget({0,0,0});
	testp::scene::ITriangleSelector * selector = scene->createTriangleSelector(
		node->getMesh(),
		node,
		false
	);
	node->setTriangleSelector(selector);
	selector->drop();
	testp::scene::ISceneNodeAnimator * collision = scene->createCollisionResponseAnimator(
		selector,
		camera,
		testp::nub::vector3df{b,b,b},
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{0,0,0},
		0.0003f
	);
	camera->addAnimator(collision);
	collision->drop();
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
}

