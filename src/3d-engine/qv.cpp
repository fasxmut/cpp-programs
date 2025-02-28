//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// testpub forks irrlicht, using sfml as backend device.

#include <testpub/core.hpp>
#include <iostream>
#include <string_view>
#include <boost/signals2.hpp>
#include <filesystem>
#include <boost/assert.hpp>
#include <stdfloat>
#include <chrono>
#include <future>

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;
using std::placeholders::_1;

namespace game
{
	class engine;

	class map_loader;

	class viewer
	{
	public:
		using signal_type = boost::signals2::signal<void(const std::string &)>;
		using slot_type = signal_type::slot_type;
		using connection_type = boost::signals2::connection;
	private:
		std::string __name;
		std::vector<connection_type> __connections;
	public:
		virtual ~viewer()
		{
			for (auto & connection: __connections)
				connection.disconnect();
		}
	public:
		viewer(const std::string_view name__):
			__name{name__}
		{
		}
	public:
		void attach(int prior__, game::engine & qml__);
		void attach(int prior__, game::map_loader & qml__);
		void queue(const std::string & msg__)
		{
			std::cout << "[" << __name << "] " << msg__ << std::endl;
		}
	};	// class viewer

	class engine
	{
	public:
		testp::TestpubDevice * device = nullptr;
		testp::video::IVideoDriver * video;
		testp::scene::ISceneManager * scene;
		testp::gui::ICursorControl * cursor;
		testp::io::IFileSystem * fs;
		testp::scene::ICameraSceneNode * camera;
		testp::scene::IMetaTriangleSelector * meta;
	private:
		game::viewer::signal_type __signal;
	public:
		virtual ~engine()
		{
			if (! device)
			{
				device->drop();
				device = nullptr;
			}
			std::cout << "Engine is closed." << std::endl;
			this->signal("engine: closed");
		}
	public:
		engine() = delete;
	public:
		engine(
			std::uint32_t width__,
			std::uint32_t height__
		):
			device{
				testp::createPub(
					testp::video::EDT_EGXU,
					testp::nub::dimension2du{
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
				testp::scene::ISceneNodeAnimator * collision =
					scene->createCollisionResponseAnimator(
						meta,
						camera,
						testp::nub::vector3df{40, 80, 40},
						testp::nub::vector3df{0, -20, 0},
						testp::nub::vector3df{0, 0, 0},
						0.00001f
					);
				camera->addAnimator(collision);
				collision->drop();
			}
			this->signal("engine: Ready!");
			std::chrono::system_clock::time_point old, now;
			now = std::chrono::system_clock::now();
			old = now;
			std::chrono::system_clock::duration du_time = std::chrono::seconds(0);
			while (device->run())
			{
				if (device->isWindowActive())
				{
					video->beginScene(true, true, testp::video::SColor{0xff3399aa});
					scene->drawAll();
					{
						now = std::chrono::system_clock::now();
						if (now - old >= std::chrono::seconds(2))
						{
							this->signal(
								"engine: time passed: "s
								+ std::to_string(du_time.count() * 1.0f/1'000'000'000)
								+ " seconds"
							);
							old = now;
							du_time += std::chrono::seconds(2);
						}
					}
					video->endScene();
				}
				else
				{
					device->yield();
				}
			}
		}
	public:
		game::viewer::connection_type attach(
			int prior__,
			const game::viewer::slot_type & slot__
		)
		{
			return __signal.connect(
				prior__,
				slot__
			);
		}
	protected:
		void signal(const std::string & msg__)
		{
			this->__signal(msg__);
		}
	};	// class engine

	class map_loader
	{
	private:
		game::engine & __engine;
		std::vector<std::string> __map_list;
		std::vector<testp::scene::IAnimatedMesh *> __mesh_list;
		game::viewer::signal_type __signal;
	public:
		map_loader() = delete;
	public:
		virtual ~map_loader()
		{
			this->signal("map loader: Game is over");
		}
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
					testp::io::IFileArchive * ar = __engine.fs->getFileArchive(i);
					if (ar)
					{
						const testp::io::IFileList * list = ar->getFileList();
						if (list)
						{
							std::uint32_t file_count = list->getFileCount();
							for (std::uint32_t i=0; i<file_count; ++i)
							{
								testp::io::path filename = list->getFileName(i);
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
					testp::scene::IAnimatedMesh * mesh = __engine.scene->getMesh(map.data());
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
				testp::scene::IOctreeSceneNode * node = __engine.scene->addOctreeSceneNode(
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
				testp::scene::ITriangleSelector * selector = __engine.scene->
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
			this->signal("map loader: Map is loaded!");
			return loaded_a;
		}
	protected:
		void load_shaders(bool map_is_loaded)
		{
			if (! map_is_loaded)
				return;
			for (testp::scene::IAnimatedMesh * mesh: __mesh_list)
			{
				testp::scene::IQ3LevelMesh * qmesh = static_cast<testp::scene::IQ3LevelMesh *>(mesh);
				for (
					const testp::scene::quake3::eQ3MeshIndex qmi:
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
					std::cout << "index: " << qmi;
					if (qmi == testp::scene::quake3::E_Q3_MESH_SIZE)
					{
						std::cout << std::endl;
						continue;
					}
					testp::scene::IMesh * imesh = qmesh->getMesh(qmi);
					if (imesh)
					{
						std::uint32_t mb_count = imesh->getMeshBufferCount();
						std::cout << "\n\tmesh buffer count: " << mb_count << std::endl;
					}
				}
				{
					testp::scene::IMesh * item_mesh
						= qmesh->getMesh(testp::scene::quake3::E_Q3_MESH_ITEMS);
					if (! item_mesh)
						continue;
					std::uint32_t mb_count = item_mesh->getMeshBufferCount();
					if (mb_count <= 0)
						continue;
					for (std::uint32_t i=0; i<mb_count; ++i)
					{
						testp::scene::IMeshBuffer * mb = item_mesh->getMeshBuffer(i);
						if (! mb)
							continue;
						testp::video::SMaterial & material = mb->getMaterial();
						int shader_index = (int)material.MaterialTypeParam2;
						const testp::scene::quake3::IShader * shader =
							qmesh->getShader(shader_index);
						testp::scene::IMeshSceneNode * node = __engine.scene->addQuake3SceneNode(
							mb,
							shader,
							nullptr,
							-1
						);
						this->signal("map loader: Items are added");
						testp::scene::ITriangleSelector * selector =
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
				testp::scene::quake3::tQ3EntityList & qe_list =
					static_cast<testp::scene::IQ3LevelMesh *>(mesh)->getEntityList();
				testp::scene::quake3::IEntity search;
				search.name = "info_player_deathmatch";
				std::int32_t s_pos = 0;
				s_pos = qe_list.binary_search_multi(search, s_pos);
				if (s_pos < 0)
				{
					std::cout << "can not find startup position" << std::endl;
					return;
				}
				// else
				testp::scene::quake3::IEntity & entity = qe_list[s_pos];
				int gsize = entity.getGroupSize();
				std::cout << "gsize: " << gsize << std::endl;
				BOOST_ASSERT(gsize >= 2);
				const testp::scene::quake3::SVarGroup * g = entity.getGroup(1);
				testp::nub::string pos_str = g->get("origin");
				testp::nub::string angle_str = g->get("angle");
				std::uint32_t p_pos = 0;
				testp::nub::vector3df pos = testp::scene::quake3::getAsVector3df(pos_str, p_pos);
				p_pos = 0;
				std::float32_t angle = testp::scene::quake3::getAsFloat(angle_str, p_pos);
				__engine.camera->setPosition(pos);
				this->signal("map loader: Player is added");
				testp::nub::vector3df dir{0, 0, 1};
				dir.rotateXZBy(angle);
				__engine.camera->setTarget(pos + dir);
			}
		}
	public:
		game::viewer::connection_type attach(
			int prior__,
			const game::viewer::slot_type & slot__
		)
		{
			return __signal.connect(prior__, slot__);
		}
	protected:
		void signal(const std::string msg__)
		{
			this->__signal(msg__);
		}
	};	// class map_loader

	void game::viewer::attach(int prior__, game::engine & engine__)
	{
		__connections.push_back(
			engine__.attach(
				prior__,
				std::bind(
					&game::viewer::queue,
					this,
					_1
				)
			)
		);
	}

	void game::viewer::attach(int prior__, game::map_loader & qml__)
	{
		__connections.push_back(
			qml__.attach(
				prior__,
				std::bind(
					&game::viewer::queue,
					this,
					_1
				)
			)
		);
	}

}	// namespace game

int main(int argc, char * argv[])
try
{
	if (argc != 2)
		throw std::runtime_error{""s + "qv <pk3 name>"};

	game::viewer viewer1{"viewer 1"};
	game::viewer viewer2{"viewer 2"};

	// viewer lifetime must be longer than source lifetime.
	{
		game::engine engine{1925, 1085};
		game::map_loader qml{engine};

		viewer1.attach(0, qml);
		viewer2.attach(0, qml);

		viewer1.attach(1, engine);
		viewer2.attach(1, engine);

		qml.load(argv[1]);

		engine.run();
	}

	return 0;
}
catch (const std::exception & e)
{
	std::cerr << "\n\n[ERROR] c++ std::exception:\n" << e.what() << std::endl << std::endl;
	return 1;
}

