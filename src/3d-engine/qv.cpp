//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// jimcpp forks irrlicht, using sfml as backend device.

#include <jimcpp/core.hpp>
#include <iostream>
#include <string_view>
#include <boost/signals2.hpp>
#include <filesystem>
#include <boost/assert.hpp>
#include <stdfloat>

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

namespace game
{

	class engine
	{
	public:
		jpp::JimcppDevice * device = nullptr;
		jpp::video::IVideoDriver * video;
		jpp::scene::ISceneManager * scene;
		jpp::gui::ICursorControl * cursor;
		jpp::io::IFileSystem * fs;
		jpp::scene::ICameraSceneNode * camera;
		jpp::scene::IMetaTriangleSelector * meta;
	public:
		virtual ~engine()
		{
			if (! device)
			{
				device->drop();
				device = nullptr;
			}
			std::cout << "Engine is closed." << std::endl;
		}
	public:
		engine() = delete;
	public:
		engine(
			std::uint32_t width__,
			std::uint32_t height__
		):
			device{
				jpp::createDevice(
					jpp::video::EDT_OPENGL,
					jpp::core::dimension2du{
						width__,
						height__
					},
					32,
					false,
					true,
					false,
					nullptr
				)
			}
		{
			if (! device)
				throw std::runtime_error{"Graphical window can not be opened."};

			video = device->getVideoDriver();
			scene = device->getSceneManager();

			cursor = device->getCursorControl();
			camera = scene->addCameraSceneNodeFPS(
				nullptr,
				75.0f,
				0.8f,
				-1,
				nullptr,
				0,
				false,
				20.0f,
				false,
				true
			);
			cursor->setVisible(false);

			fs = device->getFileSystem();

			meta = scene->createMetaTriangleSelector();
		}
	public:
		void run()
		{
			{
				jpp::scene::ISceneNodeAnimator * collision =
					scene->createCollisionResponseAnimator(
						meta,
						camera,
						jpp::core::vector3df{40, 80, 40},
						jpp::core::vector3df{0, -20, 0},
						jpp::core::vector3df{0, 0, 0},
						0.00001f
					);
				camera->addAnimator(collision);
				collision->drop();
			}
			while (device->run())
			{
				if (device->isWindowActive())
				{
					video->beginScene(true, true, jpp::video::SColor{0xff3399aa});
					scene->drawAll();
					video->endScene();
				}
				else
				{
					device->yield();
				}
			}
		}
	};	// class engine

