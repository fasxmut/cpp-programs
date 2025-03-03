#include <testpub/core.hpp>
#include <filesystem>
#include <lyra/lyra.hpp>

int main(int argc, char * argv[])
{
	bool help = false;

	std::filesystem::path lm_tex;
	std::filesystem::path f_tex;
	std::filesystem::path w1_tex;
	std::filesystem::path w2_tex;
	std::filesystem::path w3_tex;
	std::filesystem::path w4_tex;

	auto cli = lyra::help(help)
		| lyra::opt(lm_tex, "lm_tex")["-l"]("lightmap texture")
		| lyra::opt(f_tex, "f_tex")["-f"]("floor texture")
		| lyra::opt(w1_tex, "w1_tex")["-1"]("wallet 1 texture")
		| lyra::opt(w2_tex, "w2_tex")["-2"]("wallet 2 texture")
		| lyra::opt(w3_tex, "w3_tex")["-3"]("wallet 3 texture")
		| lyra::opt(w4_tex, "w4_tex")["-4"]("wallet 4 texture")
	;
	std::cout << cli << std::endl;
	cli.parse({argc, argv});

	testp::TestpubDevice * device = testp::createPub(
		testp::video::EDT_EGXU,
		testp::nub::dimension2du{1280, 720},
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
		35,
		0.3,
		-1,
		nullptr,
		0,
		false,
		30,
		false,
		true
	);
	camera->setPosition(testp::nub::vector3df{400,400,400});
	camera->setTarget(testp::nub::vector3df{0,350,0});
	testp::gui::ICursorControl * cursor = device->getCursorControl();
	cursor->setVisible(false);

	const testp::scene::IGeometryCreator * geo = scene->getGeometryCreator();

	const testp::scene::IMeshManipulator * manip = scene->getMeshManipulator();

	auto make_wall = [&] (
		const testp::nub::vector3df & size,
		const std::filesystem::path & tex,
		testp::video::ITexture * lm = nullptr
	) -> testp::scene::IAnimatedMeshSceneNode *
	{
		testp::scene::IAnimatedMeshSceneNode * node = nullptr;
		testp::scene::IMesh * mesh = geo->createCubeMesh(
			size,
			testp::scene::ECMT_6BUF_4VTX_NP
		);
		testp::scene::IMesh * t_mesh = manip->createMeshWithTangents(mesh);
		mesh->drop();
		auto a_mesh = new testp::scene::SAnimatedMesh;
		a_mesh->addMesh(t_mesh);
		t_mesh->drop();
		node = scene->addAnimatedMeshSceneNode(
			a_mesh,
			nullptr,
			-1,
			testp::nub::vector3df{0,0,0},
			testp::nub::vector3df{0,0,0},
			testp::nub::vector3df{1,1,1},
			false
		);
		a_mesh->drop();
		node->setMaterialFlag(testp::video::EMF_LIGHTING, true);
		node->setMaterialFlag(testp::video::EMF_BACK_FACE_CULLING, true);
		node->setMaterialTexture(0, video->getTexture(tex.string().data()));
		if (lm)
			node->setMaterialTexture(1, lm);
		return node;
	};

	auto lm = video->getTexture(lm_tex.string().data());
	video->makeNormalMapTexture(lm, 9.0);

	auto floor = make_wall({1000, 10, 1000}, f_tex, lm);
	floor->setMaterialType(testp::video::EMT_NORMAL_MAP_SOLID);

	auto wall1 = make_wall({10, 600, 1000}, w1_tex, lm);
	wall1->setPosition(testp::nub::vector3df{500,300,0});
	wall1->setMaterialType(testp::video::EMT_PARALLAX_MAP_SOLID);

	auto wall2 = make_wall({10, 600, 1000}, w2_tex, lm);
	wall2->setPosition(testp::nub::vector3df{-500,300,0});
	wall2->setMaterialType(testp::video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA);

	auto wall3 = make_wall({1000, 600, 10}, w3_tex, lm);
	wall3->setPosition(testp::nub::vector3df{0, 300, 500});
	wall3->setMaterialType(testp::video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA);

	auto wall4 = make_wall({1000, 600, 10}, w4_tex, lm);
	wall4->setPosition(testp::nub::vector3df{0, 300, -500});
	wall4->setMaterialType(testp::video::EMT_SOLID);

	testp::scene::ILightSceneNode * light = scene->addLightSceneNode(
		nullptr,
		testp::nub::vector3df{400, 400, 400},
		testp::video::SColor{0xffff7722},
		1000,
		-1
	);

	testp::scene::ISceneNodeAnimator * ani = scene->createFlyCircleAnimator(
		testp::nub::vector3df{0,400,0},	// center
		300.0f,	// radius
		0.004f,	// speed,	radians per milliseconds
		testp::nub::vector3df{0,1,0},	// up vector
		0,	// start, from 0 to 1
		0	// ellipsoid, from 0 to 1
	);
	light->addAnimator(ani);
	ani->drop();
	ani = nullptr;

	while (device->run())
	{
		if (! device->isWindowActive())
		{
			device->yield();
			continue;
		}
		video->beginScene(true, true, testp::video::SColor{0xff123456});
		scene->drawAll();
		video->endScene();
	}
	device->drop();
}

