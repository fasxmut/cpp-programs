//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <vector>
#include <boost/signals2.hpp>
#include <functional>
#include <future>
#include <atomic>

namespace ws
{

using std::placeholders::_1;

using signal_type = boost::signals2::signal<void(std::string)>;
using slot_type = ws::signal_type::slot_type;
using connection_type = boost::signals2::connection;

///////////////////////////////////////////////////////////////////////////

class world
{
protected:
	testp::nub::vector3df center;
	testp::float32_pub side;
	testp::float32_pub thickness;
	testp::float32_pub amplitude;
	testp::float32_pub skin_tile_size;
protected:
	std::vector<std::filesystem::path> textures;
	std::vector<testp::video::ITexture *> tex;
	int texture_used = 0;
protected:
	testp::TestpubDevice * device{nullptr};
	testp::video::IVideoDriver * video;
	testp::scene::ISceneManager * scene;
	testp::scene::IMeshManipulator * manip;
	const testp::scene::IGeometryCreator * geom;
	testp::scene::ICameraSceneNode * fps_camera;
	testp::scene::ICameraSceneNode * static_camera;
protected:
	testp::scene::IAnimatedMeshSceneNode * base = nullptr;
	testp::scene::IAnimatedMeshSceneNode * skin1 = nullptr;
protected:
	testp::scene::ILightSceneNode * light = nullptr;
protected:
	ws::signal_type __signal;
public:
	virtual ~world()
	{
	}
public:
	world(
		const testp::nub::vector3df & center__,
		testp::float32_pub side__,
		testp::float32_pub thickness__,
		testp::TestpubDevice * device__,
		testp::float32_pub amplitude__,
		testp::float32_pub skin_tile_size__,
		const std::vector<std::filesystem::path> & textures__
	):
	////////////////////////////////////////
		center{center__},
		side{side__},
		thickness{thickness__},
		amplitude{amplitude__},
		skin_tile_size{skin_tile_size__},
		textures{std::move(textures__)},
	////////////////////////////////////////
		device{device__},
		video{device->getVideoDriver()},
		scene{device->getSceneManager()},
		manip{scene->getMeshManipulator()},
		geom{scene->getGeometryCreator()},
		fps_camera{
			scene->addCameraSceneNodeFPS(
				nullptr,
				65.0f,
				0.1f,
				-1,
				nullptr,
				0,
				false,
				30.0f,
				false,
				true
			)
		},
		static_camera{
			scene->addCameraSceneNode(
				nullptr,
				{},
				{},
				-1,
				false
			)
		}
	{
	}
public:
	void create()
	{
		this->preload_textures();
		this->create_lighting();
		this->enable_lighting(true);

		this->create_base();
		this->setup_collision();

		this->update_camera(
			center + testp::nub::vector3df{0, 200, 0},
			center + testp::nub::vector3df{0, 200, 200}
		);

		this->create_skin1();
	}
public:
	void enable_lighting(bool value__)
	{
		if (base)
			base->setMaterialFlag(testp::video::EMF_LIGHTING, value__);
		if (skin1)
			skin1->setMaterialFlag(testp::video::EMF_LIGHTING, value__);

	}
	void create_lighting()
	{
		light = scene->addLightSceneNode(
			nullptr,
			center + testp::nub::vector3df{300, side*0.3f, 0},
			testp::video::SColor{0xfff2ba64},
			side*1.2,
			-1
		);
		light->setLightType(testp::video::ELT_POINT);

		testp::scene::ISceneNodeAnimator * ani = scene->createFlyCircleAnimator(
			center + testp::nub::vector3df{0, 300, 0},
			this->side * 0.3f,
			0.003f,
			center + testp::nub::vector3df{0, 500, 0},
			0,
			0
		);
		light->addAnimator(ani);
		ani->drop();
		ani = nullptr;
	}
protected:
	void preload_textures()
	{
		for (const std::filesystem::path & p: textures)
		{
			auto t = video->getTexture(p.string().data());
			if (t)
				tex.push_back(t);
		}
		std::cout << "Preloaded Texture count: " << tex.size() << std::endl;
		if (tex.size() == 0)
		{
			this->signal("Error: you are not loading any textures!");
			throw std::runtime_error{"You must load at least one texture!"};
		}
		// else
		this->signal("All textures are preloaded");
	}
protected:
	void create_base()
	{
		testp::scene::IMesh * mesh = geom->createCubeMesh(
			testp::nub::vector3df{side, thickness, side},
			testp::scene::ECMT_6BUF_4VTX_NP
		);
		testp::scene::IMesh * mesh_tg = manip->createMeshWithTangents(
			mesh
		);
		mesh->drop();
		mesh = nullptr;
		testp::scene::SAnimatedMesh * mesh_set = new testp::scene::SAnimatedMesh;
		mesh_set->addMesh(mesh_tg);
		mesh_tg->drop();
		mesh_tg = nullptr;
		base = scene->addAnimatedMeshSceneNode(
			mesh_set,
			nullptr,
			-1,
			center + testp::nub::vector3df{0,-thickness/2,0},
			{},
			{1,1,1},
			false
		);
		mesh_set->drop();
		mesh_set = nullptr;
		base->setMaterialTexture(0, this->use_texture());
		auto tex1 = this->use_texture();
		video->makeNormalMapTexture(tex1, amplitude);
		base->setMaterialTexture(1, tex1);

		std::cout << "Is base created: " << bool(base) << std::endl;
		this->signal(bool(base)?"base is created":"base is not created");
	}
protected:
	testp::video::ITexture * use_texture()
	{
		if (texture_used >= tex.size())
		{
			std::cout << "WARNING: loaded texture is too less, reuse starting from the first texture!" << std::endl;
			texture_used -= tex.size();
		}
		return tex[texture_used++];
	}
protected:
	void create_skin1()
	{
		testp::uint32_pub tile_x_n = this->side / this->skin_tile_size;
		testp::uint32_pub tile_y_n = this->side / this->skin_tile_size;
		testp::float32_pub tile_u_s = tile_x_n * 1.3f;
		testp::float32_pub tile_v_s = tile_y_n * 1.3f;

		testp::scene::IMesh * mesh = geom->createPlaneMesh(
			testp::nub::dimension2df{skin_tile_size, skin_tile_size},
			testp::nub::dimension2du{tile_x_n, tile_y_n},
			nullptr,
			testp::nub::dimension2df{tile_u_s, tile_v_s}
		);

		testp::scene::IMesh * mesh_lm = manip->createMeshWith2TCoords(mesh);
		mesh->drop();
		mesh = nullptr;

		testp::scene::SAnimatedMesh * mesh_set = new testp::scene::SAnimatedMesh;
		mesh_set->addMesh(mesh_lm);
		mesh_lm->drop();
		mesh_lm = nullptr;

		skin1 = scene->addAnimatedMeshSceneNode(
			mesh_set,
			nullptr,
			-1,
			(center + fps_camera->getPosition())*0.8f,
			{},
			{1,1,1},
			false
		);
		skin1->setMaterialType(testp::video::EMT_TRANSPARENT_ADD_COLOR);
		skin1->setMaterialTexture(0, this->use_texture());
		skin1->setMaterialTexture(1, this->use_texture());
		this->toggle_skin_position(true);
	}
public:
	void update_camera(
		const testp::nub::vector3df & position,
		const testp::nub::vector3df & target
	)
	{
		fps_camera->setPosition(position);
		fps_camera->setTarget(target);
		static_camera->setPosition(position);
		static_camera->setTarget(target);
	}
public:
	void toggle_skin_position(bool attach__)
	{
		if (attach__)
			this->skin1->setPosition(
				center + testp::nub::vector3df{0,0.1,0}
			);
		else
			this->skin1->setPosition(
				(center + fps_camera->getPosition()) / 2
			);
	}
public:
	void set_base_mt(const testp::video::E_MATERIAL_TYPE mt__)
	{
		base->setMaterialType(mt__);
	}
public:
	void set_skin_mt(const testp::video::E_MATERIAL_TYPE mt__)
	{
		skin1->setMaterialType(mt__);
	}
public:
	void release_cursor()
	{
		auto pos = scene->getActiveCamera()->getPosition();
		auto target = scene->getActiveCamera()->getTarget();
		scene->setActiveCamera(static_camera);
		scene->getActiveCamera()->setPosition(pos);
		scene->getActiveCamera()->setTarget(target);
		device->getCursorControl()->setVisible(true);
	}
	void trap_cursor()
	{
		scene->setActiveCamera(fps_camera);
		device->getCursorControl()->setVisible(false);
	}
protected:
	void setup_collision()
	{
		testp::scene::ITriangleSelector * selector = scene->createOctreeTriangleSelector(
			base->getMesh(),
			base,
			256
		);
		base->setTriangleSelector(selector);
		testp::scene::ISceneNodeAnimator * ani = scene->createCollisionResponseAnimator(
			selector,
			fps_camera,
			testp::nub::vector3df{25,25,25},
			testp::nub::vector3df{0,0,0},
			testp::nub::vector3df{0,0,0},
			0.001f
		);
		fps_camera->addAnimator(ani);
		ani->drop();
		ani = nullptr;
	}
public:
	ws::connection_type attach_viewer(const ws::slot_type & slot__)
	{
		return this->__signal.connect(slot__);
	}
public:
	void signal(const std::string & msg)
	{
		this->__signal(msg);
	}
};

///////////////////////////////////////////////////////////////////////////

static std::atomic<int> oops = 1;
static std::atomic<int> oops_high_score = 0;

class text_event
{
protected:
	ws::connection_type __connection;
	std::future<int> __future;
	std::packaged_task<int(int)> __task;
	std::future<void> __my_thread;
	bool __e;
public:
	virtual ~text_event()
	{
	}
public:
	text_event(
		ws::world & world__,
		bool e__
	):
		__connection{
			world__.attach_viewer(
				std::bind(
					&ws::text_event::update,
					this,
					ws::_1
				)
			)
		},
		__e{e__}
	{
	}
public:
	void update(const std::string & msg)
	{
		std::cout << "[msg queue] " << msg << std::endl;
		if (msg == "start-future")
		{
			__task = std::packaged_task<int(int)>{
				[ee=__e] (int x)
				{
					int sum = 0;
					for (int i=0; i<x; ++i)
					{
						sum += i;
					}
					++ws::oops;
					if (ws::oops > ws::oops_high_score)
						ws::oops_high_score.store(ws::oops.load());
					std::cout << "++[[[[ [[[[ ws::oops: " << ws::oops << std::endl;
					if (ee)
						throw std::runtime_error{"I am error"};
					return sum;
				}
			};
			__future = __task.get_future();
			this->run_task();
			std::cout << "A new thread is created" << std::endl;
		}
		if (msg == "wait-future")
		{
			int result = -1;
			try
			{
				--ws::oops;
				result = __future.get();
			}
			catch (const std::exception & e)
			{
				std::cout << "Got std::exception: " << e.what() << std::endl;
			}
			std::cout << "--[[[[ [[[[ ws::oops: " << ws::oops << std::endl;
			std::cout << "Got:";
			std::cout << "[[[result]]] " << result << std::endl;
			std::cout << "||||||||| the future is waited" << std::endl;
		}
	}
protected:
	void run_task()
	{
		__my_thread = std::async(
			std::launch::async,
			std::move(__task),
			10
		);
	}
};

class event: virtual public testp::IEventReceiver
{
protected:
	ws::world & world;
	testp::TestpubDevice * __device;
	bool __quit_oops = false;
	std::vector<ws::text_event *> __text_event_list;
public:
	virtual ~event()
	{
	}
public:
	event(ws::world & world__, testp::TestpubDevice * device__):
		world{world__},
		__device{device__}
	{
	}
public:
	bool OnEvent(const testp::SEvent & event) override
	{
		if (__quit_oops)
		{
			for (ws::text_event * ev: __text_event_list)
			{
				delete ev;
				ev = nullptr;
			}
			std::cout << "==================== Quit OOPS ====================\n";
			std::cout << "ws::oops: " << ws::oops.load() << std::endl;
			std::cout << "Some ws::oops might be not pulled by __future.get()" << std::endl;
			std::cout << "ws::oops_high_score: " << ws::oops_high_score.load() << std::endl;
			std::cout << "==================== ========= ====================\n";
			if (__device)
				__device->closeDevice();
			return false;
		}
		switch (event.EventType)
		{
		default:
			break;
		case testp::EET_KEY_INPUT_EVENT:
			this->process_key(event);
			break;
		}
		return false;
	}
protected:
	void process_key(const testp::SEvent & event)
	{
		switch (event.KeyInput.Key)
		{
		default:
			break;
		////////////////////////////////////////
		case testp::KEY_KEY_1:
			world.set_base_mt(testp::video::EMT_NORMAL_MAP_SOLID);
			break;
		case testp::KEY_KEY_2:
			world.set_base_mt(testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA);
			break;
		case testp::KEY_KEY_3:
			world.set_base_mt(testp::video::EMT_PARALLAX_MAP_SOLID);
			break;
		case testp::KEY_KEY_4:
			world.set_base_mt(testp::video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA);
			break;
		////////////////////////////////////////
		case testp::KEY_KEY_5:
			world.set_skin_mt(testp::video::EMT_SOLID);
			break;
		case testp::KEY_KEY_6:
			world.set_skin_mt(testp::video::EMT_TRANSPARENT_ADD_COLOR);
			world.signal("testp::video::EMT_TRANSPARENT_ADD_COLOR is used");
			break;
		case testp::KEY_KEY_7:
			world.set_skin_mt(testp::video::EMT_LIGHTMAP_LIGHTING);
			break;
		case testp::KEY_KEY_8:
			world.set_skin_mt(testp::video::EMT_LIGHTMAP_LIGHTING_M2);
			break;
		case testp::KEY_KEY_9:
			world.set_skin_mt(testp::video::EMT_LIGHTMAP_LIGHTING_M4);
			break;
		////////////////////////////////////////
		case testp::KEY_KEY_L:
			world.enable_lighting(true);
			break;
		case testp::KEY_KEY_N:
			world.enable_lighting(false);
			break;
		case testp::KEY_KEY_P:
			world.toggle_skin_position(true);	// true: attach skin
			break;
		case testp::KEY_KEY_O:
			world.toggle_skin_position(false);	// false: detach skin
			break;
		////////////////////////////////////////
		case testp::KEY_LCONTROL:
		case testp::KEY_RCONTROL:
			if (event.KeyInput.PressedDown)
			{
				world.release_cursor();
				world.signal("Cursor control is released");
			}
			else
			{
				world.trap_cursor();
				world.signal("Cursor control is back");
				static bool __status = false;
				if (! __status)
					world.signal("start-future");
				else
					world.signal("wait-future");
				__status = !__status;
			}
			break;
		////////////////////////////////////////
		case testp::KEY_KEY_Q:
			__quit_oops = true;
			break;
		}
	}
public:
	void add_text_event(ws::text_event * evt)
	{
		__text_event_list.push_back(evt);
	}
};

}	// namespace ws

