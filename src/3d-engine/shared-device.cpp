#include <testpub/core.hpp>
#include <memory>
#include <iostream>
#include <algorithm>
#include <bit>
#include <string_view>
#include <string>

namespace star_space
{

class device: virtual public std::enable_shared_from_this<star_space::device>
{
private:
	using self_type = star_space::device;
	self_type & self = *this;
protected:
	testp::TestpubDevice * __device;
	bool __closed;
public:
	device():
		__device{
			testp::createDevice(
				testp::video::EDT_BURNINGSVIDEO,
				testp::nub::dimension2du{1280u, 720u},
				32,
				false,
				true,
				false,
				nullptr
			)
		},
		__closed{true}
	{
		if (! __device)
			throw std::runtime_error{"create device error"};
		__closed = false;
		std::cout << "create +1" << std::endl;
	}
	void set_caption(std::string_view title__)
	{
		std::wstring wstr;
		for (char x: title__)
			wstr.push_back(static_cast<wchar_t>(x));
		std::wcout << "title: " << wstr << std::endl;
		__device->setWindowCaption(wstr.data());
	}
protected:
	void close_deivce()
	{
		if (__device)
			__device->closeDevice();
		__closed = true;
		// if __device is not ok, do nothing, it is closed already.
	}
	void drop_device()
	{
		if (__device)
		{
			__device->drop();
			__device = nullptr;
			std::cout << "destroy -1" << std::endl;
		}
		__closed = true;
	}
public:
	virtual ~device()
	{
		this->drop_device();
	}
public:
	virtual testp::video::IVideoDriver * video()
	{
		if (! __device)
			throw std::runtime_error{"no device"};
		return __device->getVideoDriver();
	}
public:
	virtual testp::scene::ISceneManager * scene()
	{
		if (! __device)
			throw std::runtime_error{"no device"};
		return __device->getSceneManager();
	}
public:
	virtual bool run_device()
	{
		if (__closed)
			return false;
		if (! __device)
			throw std::runtime_error{"no device"};
		return __device->run();
	}
public:
	virtual bool window_active() const
	{
		if (__closed)
			return false;
		if (! __device)
			return false;
		return __device->isWindowActive();
	}
	void yield()
	{
		if (__closed)
			return;
		if (! __device)
			return;
		__device->yield();
	}
public:
	virtual void run()
	{
		constexpr testp::uint16_kt all_flags = testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL;
		std::cout << "Looping ..." << std::endl;
		while (self.run_device())
		{
			if (! self.window_active())
			{
				self.yield();
				continue;
			}
			self.video()->beginScene(all_flags, testp::video::SColor{0xff123456});
			self.scene()->drawAll();
			self.video()->endScene();
		}
		std::cout << std::endl;
	}
};

}

int main()
{
	{
		testp::TestpubDevice * device = testp::createDevice(
			testp::video::EDT_BURNINGSVIDEO,
			testp::nub::dimension2du{2560u, 1440u},
			32,
			false,
			true,
			true,
			nullptr
		);
		device->drop();
	}
	{
		std::shared_ptr<star_space::device> window;
		{
			std::shared_ptr<star_space::device> __window__;
			{
				auto device = std::make_shared<star_space::device>();
				device->set_caption("c++ window");
				__window__ = device;
				std::cout << "Leaving ..." << std::endl;
			}
			std::cout << "Left!" << std::endl;
			std::cout << "Leaving again ..." << std::endl;
			window = __window__;
		}
		std::cout << "left again!" << std::endl;
		window->run();
		std::cout << "Leaving again again ..." << std::endl;
	}
	std::cout << "Left again again!" << std::endl;
}

