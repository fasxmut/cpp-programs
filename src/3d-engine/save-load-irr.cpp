//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <testpub/core.hpp>
#include <vector>
#include <lyra/lyra.hpp>
#include <filesystem>

using std::string_literals::operator""s;

namespace my
{

class engine
{
public:
	testp::TestpubDevice * __device = nullptr;
	testp::scene::IMetaTriangleSelector * __meta = nullptr;
public:
	engine(testp::uint32_pub w__, testp::uint32_pub h__, const std::wstring title__):
		__device{
			testp::createPub(
				testp::video::EDT_EGXU,
				testp::nub::dimension2du{w__, h__},
				32,
				false,
				true,
				false,
				nullptr
			)
		}
	{
		if (! __device)
			throw std::runtime_error{"device creation error"};
		__device->setWindowCaption(title__.data());
		this->scene()->addCameraSceneNodeFPS(
			nullptr,
			50,
			0.2,
			-1,
			nullptr,
			0,
			false,
			30,
			false,
			true
		);
		this->camera()->setPosition(testp::nub::vector3df{5, 10, -100});
		this->camera()->setTarget(testp::nub::vector3df{0, 7, 0});
		this->cursor()->setVisible(false);
		__meta = this->scene()->createMetaTriangleSelector();
	}
public:
	virtual ~engine()
	{
		this->drop();
	}
protected:
	void drop()
	{
		if (__device)
		{
			__device->drop();
			__device = nullptr;
			std::cout << "device is dropped" << std::endl;
		}
		else
			std::cout << "device is already dropped" << std::endl;
	}
public:
	testp::TestpubDevice * device()
	{
		return __device;
	}
	testp::scene::IMetaTriangleSelector * meta()
	{
		return __meta;
	}
	testp::scene::ISceneManager * scene()
	{
		return __device->getSceneManager();
	}
	testp::video::IVideoDriver * video()
	{
		return __device->getVideoDriver();
	}
	testp::scene::ICameraSceneNode * camera()
	{
		return this->scene()->getActiveCamera();
	}
	testp::gui::ICursorControl * cursor()
	{
		return __device->getCursorControl();
	}
	testp::io::IFileSystem * fs()
	{
		return __device->getFileSystem();
	}
public:
	void setup_collision(testp::scene::ISceneNode * sprite)
	{
		testp::scene::ISceneNodeAnimator * collision =
			this->scene()->createCollisionResponseAnimator(
				this->meta(),
				sprite,
				testp::nub::vector3df{20, 20, 20},
				testp::nub::vector3df{0, -1000, 0},
				testp::nub::vector3df{0, 5, 0},
				0.00001f
			);
		sprite->addAnimator(collision);
		collision->drop();
	}
public:
	void run()
	{
		while (this->__device->run())
		{
			if (! this->__device->isWindowActive())
			{
				this->__device->yield();
				continue;
			}
			this->video()->beginScene(
				testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL,
				testp::video::SColor{0xff123456}
			);
			this->scene()->drawAll();
			this->video()->endScene();
		}
	}
public:
	void save_scene(
		const std::string filename
	)
	{
		if (filename.empty())
			throw std::runtime_error{"save path is empty"};
		if (! filename.ends_with(".irr"))
			throw std::runtime_error{"save path filename must be ended with .irr"};
		std::cout << "======================================================================"
			<< std::endl;
		std::cout << "Save scene to " << filename << std::endl;
		this->scene()->saveScene(filename.data(), nullptr, nullptr);
		std::cout << "======================================================================"
			<< std::endl;
	}
};

class pk_loader
{
protected:
	my::engine & __engine;
protected:
	testp::scene::IQ3LevelMesh * __mesh = nullptr;
public:
	pk_loader(my::engine & engine__):
		__engine{engine__}
	{
	}
public:
	void load(
		const std::filesystem::path pk_path__,
		const std::string & map_name__
	)
	{
		if (pk_path__.empty())
			throw std::runtime_error{"pk path is empty"};
		if (pk_path__ == "/")
			throw std::runtime_error{"/ is not allowed as pk path"};
		if (pk_path__ == "//")
			throw std::runtime_error{"// is not allowed as pk path"};
		if (map_name__.empty())
			throw std::runtime_error{"map name is empty"};
		if (! map_name__.ends_with(".bsp"))
			throw std::runtime_error{"Invalid map name"};
		if (! __engine.fs()->addFileArchive(pk_path__.string().data()))
			throw std::runtime_error{"Invalid pk path"};
		this->load_master(map_name__);
		this->load_items();
		this->spawn_player();
	}
public:
	void load_master(const std::string & map_name__)
	{
		testp::scene::IAnimatedMesh * mesh = __engine.scene()->getMesh(map_name__.data());
		if (! mesh)
			throw std::runtime_error{"can not load master map"};
		__mesh = static_cast<testp::scene::IQ3LevelMesh *>(mesh);
		testp::scene::IOctreeSceneNode * map = __engine.scene()->addOctreeSceneNode(
			mesh->getMesh(0),
			nullptr,
			-1,
			128,
			false
		);
		if (! map)
			throw std::runtime_error{"Can not create world from map"};
		testp::scene::ITriangleSelector * selector = __engine.scene()->
			createOctreeTriangleSelector(
				map->getMesh(),
				map,
				128
			);
		map->setTriangleSelector(selector);
		__engine.meta()->addTriangleSelector(selector);
		selector->drop();
	}
public:
	void load_items()
	{
		if (! __mesh)
			throw std::runtime_error{"Master mesh is not loaded!"};
		for (
			testp::scene::quake3::eQ3MeshIndex qm_index:
				{
					testp::scene::quake3::E_Q3_MESH_GEOMETRY,
					testp::scene::quake3::E_Q3_MESH_ITEMS,
					testp::scene::quake3::E_Q3_MESH_BILLBOARD,
					testp::scene::quake3::E_Q3_MESH_FOG,
					testp::scene::quake3::E_Q3_MESH_UNRESOLVED,
					testp::scene::quake3::E_Q3_MESH_SIZE
				}
		)
		{
			std::cout << "qm_index: " << static_cast<int>(qm_index) << std::endl;
			std::cout << "\tMesh Buffer Count: ";

			if (qm_index == testp::scene::quake3::E_Q3_MESH_SIZE)
			{
				std::cout << "not mesh" << std::endl;
				continue;
			}
			// else
			std::cout << __mesh->getMesh(qm_index)->getMeshBufferCount() << std::endl;
		}
		testp::scene::IMesh * q_mesh = __mesh->getMesh(testp::scene::quake3::E_Q3_MESH_ITEMS);
		for (testp::uint32_pub i=0; i<q_mesh->getMeshBufferCount(); ++i)
		{
			testp::scene::IMeshBuffer * mesh_buffer = q_mesh->getMeshBuffer(i);
			if (! mesh_buffer)
				continue;
			testp::video::SMaterial & material = mesh_buffer->getMaterial();
			testp::int32_pub shader_index = static_cast<testp::int32_pub>(material.MaterialTypeParam2);
			if (shader_index < 1)
			{
				std::cout << i << " no shader" << std::endl;
				continue;
			}
			// else
			std::cout << i << " shader index: " << shader_index << std::endl;
			const testp::scene::quake3::IShader * shader = __mesh->getShader(shader_index);
			if (! shader)
			{
				std::cout << "\tcan not get shader" << std::endl;
				continue;
			}
			// else
			std::cout << "\tGot shader" << std::endl;
			testp::scene::IMeshSceneNode * node = __engine.scene()->addQuake3SceneNode(
				mesh_buffer,
				shader,
				nullptr,
				-1
			);
			if (! node)
			{
				std::cout << "\t\tnode is not added" << std::endl;
				continue;
			}
			// else
			std::cout << "\t\tnode is added" << std::endl;
			testp::scene::ITriangleSelector * selector = __engine.scene()->
				createTriangleSelector(
					node->getMesh(),
					node,
					false
				);
			node->setTriangleSelector(selector);
			__engine.meta()->addTriangleSelector(selector);
			selector->drop();
			selector = nullptr;
		}
	}
public:
	void spawn_player()
	{
		testp::scene::quake3::tQ3EntityList & e_list = __mesh->getEntityList();
		testp::scene::quake3::IEntity search;
		search.name = "info_player_deathmatch";
		testp::int32_pub s_pos = 0;
		testp::int32_pub result = e_list.binary_search_multi(search, s_pos);
		if (result < 0)
		{
			std::cout << search.name.data() << " is not found" << std::endl;
			return;
		}
		// else
		std::cout << "Got player " << search.name.data() << " : " << result << std::endl;
		testp::scene::quake3::IEntity & entity = e_list[result];
		if (entity.getGroupSize() < 2)
		{
			std::cout << "Requires: group size >= 2" << std::endl;
			return;
		}
		const testp::scene::quake3::SVarGroup * group = entity.getGroup(1);
		testp::nub::string pos_str = group->get("origin");
		testp::nub::string angle_str = group->get("angle");
		std::cout << "pos_str: " << pos_str.data() << std::endl;
		std::cout << "angle_str: " << angle_str.data() << std::endl;
		testp::uint32_pub m_pos = 0;
		testp::nub::vector3df pos = testp::scene::quake3::getAsVector3df(pos_str, m_pos);
		m_pos = 0;
		testp::float32_pub angle = testp::scene::quake3::getAsFloat(angle_str, m_pos);
		__engine.camera()->setPosition(pos);
		testp::nub::vector3df dir{0, 0, 1};
		dir.rotateXZBy(angle);
		__engine.camera()->setTarget(pos+dir);
	}
};

class model
{
protected:
	my::engine & __engine;
	testp::scene::IAnimatedMeshSceneNode * __node = nullptr;
public:
	model(my::engine & engine__):
		__engine{engine__}
	{
	}
public:
	void load(
		const std::filesystem::path model_path__,
		const testp::nub::vector3df & pos__
	)
	{
		if (model_path__.empty())
			throw std::runtime_error{"model path is empty"};
		__node = __engine.scene()->addAnimatedMeshSceneNode(
			__engine.scene()->getMesh(model_path__.string().data()),
			nullptr,
			-1,
			pos__,
			{},
			{1,1,1},
			false
		);
		if (! __node)
		{
			std::cout << "Model: " << model_path__ << " can not be loaded" << std::endl;
			return;
		}
		// else
		std::cout << "Model: " << model_path__ << " is loaded" << std::endl;
		__node->setMaterialFlag(testp::video::EMF_LIGHTING, false);
	}
	testp::scene::IAnimatedMeshSceneNode * get_node()
	{
		return __node;
	}
};

}

