//
// Copyright (c) 2025 Fas Xmut (fasxmut at protonmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// commndline example:

// export-wave --help
// export-wave -t ./export-wave.texture.png -l true -c 0xffff9900 -r 12 -R 6 -i 2 -e .b3d

#include <testpub/core.hpp>
#include <iostream>
#include <future>
#include <chrono>
#include <lyra/lyra.hpp>
#include <boost/assert.hpp>
#include <botan/bigint.h>
#include <boost/signals2.hpp>

using std::numbers::pi;
using std::string_literals::operator""s;
using std::placeholders::_1;

namespace my_cpp
{
	using signal_type = boost::signals2::signal<void(const std::string &)>;
	using slot_type = my_cpp::signal_type::slot_type;
	using connection_type = boost::signals2::connection;

	class event
	{
	protected:
		my_cpp::signal_type __signal;
		std::string __sender;
	public:
		virtual ~event() = default;
	public:
		event(const std::string & sender__):
			__sender{sender__}
		{
		}
	public:
		virtual my_cpp::connection_type connect(int prior__, const my_cpp::slot_type & slot__) = 0;
	public:
		void signal(const std::string & msg__)
		{
			this->__signal("["s + __sender + "] " + msg__);
		}
	};

	class viewer
	{
	private:
		connection_type __connection;
	public:
		viewer() = default;
	public:
		virtual ~viewer()
		{
			__connection.disconnect();
		}
	public:
		void attach(int prior__, my_cpp::event & event__)
		{
			__connection = event__.connect(
				prior__,
				std::bind(
					&my_cpp::viewer::queue,
					this,
					_1
				)
			);
		}
	public:
		void queue(const std::string & msg__)
		{
			std::cout << "[queue]" << msg__ << std::endl;
		}
	};

