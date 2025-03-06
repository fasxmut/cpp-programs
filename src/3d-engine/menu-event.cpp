//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <tuple>

namespace wk
{

enum
{
	id_null,
	id_a,
	id_b,
	id_c,
	id_d
};

class event: virtual public testp::IEventReceiver
{
protected:
	testp::TestpubDevice * __device = nullptr;
	testp::scene::ICameraSceneNode * __fps;
	testp::scene::ICameraSceneNode * __static;
	testp::gui::ICursorControl * __cursor;
	testp::scene::IAnimatedMeshSceneNode * __node;
	testp::gui::IGUIEnvironment * __gui;
public:
	virtual ~event()
	{
	}
public:
	event(
		testp::TestpubDevice * device__,
		testp::scene::ICameraSceneNode * fps__,
		testp::scene::ICameraSceneNode * static__,
		testp::gui::ICursorControl * cursor__,
		testp::scene::IAnimatedMeshSceneNode * node__,
		testp::gui::IGUIEnvironment * gui__
	):
		__device{device__},
		__fps{fps__},
		__static{static__},
		__cursor{cursor__},
		__node{node__},
		__gui{gui__}
	{
	}
public:
	bool OnEvent(const testp::SEvent & event__) override
	{
		this->process_event(event__);
		return false;
	}
protected:
	void process_event(const testp::SEvent & event__)
	{
		switch (event__.EventType)
		{
		default:
			break;
		case testp::EET_KEY_INPUT_EVENT:
			this->process_key(event__);
			break;
		case testp::EET_GUI_EVENT:
			this->process_gui(event__);
			break;
		}
	}
protected:
	void process_key(const testp::SEvent & event__)
	{
		switch (event__.KeyInput.Key)
		{
		default:
			break;
		case testp::KEY_ESCAPE:
			if (! event__.KeyInput.PressedDown)
				this->swap_gui(true);
			break;
		}
	}
protected:
	void swap_gui(bool value__)
	{
		if (value__)
		{
			__device->getSceneManager()->setActiveCamera(__static);
			__static->setPosition(__fps->getPosition());
			__static->setTarget(__fps->getTarget());
			__cursor->setVisible(true);
			return;
		}
		// else
		__device->getSceneManager()->setActiveCamera(__fps);
		__cursor->setVisible(false);
	}
protected:
	void process_gui(const testp::SEvent & event__)
	{
		switch (event__.GUIEvent.EventType)
		{
		default:
			break;
		case testp::gui::EGET_MENU_ITEM_SELECTED:
			this->process_menu(event__);
			break;
		}
	}
	void process_menu(const testp::SEvent & event__)
	{
		auto menu = (testp::gui::IGUIContextMenu *)event__.GUIEvent.Caller;
		testp::int32_pub command_id = menu->getItemCommandId(menu->getSelectedItem());
		switch (command_id)
		{
		default:
			break;
		case wk::id_c:
			this->swap_gui(false);
			break;
		case wk::id_a:
			this->enable_lighting(true);
			break;
		case wk::id_b:
			this->enable_lighting(false);
			break;
		case wk::id_d:
			__device->closeDevice();
			break;
		}
	}
protected:
	void enable_lighting(bool value__)
	{
		__node->setMaterialFlag(testp::video::EMF_LIGHTING, value__);
	}
};

}	// namespace wk

