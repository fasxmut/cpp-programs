//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <boost/assert.hpp>
#include <vector>
#include <filesystem>

namespace wd
{

class event: virtual public testp::IEventReceiver
{
protected:
	testp::TestpubDevice * __device;
	testp::gui::IGUIEnvironment * __gui;
	testp::scene::ICameraSceneNode * __fps = nullptr;
	testp::scene::ICameraSceneNode * __static = nullptr;
	testp::gui::ICursorControl * __cursor;
	bool __show_gui = false;
public:
	event(
		testp::TestpubDevice * device__,
		testp::gui::IGUIEnvironment * gui__,
		testp::scene::ICameraSceneNode * fps__,
		testp::scene::ICameraSceneNode * static__,
		testp::gui::ICursorControl * cursor__
	):
		__device{device__},
		__gui{gui__},
		__fps{fps__},
		__static{static__},
		__cursor{cursor__}
	{
	}
public:
	bool OnEvent(const testp::SEvent & event__) override
	{
		switch (event__.EventType)
		{
		default:
			break;
		case testp::EET_KEY_INPUT_EVENT:
			this->process_key(event__);
			break;
		}
		return false;
	}
protected:
	void process_key(const testp::SEvent & event__)
	{
		switch (event__.KeyInput.Key)
		{
		default:
			break;
		case testp::KEY_LCONTROL:
		case testp::KEY_RCONTROL:
			__show_gui = event__.KeyInput.PressedDown;
			this->swap_gui(__show_gui);
			break;
		case testp::KEY_KEY_V:
			if (! event__.KeyInput.PressedDown)
				__show_gui = ! __show_gui;
			this->swap_gui(__show_gui);
			break;
		}
	}
	void cursor_visible(bool value__)
	{
		__cursor->setVisible(value__);
	}
	void camera_active(bool value__)
	{
		if (value__)
		{
			__device->getSceneManager()->setActiveCamera(__fps);
			return;
		}
		// else
		__static->setPosition(__fps->getPosition());
		__static->setTarget(__fps->getTarget());
		__device->getSceneManager()->setActiveCamera(__static);
	}
	void gui_visible(bool value__)
	{
		__gui->getRootGUIElement()->setVisible(value__);
	}
public:
	void swap_gui(bool value__)
	{
		this->cursor_visible(value__);
		this->camera_active(! value__);
		this->gui_visible(value__);
	}
};	// class event

}	// namespace wd

int main(int argc, char * argv[])
{
	std::vector<std::filesystem::path> tex;
	for (int i=1; i<argc; ++i)
		tex.push_back(argv[i]);

	if (tex.size() == 0)
		throw std::runtime_error{"No texture"};

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{2560, 1440},
		32,
		false,
		true,
		false,
		nullptr
	);

	testp::video::IVideoDriver * video = device->getVideoDriver();
	testp::scene::ISceneManager * scene = device->getSceneManager();
	testp::gui::IGUIEnvironment * gui = device->getGUIEnvironment();

	testp::gui::IGUIStaticText * text = gui->addStaticText(
		L"Hello, c++ world"
		L"\n\n"
		L"CTRL		-		Hold CTRL to show gui\n"
		L"V		-		Press V to swap gui ON and OFF\n"
		,
		testp::nub::recti{300, 50, 800, 550},
		true,
		true,
		nullptr,
		-1,
		true
	);

	testp::gui::IGUIElement * root_gui = gui->getRootGUIElement();
	testp::gui::IGUIElement * text_parent = text->getParent();
	BOOST_ASSERT(root_gui == text_parent);
	root_gui->setVisible(true);

	const testp::scene::IGeometryCreator * geom = scene->getGeometryCreator();
	testp::scene::ICameraSceneNode * fps = scene->addCameraSceneNodeFPS(
		nullptr,
		45.0f,
		0.07f,
		-1,
		nullptr,
		0,
		false,
		30.0f,
		false,
		true
	);
	fps->setPosition(testp::nub::vector3df{0, 50, -200});
	fps->setTarget(testp::nub::vector3df{0, 0, 0});
	testp::scene::ICameraSceneNode * static__ = scene->addCameraSceneNode(
		nullptr,
		fps->getPosition(),
		fps->getTarget(),
		-1,
		false
	);

	testp::scene::IMesh * mesh = geom->createCubeMesh(
		testp::nub::vector3df{1000, 20, 1000},
		testp::scene::ECMT_6BUF_4VTX_NP
	);
	testp::scene::IMeshManipulator * manip = scene->getMeshManipulator();
	testp::scene::IMesh * mesh_tg = manip->createMeshWithTangents(mesh);
	mesh->drop();
	mesh = nullptr;
	testp::scene::SAnimatedMesh * mesh_set = new testp::scene::SAnimatedMesh();
	mesh_set->addMesh(mesh_tg);
	mesh_tg->drop();
	mesh_tg = nullptr;
	testp::scene::IAnimatedMeshSceneNode * node = scene->addAnimatedMeshSceneNode(
		mesh_set,
		nullptr,
		-1,
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{0,0,0},
		testp::nub::vector3df{1,1,1},
		false
	);
	mesh_set->drop();
	mesh_set = nullptr;
	node->setMaterialTexture(0, video->getTexture(tex.back().string().data()));
	tex.pop_back();

	testp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{100, 100, 0},
		testp::video::SColor{0xff78fa69},
		10000,
		-1
	);
	light->setLightType(testp::video::ELT_POINT);
	testp::gui::ICursorControl * cursor = device->getCursorControl();
	cursor->setVisible(false);

	testp::video::ITexture * tex1 = video->getTexture(tex.back().string().data());
	tex.pop_back();
	if (tex1)
	{
		video->makeNormalMapTexture(tex1, 19.0f);
		node->setMaterialTexture(1, tex1);
	}

	node->setMaterialType(testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA);

	testp::gui::IGUIToolBar * bar = gui->addToolBar(nullptr, -1);

	testp::gui::IGUIButton * button = bar->addButton(
		-1,
		L"This Tool Bar",
		L"This is a tool bar",
		nullptr,
		nullptr,
		false,
		false
	);

	wd::event event{device, gui, fps, static__, cursor};
	device->setEventReceiver(&event);

	gui->getRootGUIElement()->setVisible(false);

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