	class wave_mesh:
		virtual public testp::scene::SMesh,
		virtual public my_cpp::event
	{
	private:
		const float radius;
		const float height;
	public:
		virtual ~wave_mesh()
		{
		}
	public:
		wave_mesh(float radius, float height, int prior__, my_cpp::viewer & viewer__):
			event{"my_cpp::wave_mesh"},
			radius{radius},
			height{height}
		{
			viewer__.attach(prior__, *this);
			this->init();
		}
	private:
		void init()
		{
			{
				auto mesh_buffer = new testp::scene::SMeshBuffer;
				this->addMeshBuffer(mesh_buffer);
				mesh_buffer->drop();
			}
			{
			/////////////////	Section: create vertices
				testp::scene::SMeshBuffer * mesh_buffer =
					static_cast<testp::scene::SMeshBuffer *>(this->getMeshBuffer(0));

				const float x_start = -radius;
				const float x_end = radius;
				const float z_start = -radius;
				const float z_end = radius;
				const float x_len = x_end - x_start;
				const float z_len = z_end - z_start;
				const float dx = 0.2;
				const float dz = 0.2;
				const float y_height = height;

				int z_count = 0;
				int x_count = 0;

				auto u_norm = [&] (float x)	// normalize U
				{
					return (x-x_start) / x_len;
				};
				auto v_norm = [&] (float z)	// normalize V
				{
					return (z_end - z) / z_len;
				};

				std::vector<testp::video::S3DVertex> vertices;
				for (float z=z_start; z<=z_end; z+=dz)
				{
					++z_count;
					for (float x=x_start; x<=x_end; x+=dx)
					{
						float y = y_height*cos(x)*cos(z);
						vertices.push_back({x,y,z,0,1,0,0x00ffffff,u_norm(x),v_norm(z)});
					}
				}
				x_count = vertices.size() / z_count;
				std::cout << "\n\nx_count: " << x_count << '\n' << "z_count: " << z_count << '\n'
					<< "total: " << vertices.size() << '\n'
					<< "total: " << x_count * z_count << "\n\n";

			/////////////////	Section: create indices
				std::vector<testp::uint16_pub> indices;
				for (int j=0; j<z_count-1; ++j)
				{
					for (int i=0; i<x_count-1; ++i)
					{
						const int linear = j*x_count+i;
						const int linear_right = linear + 1;
						const int linear_up = linear + x_count;
						const int linear_up_right = linear_up + 1;
						indices.push_back(linear);
						indices.push_back(linear_up);
						indices.push_back(linear_up_right);
						indices.push_back(linear);
						indices.push_back(linear_up_right);
						indices.push_back(linear_right);
					}
				}
				std::cout << "\nTotal indices: " << indices.size() << std::endl;
				std::cout << "\nTotal Triangles: " << indices.size()/3 << std::endl;
				{ // simple test indices
					int triangle_count_1 = (x_count - 1) * (z_count - 1) * 2;
					int triangle_count_2 = indices.size() / 3;
					std::cout << "\n\ntriangle_count_1: " << triangle_count_1 << '\n'
						<< "triangle_count_2: " << triangle_count_2 << "\n\n";
					BOOST_ASSERT_MSG(
						triangle_count_1 == triangle_count_2,
						"Test failed: indices might be wrong!"
					);
				}
			/////////////////	Section: update vertices normals
				for (int j=0; j<z_count-1; ++j)
				{
					for (int i=0; i<x_count-1; ++i)
					{
						auto points = new std::vector<testp::nub::vector3df>;
						points->resize(0);
						int linear = j*x_count + i;

						points->push_back(vertices[linear].Pos);
						points->push_back(vertices[linear+x_count].Pos);
						points->push_back(vertices[linear+1].Pos);

						testp::nub::vector3df dir1 = points->at(1) - points->at(0);
						testp::nub::vector3df dir2 = points->at(2) - points->at(0);
						testp::nub::vector3df normal = dir1.crossProduct(dir2);
						if (normal.Y < 0)
							normal = -normal;
						normal.normalize();
						vertices[linear].Normal = normal;

						delete points;
						points = nullptr;
					}
				}
			/////////////////	Section: write data
				mesh_buffer->append(
					vertices.data(),
					vertices.size(),
					indices.data(),
					indices.size()
				);
				this->setDirty();
				this->recalculateBoundingBox();
				this->signal("Wave Mesh is created");
			}
		}
	public:
		my_cpp::connection_type connect(int prior__, const my_cpp::slot_type & slot__) override
		{
			return this->__signal.connect(slot__);
		}
	};

	class mesh_exporter:
		virtual public my_cpp::event
	{
	public:
		virtual ~mesh_exporter()
		{
		}
	public:
		mesh_exporter(int prior__, my_cpp::viewer & viewer__):
			my_cpp::event{"my_cpp::mesh_exporter"}
		{
			viewer__.attach(prior__, * this);
		}
	public:
		void export_mesh(
			testp::scene::IMeshSceneNode * node__,
			testp::scene::EMESH_WRITER_TYPE type__, 
			const std::string & filename__
		)
		{
			testp::scene::IMeshWriter * mw = node__->getSceneManager()->createMeshWriter(
				type__
			);
			testp::io::IWriteFile * file = node__->getSceneManager()->getFileSystem()->createAndWriteFile(filename__.data(), false);

			testp::scene::E_MESH_WRITER_FLAGS flags = testp::scene::EMWF_NONE;
			if (type__ == testp::scene::EMWT_B3D)
				flags =
					static_cast<testp::scene::E_MESH_WRITER_FLAGS>(testp::scene::EMWF_WRITE_COMPRESSED | testp::scene::EMWF_WRITE_BINARY);
			else if (type__ == testp::scene::EMWT_IRR_MESH)
				flags = testp::scene::EMWF_NONE;
			mw->writeMesh(
				file,
				static_cast<testp::scene::SMesh *>(node__->getMesh()),
				flags
			);
			mw->drop();
			this->signal("Mesh has been written to "s + filename__);
		}
	public:
		my_cpp::connection_type connect(int prior__, const my_cpp::slot_type & slot__) override
		{
			return this->__signal.connect(prior__, slot__);
		}
	};