int main(int argc, char * argv[])
{
	std::vector<std::filesystem::path> tex;
	for (int i=1; i<argc; ++i)
		tex.push_back(argv[i]);
	for (const auto & p: tex)
		std::cout << "tex: " << p << std::endl;
	if (tex.size() == 0)
		throw std::runtime_error{"Requires at least one texture"};

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{2560, 1440},
		32,
		false,
		true,
		true,
		nullptr
	);
	auto [video, scene, cursor, gui] = std::tuple{
		device->getVideoDriver(),
		device->getSceneManager(),
		device->getCursorControl(),
		device->getGUIEnvironment()
	};
	cursor->setVisible(false);

	auto [fps, static_] = std::tuple{
		scene->addCameraSceneNodeFPS(nullptr,75,0.09,-1,nullptr,0,false,30,false,true),
		scene->addCameraSceneNode(nullptr,{},{},-1, false)
	};

	testp::scene::IMeshManipulator * manip = scene->getMeshManipulator();
	const testp::scene::IGeometryCreator * geom = scene->getGeometryCreator();

	testp::scene::IMesh * mesh = geom->createCubeMesh(
		testp::nub::vector3df{1000, 30, 1000},
		testp::scene::ECMT_6BUF_4VTX_NP
	);
	testp::scene::IMesh * mesh_lm = manip->createMeshWith2TCoords(mesh);
	mesh->drop();
	mesh = nullptr;

	testp::scene::SAnimatedMesh * mesh_ani = new testp::scene::SAnimatedMesh;
	mesh_ani->addMesh(mesh_lm);
	mesh_lm->drop();
	mesh_lm = nullptr;

	testp::scene::IAnimatedMeshSceneNode * node = scene->addAnimatedMeshSceneNode(
		mesh_ani,
		nullptr,
		-1,
		{},
		{},
		{1,1,1},
		false
	);

	if (! tex.empty())
	{
		node->setMaterialTexture(0, video->getTexture(tex.back().string().data()));
		tex.pop_back();
	}
	if (! tex.empty())
	{
		node->setMaterialTexture(1, video->getTexture(tex.back().string().data()));
		tex.pop_back();
	}
	node->setMaterialType(testp::video::EMT_LIGHTMAP_LIGHTING_M4);

	fps->setPosition(testp::nub::vector3df{0, 100, 0});
	fps->setTarget(testp::nub::vector3df{0, 80, 80});

	testp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{200, 200, 0},
		testp::video::SColor{0xff79c4a6},
		1000,
		-1
	);

	testp::scene::ISceneNodeAnimator * ani = scene->createFlyCircleAnimator(
		testp::nub::vector3df{0,200,0},
		300,
		0.003,
		testp::nub::vector3df{0,400,0},
		0,
		0
	);
	light->addAnimator(ani);
	ani->drop();
	ani = nullptr;
	light->setLightType(testp::video::ELT_POINT);

	testp::scene::ILightSceneNode * light_spot = scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{-100,300,0},
		testp::video::SColor{0xff0000ff},
		1000,
		-1
	);
	light_spot->setLightType(testp::video::ELT_SPOT);

	static_->setPosition(fps->getPosition());
	static_->setTarget(fps->getTarget());
	scene->setActiveCamera(fps);
	cursor->setVisible(false);

	testp::gui::IGUIContextMenu * menu = gui->addMenu(nullptr, -1);

	menu->addItem(L"File", -1, true, true, false, false);
	menu->addItem(L"Edit", -1, true, true, false, false);
	menu->addItem(L"Help", -1, true, false, false, false);

	testp::gui::IGUIContextMenu * fsub = menu->getSubMenu(0);
	fsub->addItem(L"New ...", -1, true, true, false, false);
	testp::gui::IGUIContextMenu * fsub_sub = fsub->getSubMenu(0);
	fsub_sub->addItem(L"New File ...", -1, true, false, false, false);

	testp::gui::IGUIContextMenu * esub = menu->getSubMenu(1);
	esub->addItem(L"Enable Lighting ...", wk::id_a, true, false, false, false);
	esub->addItem(L"Disable Lighting ...", wk::id_b, true, false, false, false);
	esub->addItem(L"go to world ...", wk::id_c, true, false, false, false);
	esub->addItem(L"Quit ...", wk::id_d, true, false, false, false);

	menu->addItem(L"c++", -1, true, false, false, false);

	gui->addStaticText(
		L"Presss Esc to release cursor ...",
		testp::nub::recti{50, 50, 250, 80},
		true,
		true,
		nullptr,
		-1,
		true
	);

	wk::event event{device, fps, static_, cursor, node, gui};
	device->setEventReceiver(&event);

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