	class map_loader
	{
	private:
		game::engine & __engine;
		std::vector<std::string> __map_list;
		std::vector<jpp::scene::IAnimatedMesh *> __mesh_list;
	public:
		map_loader() = delete;
	public:
		virtual ~map_loader() = default;
	public:
		map_loader(game::engine & engine__):
			__engine{engine__}
		{
		}
	public:
		void load(const std::string_view map_name)
		{
			bool status = this->search_map(map_name);
			status = this->add_meshes(status);
			status = this->load_maps(status);
			this->load_shaders(status);
			this->startup(status);
		}
	protected:
		bool search_map(const std::string_view map_name)
		{
			if (! map_name.ends_with(".pk3"))
				throw std::runtime_error{"only pk3 map is allowed."};
			if (! std::filesystem::exists(map_name))
				throw std::runtime_error{""s + map_name.data() + " does not exist!"};

			{
				bool status = __engine.fs->addFileArchive(map_name.data());
				if (! status)
					throw std::runtime_error{""s + map_name.data() + " can not be loaded!"};
			}

			{

				std::uint32_t ar_count = __engine.fs->getFileArchiveCount();
				for (std::uint32_t i=0; i<ar_count; ++i)
				{
					jpp::io::IFileArchive * ar = __engine.fs->getFileArchive(i);
					if (ar)
					{
						const jpp::io::IFileList * list = ar->getFileList();
						if (list)
						{
							std::uint32_t file_count = list->getFileCount();
							for (std::uint32_t i=0; i<file_count; ++i)
							{
								jpp::io::path filename = list->getFileName(i);
								std::string name = filename.data();
								if (name.ends_with(".bsp"))
									__map_list.push_back(name);
							}
						}
					}
				}

				if (__map_list.empty())
					throw std::runtime_error{"bsp format map can not be found!"};
				// else
				std::cout << "Found map count: " << __map_list.size() << std::endl;
			}

			return ! __map_list.empty();
		}
	protected:
		bool add_meshes(bool has_map)
		{
			if (! has_map || __map_list.empty())
				return false;
			for (const auto & map: __map_list)
			{
				if (map.ends_with(".bsp"))
				{
					jpp::scene::IAnimatedMesh * mesh = __engine.scene->getMesh(map.data());
					if (mesh)
						__mesh_list.push_back(mesh);
				}
			}
			if (__mesh_list.empty())
				throw std::runtime_error{"Can not add at least one master mesh!"};
			return ! __mesh_list.empty();
		}
	protected:
		bool load_maps(bool mesh_is_added)
		{
			if (! mesh_is_added || __mesh_list.empty())
				return false;
			bool loaded_a = false;
			for (auto * mesh: __mesh_list)
			{
				jpp::scene::IOctreeSceneNode * node = __engine.scene->addOctreeSceneNode(
					mesh->getMesh(0),
					nullptr,
					-1,
					1024,
					false
				);
				if (node)
					loaded_a = true;
				else
					continue;
				jpp::scene::ITriangleSelector * selector = __engine.scene->
					createOctreeTriangleSelector(
						node->getMesh(),
						node,
						1024
					);
				node->setTriangleSelector(selector);
				__engine.meta->addTriangleSelector(selector);
				selector->drop();
			}
			if (! loaded_a)
				throw std::runtime_error{"Can not load at least one master map!"};
			return loaded_a;
		}
	protected:
		void load_shaders(bool map_is_loaded)
		{
			if (! map_is_loaded)
				return;
			for (jpp::scene::IAnimatedMesh * mesh: __mesh_list)
			{
				jpp::scene::IQ3LevelMesh * qmesh = static_cast<jpp::scene::IQ3LevelMesh *>(mesh);
				for (
					const jpp::scene::quake3::eQ3MeshIndex qmi:
						{
							jpp::scene::quake3::E_Q3_MESH_GEOMETRY,
							jpp::scene::quake3::E_Q3_MESH_ITEMS,
							jpp::scene::quake3::E_Q3_MESH_BILLBOARD,
							jpp::scene::quake3::E_Q3_MESH_FOG,
							jpp::scene::quake3::E_Q3_MESH_UNRESOLVED,
							jpp::scene::quake3::E_Q3_MESH_SIZE
						}
				)
				{
					std::cout << "index: " << qmi;
					if (qmi == jpp::scene::quake3::E_Q3_MESH_SIZE)
					{
						std::cout << std::endl;
						continue;
					}
					jpp::scene::IMesh * imesh = qmesh->getMesh(qmi);
					if (imesh)
					{
						std::uint32_t mb_count = imesh->getMeshBufferCount();
						std::cout << "\n\tmesh buffer count: " << mb_count << std::endl;
					}
				}
				{
					jpp::scene::IMesh * item_mesh
						= qmesh->getMesh(jpp::scene::quake3::E_Q3_MESH_ITEMS);
					if (! item_mesh)
						continue;
					std::uint32_t mb_count = item_mesh->getMeshBufferCount();
					if (mb_count <= 0)
						continue;
					for (std::uint32_t i=0; i<mb_count; ++i)
					{
						jpp::scene::IMeshBuffer * mb = item_mesh->getMeshBuffer(i);
						if (! mb)
							continue;
						jpp::video::SMaterial & material = mb->getMaterial();
						int shader_index = (int)material.MaterialTypeParam2;
						const jpp::scene::quake3::IShader * shader =
							qmesh->getShader(shader_index);
						jpp::scene::IMeshSceneNode * node = __engine.scene->addQuake3SceneNode(
							mb,
							shader,
							nullptr,
							-1
						);
						jpp::scene::ITriangleSelector * selector =
							__engine.scene->createTriangleSelector(
								node->getMesh(),
								node,
								false
							);
						node->setTriangleSelector(selector);
						__engine.meta->addTriangleSelector(selector);
						selector->drop();
					}
				}
			}
		}
	protected:
		void startup(bool map_is_loaded)
		{
			if (! map_is_loaded)
				return;
			for (auto * mesh: __mesh_list)
			{
				jpp::scene::quake3::tQ3EntityList & qe_list =
					static_cast<jpp::scene::IQ3LevelMesh *>(mesh)->getEntityList();
				jpp::scene::quake3::IEntity search;
				search.name = "info_player_deathmatch";
				std::int32_t s_pos = 0;
				s_pos = qe_list.binary_search_multi(search, s_pos);
				if (s_pos < 0)
				{
					std::cout << "can not find startup position" << std::endl;
					return;
				}
				// else
				jpp::scene::quake3::IEntity & entity = qe_list[s_pos];
				int gsize = entity.getGroupSize();
				std::cout << "gsize: " << gsize << std::endl;
				BOOST_ASSERT(gsize >= 2);
				const jpp::scene::quake3::SVarGroup * g = entity.getGroup(1);
				jpp::core::string pos_str = g->get("origin");
				jpp::core::string angle_str = g->get("angle");
				std::uint32_t p_pos = 0;
				jpp::core::vector3df pos = jpp::scene::quake3::getAsVector3df(pos_str, p_pos);
				p_pos = 0;
				std::float32_t angle = jpp::scene::quake3::getAsFloat(angle_str, p_pos);
				__engine.camera->setPosition(pos);
				jpp::core::vector3df dir{0, 0, 1};
				dir.rotateXZBy(angle);
				__engine.camera->setTarget(pos + dir);
			}
		}
	};

}	// namespace game

int main(int argc, char * argv[])
try
{
	if (argc != 2)
		throw std::runtime_error{""s + "qv <pk3 name>"};
	game::engine engine{1925, 1085};

	game::map_loader qml{engine};
	qml.load(argv[1]);

	engine.run();
	return 0;
}
catch (const std::exception & e)
{
	std::cerr << "\n\n[ERROR] c++ std::exception:\n" << e.what() << std::endl << std::endl;
	return 1;
}