	class arg
	{
	public:
		std::string texture = "no";
		bool lighting = false;
		std::string light_color = "0xff0000ff";
		float radius = 4*pi;
		float Radius = 3*pi;
		float height = 2;
		std::string export_type = ".b3d";
	public:
		void print() const
		{
			std::cout << "----------------------------------------\n";
			std::cout << "texture: " << texture << std::endl;
			std::cout << "lighting: " << std::boolalpha << lighting << std::endl;
			std::cout << "light_color: " << light_color << std::endl;
			std::cout << "(arena) radius: " << radius << std::endl;
			std::cout << "(light) Radius: " << Radius << std::endl;
			std::cout << "height: " << height << std::endl;
			std::cout << "export_type: " << export_type << std::endl;
			std::cout << "----------------------------------------\n";
		}
	};
}

int main(int argc, char * argv[])
try
{
	my_cpp::arg arg;
	bool help = false;
	auto cli = lyra::help(help)
		| lyra::opt(arg.texture, "texture")["-t"]("path to texture image filename, .png or .jpg only")
		| lyra::opt(arg.lighting, "lighting")["-l"]("enable lighting?")
		| lyra::opt(arg.light_color, "light_color")["-c"]("set light color")
		| lyra::opt(arg.radius, "radius")["-r"]("The radius of the arena")
		| lyra::opt(arg.Radius, "Radius")["-R"]("The radius of the light")
		| lyra::opt(arg.height, "height")["-i"]("The height of the arena")
		| lyra::opt(arg.export_type, "export_type")["-e"]("mesh export type, .b3d or .irrmesh only")
	;
	auto result = cli.parse({argc, argv});
	if (help || argc == 1)
	{
		std::cout << cli << std::endl;
		return 0;
	}

	arg.print();
	testp::scene::EMESH_WRITER_TYPE export_type;
	std::string export_filename;
	if (arg.export_type == ".b3d")
	{
		export_type = testp::scene::EMWT_B3D;
		export_filename = "export-wave.b3d";
	}
	/*
	else if (arg.export_type == ".irrmesh")
	{
		export_type  = testp::scene::EMWT_IRR_MESH;
		export_filename = "export-wave.irrmesh";
	}
	*/
	else
	{
		throw std::runtime_error{"export_type only supports .b3d"};
	}

	const std::chrono::system_clock::time_point __start__ = std::chrono::system_clock::now();
	my_cpp::viewer viewer;

	{
		auto device = testp::createPub(testp::video::EDT_OPENGL, testp::nub::dimension2du{100,100});
		auto scene = device->getSceneManager();
		auto video = device->getVideoDriver();
		auto camera = scene->addCameraSceneNodeFPS(
			nullptr,
			20.0f,
			0.02f
		);
		camera->setPosition({0,arg.height*2, -arg.radius*1.2f});
		camera->setTarget({0, -arg.height, 0});
		device->getCursorControl()->setVisible(false);
		auto wave_mesh = new my_cpp::wave_mesh{
			arg.radius,
			arg.height,
			0,
			viewer
		};
		auto wave = scene->addMeshSceneNode(
			wave_mesh,
			nullptr,
			-1,
			{0,0,0},
			{0,0,0},
			{1,1,1},
			false
		);
		wave_mesh->drop();
		if (wave)
		{
			std::cout << "mesh buffer count: " << wave->getMesh()->getMeshBufferCount()
				<< std::endl;
			wave->setMaterialFlag(testp::video::EMF_LIGHTING, arg.lighting);
			if (arg.texture.ends_with(".png") || arg.texture.ends_with(".jpg"))
				wave->setMaterialTexture(0, video->getTexture(arg.texture.data()));

			my_cpp::mesh_exporter{1, viewer}.export_mesh(
				wave,
				export_type,
				export_filename
			);
			std::cout << "Mesh has been exported to " << export_filename << std::endl;
		}
		if (arg.lighting)
			scene->addLightSceneNode(
				camera,
				testp::nub::vector3df{0},
				testp::video::SColor{Botan::BigInt{arg.light_color}.to_u32bit()},
				arg.Radius,
				-1
			);
		while (device->run())
		{
			video->beginScene();
			scene->drawAll();
			video->endScene();
			if (std::chrono::system_clock::now() > __start__ + std::chrono::seconds(1))
				device->closeDevice();
		}
		device->drop();
	}
	{
		testp::TestpubDevice * device = testp::createPub(
			testp::video::EDT_OPENGL,
			testp::nub::dimension2du{1925, 1085},
			32,
			false,
			true,
			false,
			nullptr
		);
		testp::scene::ISceneManager * scene = device->getSceneManager();
		testp::video::IVideoDriver * video = device->getVideoDriver();
		testp::scene::ICameraSceneNode * camera = scene->addCameraSceneNodeFPS(
			nullptr,
			30.0f,
			0.005f,
			-1,
			nullptr,
			0,
			false,
			15.0f,
			false,
			true
		);
		camera->setPosition({0,arg.height*2, -arg.radius*1.2f});
		camera->setTarget({0, -arg.height, 0});
		device->getCursorControl()->setVisible(false);
		testp::scene::IAnimatedMeshSceneNode * node = scene->addAnimatedMeshSceneNode(
			scene->getMesh(export_filename.data()),
			nullptr,
			-1,
			testp::nub::vector3df{0},
			testp::nub::vector3df{0},
			testp::nub::vector3df{1},
			false
		);
		if (node)
		{
			node->setMaterialFlag(testp::video::EMF_LIGHTING, arg.lighting);
			if (arg.texture.ends_with(".png") || arg.texture.ends_with(".jpg"))
				node->setMaterialTexture(0, video->getTexture(arg.texture.data()));
			{
				class qmsg: virtual public my_cpp::event
				{
				public:
					qmsg(int prior__, my_cpp::viewer & viewer__):
						my_cpp::event{"in qmsg"}
					{
						viewer__.attach(prior__, * this);
					}
					my_cpp::connection_type connect(
						int prior__,
						const my_cpp::slot_type & slot__
					) override
					{
						return this->__signal.connect(prior__, slot__);
					}
				};
				qmsg qm{2, viewer};
				qm.signal("Mesh has been loaded.");
			}
		}
		if (arg.lighting)
			scene->addLightSceneNode(
				camera,
				testp::nub::vector3df{0,0,0},
				testp::video::SColor{Botan::BigInt{arg.light_color}.to_u32bit()},
				arg.Radius,
				-1
			);

		testp::scene::ITriangleSelector * selector = scene->createOctreeTriangleSelector(
			node->getMesh(),
			node,
			1024
		);
		node->setTriangleSelector(selector);
		testp::scene::ISceneNodeAnimator * collision = scene->createCollisionResponseAnimator(
			selector,
			camera,
			testp::nub::vector3df{arg.radius / 8, arg.radius / 5, arg.radius / 8},
			testp::nub::vector3df{0,0,0},
			testp::nub::vector3df{0,0,0},
			0.001f
		);
		selector->drop();
		camera->addAnimator(collision);
		collision->drop();
		while (device->run())
		{
			video->beginScene(
				testp::video::ECBF_COLOR | testp::video::ECBF_DEPTH | testp::video::ECBF_STENCIL,
				testp::video::SColor{0xff123456}
			);
			scene->drawAll();
			video->endScene();
		}
		device->drop();
	}
	return 0;
}
catch (const std::exception & e)
{
	std::cerr << "\n\n"
		<< "c++ std::exception:\n\n"
		<< e.what() << std::endl << std::endl;
	return 1;
}

