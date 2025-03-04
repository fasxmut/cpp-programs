//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <lyra/lyra.hpp>
#include <vector>
#include <filesystem>

namespace raid
{

enum class type
{
	standard,
	transparent,
	lightmap,
	normal_map_solid,
	normal_map_transparent,
	parallax_map_solid,
	parallax_map_transparent
};

class wall
{
protected:
	std::string __label;
	testp::scene::ISceneManager * __scene;
	testp::video::IVideoDriver * __video;
	const testp::scene::IGeometryCreator * const __geom;
	const testp::scene::IMeshManipulator * const __manip;

	raid::type __type;
	std::vector<std::filesystem::path> __texture_list;
	std::vector<testp::nub::vector3df> __from_to;
	testp::float32_pub __thickness;
	testp::float32_pub __zoom_space;
	bool __disable_lighting;
	bool __disable_texture;
protected:
	testp::scene::IAnimatedMeshSceneNode * __node = nullptr;
public:
	static int counter;
public:
	wall(
		const std::string label__,
		testp::TestpubDevice * device__,
		raid::type type__,
		const std::initializer_list<std::filesystem::path> & texture_list__,	// 1~2 values
		const std::initializer_list<testp::nub::vector3df> from_to__,	// 2 values
		testp::float32_pub thickness__,
		testp::float32_pub zoom_space__,
		bool disable_lighting__,
		bool disable_texture__
	):
		__label{label__},
		__scene{device__->getSceneManager()},
		__video{device__->getVideoDriver()},
		__geom{__scene->getGeometryCreator()},
		__manip{__scene->getMeshManipulator()},
		__type{type__},
		__texture_list{texture_list__},
		__from_to{from_to__},
		__thickness{thickness__},
		__zoom_space{zoom_space__},
		__disable_lighting{disable_lighting__},
		__disable_texture{disable_texture__}
	{
		this->create();
	}
protected:
	void create()
	{
		std::cout << "========================================\n"
			<< "----- begin: " << __label << " -----" << std::endl << std::endl;

		testp::float32_pub my_width = __from_to[1].X - __from_to[0].X - __zoom_space *2;
		testp::float32_pub my_height = __from_to[1].Y - __from_to[0].Y;
		testp::float32_pub my_thickness = __thickness;
		testp::scene::IMesh * my_mesh = nullptr;
		bool use_plane_mesh = false;
		if (! use_plane_mesh)
		{
			my_mesh = __geom->createCubeMesh(
				testp::nub::vector3df{my_width, my_height, my_thickness},
				testp::scene::ECMT_6BUF_4VTX_NP
				);
		}
		else
		{
			testp::uint32_pub size_w = 3, size_h = 5;
			testp::float32_pub tsize_w = my_width / size_w;
			testp::float32_pub tsize_h = my_height / size_h;
			my_mesh = __geom->createPlaneMesh(
				testp::nub::dimension2df{tsize_w, tsize_h},
				testp::nub::dimension2du{size_w, size_h},
				nullptr,
				testp::nub::dimension2df{size_w*1.0f, size_h*1.0f}
			);
		}
		testp::video::E_MATERIAL_TYPE my_emt;
		testp::scene::IMesh * my_mesh_new = nullptr;
		testp::video::ITexture * nm = nullptr;
		switch (__type)
		{
		case raid::type::standard:
			my_emt = testp::video::EMT_SOLID;
			my_mesh_new = my_mesh;
			break;
		case raid::type::transparent:
			my_emt = testp::video::EMT_TRANSPARENT_ADD_COLOR;
			my_mesh_new = my_mesh;
			break;
		case raid::type::lightmap:
			my_emt = testp::video::EMT_LIGHTMAP_ADD;
			my_mesh_new = __manip->createMeshWith2TCoords(my_mesh);
			my_mesh->drop();
 			my_mesh = nullptr;
			break;
		case raid::type::normal_map_solid:
			my_emt = testp::video::EMT_NORMAL_MAP_SOLID;
			my_mesh_new = __manip->createMeshWithTangents(my_mesh, true);
	 		my_mesh->drop();
 			my_mesh = nullptr;
 			if (__texture_list.size() > 1)
 			{
				nm = __video->getTexture(__texture_list[1].string().data());
				__video->makeNormalMapTexture(nm, 32.0);
			}
 			break;
 		case raid::type::normal_map_transparent:
 			my_emt = testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA;
 			my_mesh_new = __manip->createMeshWithTangents(my_mesh, true);
 			my_mesh->drop();
 			my_mesh = nullptr;
 			if (__texture_list.size() > 1)
 			{
				nm = __video->getTexture(__texture_list[1].string().data());
				__video->makeNormalMapTexture(nm);
			}
 			break;
 		case raid::type::parallax_map_solid:
 			my_emt = testp::video::EMT_PARALLAX_MAP_SOLID;
 			my_mesh_new = __manip->createMeshWithTangents(my_mesh, true);
 			my_mesh->drop();
 			my_mesh = nullptr;
 			if (__texture_list.size() > 1)
 			{
				nm = __video->getTexture(__texture_list[1].string().data());
				__video->makeNormalMapTexture(nm);
			}
 			break;
 		case raid::type::parallax_map_transparent:
 			my_emt = testp::video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA;
 			my_mesh_new = __manip->createMeshWithTangents(my_mesh, true);
 			my_mesh->drop();
 			my_mesh = nullptr;
 			if (__texture_list.size() > 1)
 			{
				nm = __video->getTexture(__texture_list[1].string().data());
				__video->makeNormalMapTexture(nm);
			}
 			break;
		}
		if (my_mesh_new)
			std::cout << "[mesh is created]" << std::endl;
		else
		{
			std::cout << "[mesh is not created]" << std::endl;
			return;
		}
		std::cout << std::boolalpha << "[is renew]: " << !my_mesh << std::endl;
		my_mesh = nullptr;

		testp::scene::SAnimatedMesh * my_a_mesh = new testp::scene::SAnimatedMesh;
		my_a_mesh->addMesh(my_mesh_new);
		my_mesh_new->drop();
		my_mesh_new = nullptr;

		__node = __scene->addAnimatedMeshSceneNode(
			my_a_mesh,
			nullptr,
			-1,
			(__from_to[0] + __from_to[1])/2,
			use_plane_mesh?testp::nub::vector3df{-90,0,0}:testp::nub::vector3df{0,0,0},
			{1,1,1},
			false
		);
		my_a_mesh->drop();
		my_a_mesh = nullptr;
		if (__node)
		{
			++raid::wall::counter;
			std::cout << "[node is created]" << std::endl;
		}
		else
		{
			std::cout << "[node is not created]" << std::endl;
			return;
		}

		__node->setMaterialType(my_emt);
		__node->setMaterialFlag(testp::video::EMF_LIGHTING, ! __disable_lighting);
		__node->setMaterialFlag(testp::video::EMF_BACK_FACE_CULLING, true);
		if (! __disable_texture)
		{
			__node->setMaterialTexture(
				0,
				__video->getTexture(__texture_list[0].string().data())
			);
			std::cout << "0\n";
			std::cout << "\t" << __texture_list[0] << std::endl;
			if (__texture_list.size() > 1 && nm)
			{
				__node->setMaterialTexture(
					1,
					nm
				);
				std::cout << "1\n";
				std::cout << "\t" << __texture_list[1] << std::endl;
			}
		}

		testp::scene::ILightSceneNode * light = __scene->addLightSceneNode(
			nullptr,
			__node->getPosition() + testp::nub::vector3df{0, 0, -100},
			testp::video::SColor{0xffffffff},
			2000,
			-1
		);
		if (light)
		{
			if (true)
			{
				light->setLightType(testp::video::ELT_POINT);
				std::cout << "\t\tlight is added" << std::endl;
				auto p = light->getPosition();
				std::cout << "\t\tLight Pos: "
					<< p.X << ',' << p.Y << ',' << p.Z << std::endl;
				auto np = __node->getPosition();
				std::cout << "\t\tNode Pos:"
					<< np.X << "," << np.Y << ',' << np.Z << std::endl;
			}
			if (false)
			{
				testp::float32_pub radius = 50;
				auto ani = __scene->createFlyCircleAnimator(
					light->getPosition(),
					radius,
					0.0003,
					light->getPosition()+testp::nub::vector3df{0,0,-100},
					0,
					0
				);
				light->setPosition(light->getPosition()+testp::nub::vector3df{radius,0,0});
				light->addAnimator(ani);
				ani->drop();
				ani = nullptr;
			}
		}
		else
			std::cout << "\t\tlight is not added" << std::endl;

		std::cout << "\n----- end  : " << __label << " -----" << std::endl;
		std::cout << "========================================\n";
	}
};	// class wall
int raid::wall::counter = 0;

}	// namespace raid