int main(int argc, char * argv[])
try
{
	bool help = false;
	std::filesystem::path pk_path;
	std::string map_name;
	std::filesystem::path model_name;
	std::string save_to;
	auto cli = lyra::help(help)
		| lyra::opt(pk_path, "pk_path")["-p"]("The pk map path")
		| lyra::opt(map_name, "map_name")["-m"]("The map name")
		| lyra::opt(model_name, "model_name")["-d"]("The extra model name")
		| lyra::opt(save_to, "save_to")["-s"]("The target .irr path to save world")
	;

	if (help || argc == 1 || ! cli.parse({argc, argv}))
	{
		std::cout << cli << std::endl;
		return 0;
	}

	std::cout << "pk_path: " << pk_path << std::endl;
	std::cout << "model_name: " << model_name << std::endl;

	my::engine engine{1280, 720, L"c++ window, 3D, : export"};
	my::pk_loader qml{engine};
	qml.load(pk_path, map_name);
	my::model model{engine};
	model.load(model_name, engine.camera()->getPosition() + testp::nub::vector3df{0, 100, 50});

	engine.save_scene(save_to);

	engine.setup_collision(engine.camera());
	engine.setup_collision(model.get_node());
	engine.run();

	std::cout << "======================================================================\n";
	std::cout << "======================================================================\n";
	std::cout << "======================================================================\n";
	std::cout << "======================================================================\n";

	{
		my::engine egx{1600, 900, L"c++ window, 3D, : import"};
		bool status = egx.fs()->addFileArchive(pk_path.string().data());
		if (! status)
			throw std::runtime_error{"Add file system error at import, "s + pk_path.string()};
		egx.scene()->loadScene(save_to.data());
		
		testp::nub::array<testp::scene::ISceneNode *> nodes;
		egx.scene()->getSceneNodesFromType(testp::scene::ESNT_ANY, nodes);
		std::vector<testp::scene::ISceneNode *> sprites;
		for (testp::uint32_pub i=0; i<nodes.size(); ++i)
		{
			testp::scene::ISceneNode * node = nodes[i];
			testp::scene::ITriangleSelector * selector = nullptr;
			switch (node->getType())
			{
			case testp::scene::ESNT_OCTREE:
				std::cout << "Got octree node" << std::endl;
				selector = engine.scene()->createOctreeTriangleSelector(
					static_cast<testp::scene::IOctreeSceneNode *>(node)->getMesh(),
					node,
					128
				);
				node->setTriangleSelector(selector);
				egx.meta()->addTriangleSelector(selector);
				selector->drop();
				selector = nullptr;
				break;
			case testp::scene::ESNT_ANIMATED_MESH:
				std::cout << "Got animated mesh node" << std::endl;
				sprites.push_back(node);
				break;
			case testp::scene::ESNT_MESH:
				std::cout << "Got mesh node" << std::endl;
				sprites.push_back(node);
				break;
			default:
				std::cout << "Got unknown node" << std::endl;
				break;
			}
		}
		sprites.push_back(egx.camera());
		for (testp::scene::ISceneNode * sprite: sprites)
		{
			testp::scene::ISceneNodeAnimator * collision = egx.scene()->
				createCollisionResponseAnimator(
					egx.meta(),
					sprite,
					testp::nub::vector3df{20, 20, 20},
					testp::nub::vector3df{0, -1000, 0},
					testp::nub::vector3df{0, 5, 0},
					0.00001f
				);
			sprite->addAnimator(collision);
			collision->drop();
			collision = nullptr;
		}
		egx.run();
	}
}
catch (const std::exception & e)
{
	std::cout << "======================================================================"
		<< std::endl;
	std::cout << "c++ std::exception: ERROR: " << e.what() << std::endl;
}

