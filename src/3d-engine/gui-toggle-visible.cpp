//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <testpub/core.hpp>
#include <iostream>
#include <lyra/lyra.hpp>
#include <filesystem>

using std::string_literals::operator""s;

namespace ws
{

class event: virtual public testp::IEventReceiver
{
protected:
	bool __closed = false;
	testp::TestpubDevice * __device;
	testp::gui::IGUIElement * __help_text;
	bool __has_child = false;
	testp::gui::ICursorControl * __cursor;
	testp::scene::ICameraSceneNode * const __camera;
	testp::scene::ICameraSceneNode * __s_camera;
	testp::scene::IOctreeSceneNode * __world_map;
public:
	virtual ~event() = default;
public:
	event(
		testp::TestpubDevice * device__,
		testp::gui::IGUIElement * help_text__,
		testp::gui::ICursorControl * cursor__,
		testp::scene::ICameraSceneNode * get_fps_camera__,
		testp::scene::IOctreeSceneNode * world_map__
	):
		__device{device__},
		__help_text{help_text__},
		__cursor{cursor__},
		__camera{get_fps_camera__},
		__s_camera{
			__device->getSceneManager()->addCameraSceneNode(
				nullptr,
				__camera->getPosition(),
				__camera->getTarget(),
				-1,
				false
			)
		},
		__world_map{world_map__}
	{
	}
public:
	bool OnEvent(const testp::SEvent & event__) override
	{
		if (__closed)
			return false;
		switch (event__.EventType)
		{
		case testp::EET_KEY_INPUT_EVENT:
			this->process_key(event__);
			break;
		default:
			break;
		}
		return false;
	}
protected:
	void process_key(const testp::SEvent & event__)
	{
		switch (event__.KeyInput.Key)
		{
		case testp::KEY_KEY_Q:
			if (! event__.KeyInput.PressedDown)
				this->close_device();
			break;
		case testp::KEY_LCONTROL:
		case testp::KEY_RCONTROL:
			this->camera_visible(event__.KeyInput.PressedDown);
			this->cursor_visible(event__.KeyInput.PressedDown);
			this->help_text_visible(event__.KeyInput.PressedDown);
			break;
		case testp::KEY_KEY_C:
			if (! event__.KeyInput.PressedDown)
				this->camera_active();
			break;
		case testp::KEY_KEY_M:
			if (! event__.KeyInput.PressedDown)
				this->world_visible();
			break;
		default:
			break;
		}
	}
protected:
	void close_device()
	{
		if (__closed)
			return;
		if (__device)
			__device->closeDevice();
		__closed = true;
	}
protected:
	void camera_visible(bool visible__)
	{
		if (visible__)
		{
			__s_camera->setPosition(__camera->getPosition());
			__s_camera->setTarget(__camera->getTarget());
			__device->getSceneManager()->setActiveCamera(__s_camera);
		}
		else
		{
			__device->getSceneManager()->setActiveCamera(__camera);
		}
	}
	void cursor_visible(bool visible__)
	{
		__cursor->setVisible(visible__);
	}
	void help_text_visible(bool visible__)
	{
		__help_text->setVisible(visible__);
	}
	void camera_active()
	{
		testp::scene::ICameraSceneNode * active_camera = __device->getSceneManager()->getActiveCamera();
		if (active_camera)
			__device->getSceneManager()->setActiveCamera(nullptr);
		else
			__device->getSceneManager()->setActiveCamera(__camera);
	}
	void world_visible()
	{
		__world_map->setVisible(
			! __world_map->isVisible()
		);
	}
};

}	// namespace ws

int main(int argc, char * argv[])
{
	bool help = false;
	std::filesystem::path filesystem;
	std::filesystem::path map_name;
	auto cli = lyra::help(help)
		| lyra::opt(filesystem, "filesystem")["-f"]("File System Path")
		| lyra::opt(map_name, "map_name")["-m"]("World map path")
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
		true,
		nullptr
	);
	testp::scene::ISceneManager * scene = device->getSceneManager();
	testp::video::IVideoDriver * video = device->getVideoDriver();

	testp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
		nullptr,
		75.0f,
		0.1f,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);

	testp::gui::ICursorControl * cursor = device->getCursorControl();
	cursor->setVisible(false);

	testp::io::IFileSystem * fs = device->getFileSystem();

	if (! fs->addFileArchive(filesystem.string().data()))
		throw std::runtime_error{"can not add file system "s + filesystem.string()};
	else
		std::cout << filesystem << " is addded" << std::endl;

	testp::scene::IOctreeSceneNode * map = scene->addOctreeSceneNode(
		scene->getMesh(map_name.string().data())->getMesh(0),
		nullptr,
		-1,
		256,
		false
	);
	if (! map)
		throw std::runtime_error{"Map can not be loaded"};
	else
		std::cout << map_name << " is loaded" << std::endl;

	testp::gui::IGUIEnvironment * gui = device->getGUIEnvironment();

	testp::gui::IGUIStaticText * help_text = gui->addStaticText(
		L"Help:\n"
		L"	CTRL - Hold control to view this help menu.\n"
		L"	Q - Press Q to quit\n"
		L"	C - Press C to active/deactive camera\n"
		L"	M - Press M to show/hide world\n"
		L"\n\n"
		L"	W - Walk forward\n"
		L"	S - Walk Backward\n"
		L"	A - Walk Left\n"
		L"	D - Walk Right\n"
		,
		testp::nub::recti{800, 300, 1200, 800},
		true,
		true,
		nullptr,
		-1,
		true
	);
	help_text->setVisible(false);

	ws::event event{
		device,
		help_text,
		cursor,
		camera,
		map
	};

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
	std::cout << "Device is dropped successfully" << std::endl;
}