int main(int argc, char * argv[])
{
	std::cout << std::boolalpha;
	std::vector<std::filesystem::path> tex;
	for (int i=1; i<argc; ++i)
		tex.push_back(argv[i]);

	for (const auto & t: tex)
		std::cout << "tex: " << t << std::endl;

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{2560, 1440},
		32,
		false,
		true,
		true,
		nullptr
	);
	auto [video, scene, cursor, fs, gui] = std::tuple{
		device->getVideoDriver(),
		device->getSceneManager(),
		device->getCursorControl(),
		device->getFileSystem(),
		device->getGUIEnvironment()
	};

	cursor->setVisible(false);

	ws::world world{{0,0,0},2000,30,device,12.0f, 128, std::move(tex)};

	ws::event event{world, device};

	auto text_event1 = new ws::text_event{world, false};
	auto text_event2 = new ws::text_event{world, true};
	event.add_text_event(text_event1);
	event.add_text_event(text_event2);

	world.create();

	device->setEventReceiver(&event);

	testp::int32_pub x, y, w, h;
	x = 20;
	y = 10;
	w = 400;
	h = 400;
	gui->addStaticText(
		L"Help:\n"
		L"1		-		base normal map solid\n"
		L"2		-		base normal map transparent\n"
		L"3		-		base parallax map solid\n"
		L"4		-		base parallax map transparent\n"
		L"\n\n"
		L"5		-		skin solid\n"
		L"6		-		skin transparent\n"
		L"7		-		skin lightmap\n"
		L"8		-		skin lightmap m2\n"
		L"9		-		skin lightmap m4\n"
		L"\n\n"
		L"L		-		Lighting On\n"
		L"N		-		Lighting Off\n"
		L"\n\n"
		L"P		-		attach skin\n"
		L"O		-		detach skin\n"
		L"\n\n"
		L"CTRL		-		Hold CTRL to release cursor control\n"
		L"\n\n"
		L"Q		-		Quit OOPS"
		,
		testp::nub::recti{x, y, x+w, y+h},
		true,
		true,
		nullptr,
		-1,
		true
	);

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
		gui->drawAll();
		video->endScene();
	}
	device->drop();
	device = nullptr;
}