namespace raid::info
{
	constexpr testp::float32_pub half_space = 10;

	constexpr testp::float32_pub out_width = 600;
	constexpr testp::float32_pub out_height = 1000;
	constexpr testp::float32_pub thickness = 20;

	const testp::nub::vector3df start{-4*raid::info::out_width, -raid::info::out_height/2, 0};
}	// namespace raid::info

int main(int argc, char * argv[])
{
	std::filesystem::path tex0;
	std::filesystem::path tex1;

	bool help = false;
	auto cli = lyra::help(help)
		| lyra::opt(tex0, "tex0")["-0"]("Texture 0 for all")
		| lyra::opt(tex1, "tex1")["-1"]("Texture 1 for all")
	;

	if (help || argc == 1 || ! cli.parse({argc, argv}))
	{
		std::cout << cli << std::endl;
		return 0;
	}

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{2560, 1440},
		32,
		false,
		true,
		false,
		nullptr
	);

	testp::scene::ISceneManager * scene = device->getSceneManager();
	auto video = device->getVideoDriver();
	auto camera = scene->addCameraSceneNodeFPS(
		nullptr,
		50,
		0.5,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);
	camera->setPosition(testp::nub::vector3df{0,100,-1200});
	camera->setTarget(testp::nub::vector3df{0,100,100});
	auto cursor = device->getCursorControl();
	cursor->setVisible(false);

	std::vector<raid::wall> wall_raid
		{
			{
				"solid wall",
				device,
				raid::type::standard,
				{
					tex0
				},
				{
					{ // from
						raid::info::start.X,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{ // to
						raid::info::start.X + raid::info::out_width,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					}
				},
				raid::info::thickness,
				raid::info::half_space,
				false,	// disable lighting ?
				false	// disable texture ?
			},
			{
				"transparent wall",
				device,
				raid::type::transparent,
				{
					tex0,
					tex1
				},
				{
					{
						raid::info::start.X + raid::info::out_width,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{
						raid::info::start.X + raid::info::out_width * 2,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					},
				},
				raid::info::thickness,
				raid::info::half_space,
				false,
				false
			},
			{
				"lightmap wall",
				device,
				raid::type::lightmap,
				{
					tex0,
					tex1
				},
				{
					{
						raid::info::start.X + raid::info::out_width * 2,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{
						raid::info::start.X + raid::info::out_width * 3,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					},
				},
				raid::info::thickness,
				raid::info::half_space,
				false,
				false
			},
			{
				"normal map solid",
				device,
				raid::type::normal_map_solid,
				{
					tex0,
					tex1
				},
				{
					{
						raid::info::start.X + raid::info::out_width * 3,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{
						raid::info::start.X + raid::info::out_width * 4,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					},
				},
				raid::info::thickness,
				raid::info::half_space,
				false,
				false
			},
			{
				"normal map transparent",
				device,
				raid::type::normal_map_transparent,
				{
					tex0,
					tex1
				},
				{
					{
						raid::info::start.X + raid::info::out_width * 4,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{
						raid::info::start.X + raid::info::out_width * 5,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					},
				},
				raid::info::thickness,
				raid::info::half_space,
				false,
				false
			},
			{
				"parallax map solid",
				device,
				raid::type::parallax_map_solid,
				{
					tex0,
					tex1
				},
				{
					{
						raid::info::start.X + raid::info::out_width * 5,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{
						raid::info::start.X + raid::info::out_width * 6,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					},
				},
				raid::info::thickness,
				raid::info::half_space,
				false,
				false
			},
			{
				"parallax map transparent",
				device,
				raid::type::parallax_map_transparent,
				{
					tex0,
					tex1
				},
				{
					{
						raid::info::start.X + raid::info::out_width * 6,
						raid::info::start.Y,
						raid::info::start.Z
					},
					{
						raid::info::start.X + raid::info::out_width * 7,
						raid::info::start.Y + raid::info::out_height,
						raid::info::start.Z
					},
				},
				raid::info::thickness,
				raid::info::half_space,
				false,
				false
			}
		};

	std::cout << "Created wall counter: " << raid::wall::counter << std::endl;

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

