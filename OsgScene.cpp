#include "pch.h"
#include "OsgScene.h"

#include "gis3d.h"


#include <osg/Notify>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/fstream>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>

#include <iostream>
#include <stdio.h>
#include <string.h>

#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/FlightManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osg/TextureCubeMap>
#include <osg/MatrixTransform>


#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/Program>
#include <osg/LightSource>

#include <osgShadow/ShadowedScene>
#include <osgShadow/ViewDependentShadowMap>
#include <osgGA/StateSetManipulator>

#include "SceneEventHandler.h"
#include <osg/Geode>
#include <osgUtil/Tessellator>
#include <osg/AlphaFunc>
#include <osgGA/TerrainManipulator>
#include <osg/BlendFunc>

#include "SceneManageHandler.h"

using namespace osg;

COsgScene::COsgScene(HWND hWnd) :
	m_hWnd(hWnd)
{
	mViewer = NULL;
}


COsgScene::~COsgScene(void)
{
	mViewer->setDone(true);
	Sleep(1000);
	mViewer->stopThreading();

	delete mViewer;
}


void COsgScene::InitOSG(std::string modelname)
{
	// Store the name of the model to load
	m_ModelName = modelname;

	// Init different parts of OSG
	InitManipulators();
	InitSceneGraph();
	InitCameraConfig();
}

void COsgScene::InitManipulators(void)
{
	// Create a trackball manipulator
	trackball = new osgGA::TrackballManipulator();

	// Create a Manipulator Switcher
	keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;

	// Add our trackball manipulator to the switcher
	//设置多个漫游器
	keyswitchManipulator->addMatrixManipulator('1', "Trackball", trackball.get());
	keyswitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator);
	keyswitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator);
	keyswitchManipulator->addMatrixManipulator('4', "Terrain", new osgGA::TerrainManipulator);
	// Init the switcher to the first manipulator (in this case the only manipulator)
	keyswitchManipulator->selectMatrixManipulator(0);  // Zero based index Value
}

void COsgScene::InitSceneGraph(void)
{
	// Init the main Root Node/Group
	mRoot = new osg::Group;
	osg::notify(osg::NOTICE) << "[BG-DIAG] root created valid=" << (mRoot.valid()?"YES":"NO") << std::endl;
    BG_DIAG_LOG("[BG-DIAG] root created valid=" << (mRoot.valid()?"YES":"NO"));


	std::string strPath = theApp.GetExePath();

	scene = new CScene(strPath);
	double oceanSurfaceHeight = 0.0f;
	osg::ref_ptr<osg::Group> environment_root;
	bool useDebugDraw = false;
	bool disableShaders = false;
	
	{
		environment_root = new osg::Group;
	}

	
	environment_root->addChild(scene->getScene());
	osg::notify(osg::NOTICE) << "[BG-DIAG] environment child count after scene add=" << environment_root->getNumChildren() << std::endl;
    BG_DIAG_LOG("[BG-DIAG] environment child count after scene add=" << environment_root->getNumChildren());
	mRoot->addChild(environment_root);
	osg::notify(osg::NOTICE) << "[BG-DIAG] root child count after environment add=" << mRoot->getNumChildren() << std::endl;

	osgUtil::Optimizer optimizer;
	optimizer.optimize(mRoot);
	optimizer.reset();

	


}

void COsgScene::InitCameraConfig(void)
{
	// Local Variable to hold window size data
	RECT rect;

	// Create the viewer for this window
	mViewer = new osgViewer::Viewer();
	osg::notify(osg::NOTICE) << "[BG-DIAG] viewer created=" << (mViewer!=NULL?"YES":"NO") << std::endl;
    BG_DIAG_LOG("[BG-DIAG] viewer created=" << (mViewer!=NULL?"YES":"NO"));

	// Add a Stats Handler to the viewer
	mViewer->addEventHandler(new osgViewer::StatsHandler);


	// Get the current window size
	::GetWindowRect(m_hWnd, &rect);

	// Init the GraphicsContext Traits
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;

	// Init the Windata Variable that holds the handle for the Window to display OSG in.
	osg::ref_ptr<osg::Referenced> windata = new osgViewer::GraphicsWindowWin32::WindowData(m_hWnd);

	// Setup the traits parameters
	traits->x = 0;
	traits->y = 0;
	traits->width = rect.right - rect.left;
	traits->height = rect.bottom - rect.top;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;
	traits->setInheritedWindowPixelFormat = true;
	traits->inheritedWindowData = windata;

	// Create the Graphics Context
	osg::GraphicsContext* gc = osg::GraphicsContext::createGraphicsContext(traits.get());
	osg::notify(osg::NOTICE) << "[BG-DIAG] graphics context created=" << (gc?"YES":"NO") << std::endl;

	// Init Master Camera for this View
	osg::ref_ptr<osg::Camera> camera = mViewer->getCamera();
	osg::notify(osg::NOTICE) << "[BG-DIAG] camera created=" << (camera.valid()?"YES":"NO") << std::endl;
    BG_DIAG_LOG("[BG-DIAG] camera created=" << (camera.valid()?"YES":"NO"));

	// Assign Graphics Context to the Camera
	camera->setGraphicsContext(gc);

	// Set the viewport for the Camera
	camera->setViewport(new osg::Viewport(/*traits->x, traits->y*/0, 0, traits->width, traits->height));
	osg::notify(osg::NOTICE) << "[BG-DIAG] viewport=0,0," << traits->width << "," << traits->height << std::endl;
    BG_DIAG_LOG("[BG-DIAG] viewport=0,0," << traits->width << "," << traits->height);

	// set the draw and read buffers up for a double buffered window with rendering going to back buffer
	camera->setDrawBuffer(GL_BACK);
	camera->setReadBuffer(GL_BACK);

	// Set projection matrix and camera attribtues
	camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	// 修复天空/地面交界处黑边：清屏色恢复为接近天空盒的蓝色，避免边界像素漏出时出现黑边。
	camera->setClearColor(osg::Vec4f(0.f, 0.f, 0.f, 1.0f));//osg::Vec4f(0.2f, 0.2f, 0.4f, 1.0f));
	osg::notify(osg::NOTICE) << "[BG-DIAG] clear color=0,0,0,1.0" << std::endl;
    BG_DIAG_LOG("[BG-DIAG] clear color=0,0,0,1.0");
	camera->setProjectionMatrixAsPerspective(45.f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 0.1, 100000.0);
	osg::notify(osg::NOTICE) << "[BG-DIAG] projection fovy=45 aspect=" << (static_cast<double>(traits->width) / static_cast<double>(traits->height)) << " near=0.1 far=100000" << std::endl;
    BG_DIAG_LOG("[BG-DIAG] projection fovy=45 aspect=" << (static_cast<double>(traits->width) / static_cast<double>(traits->height)) << " near=0.1 far=100000");
	osg::notify(osg::NOTICE) << "[BG-DIAG] nearFarMode=" << (int)camera->getComputeNearFarMode() << " nearFarRatio=" << camera->getNearFarRatio() << std::endl;
    BG_DIAG_LOG("[BG-DIAG] nearFarMode=" << (int)camera->getComputeNearFarMode() << " nearFarRatio=" << camera->getNearFarRatio());
	// Add the Camera to the Viewer
	//mViewer->addSlave(camera.get());
	mViewer->setCamera(camera.get());
	//	osg::Vec2s size(traits->width,traits->height);
		//scene->getOceanScene()->setScreenDims(size);
	mViewer->addEventHandler(new osgViewer::StatsHandler);
	mViewer->addEventHandler(new osgGA::StateSetManipulator(mViewer->getCamera()->getOrCreateStateSet()));

	osg::ref_ptr<TextHUD> hud = new TextHUD;

	// Add the Camera Manipulator to the Viewer
 //   mViewer->setCameraManipulator(keyswitchManipulator.get());

	mViewer->getCamera()->addChild(hud->getHudCamera());

	// 	mViewer->addEventHandler(scene->getOceanSceneEventHandler());
	// 	mViewer->addEventHandler(scene->getOceanSurface()->getEventHandler());

	osg::Vec3 initialCameraPosition(10, -10, 25);//8);//118,32,30);//11800,3200,20);//118000,32000,20);//0,0,20);//0,-50,50);//相机位置  ，高度不高于50
	osg::Vec3 initialCenterPosition(15, -5, 25);//8);
	mViewer->addEventHandler(new CSceneEventHandler(scene.get(), hud.get(), mViewer, initialCameraPosition, initialCenterPosition));
	mViewer->addEventHandler(new osgViewer::HelpHandler);
	mViewer->addEventHandler(new CSceneManageHandler(m_hWnd, mRoot, mViewer));//加入自定义层管理   jwp  2013.12.13
	mViewer->getCamera()->setName("MainCamera");




			// Set the Scene Data

	mViewer->setSceneData(mRoot.get());
	osg::notify(osg::NOTICE) << "[BG-DIAG] scene data set rootValid=" << (mRoot.valid()?"YES":"NO") << " rootChildCount=" << (mRoot.valid()?mRoot->getNumChildren():0) << std::endl;
    BG_DIAG_LOG("[BG-DIAG] scene data set rootValid=" << (mRoot.valid()?"YES":"NO") << " rootChildCount=" << (mRoot.valid()?mRoot->getNumChildren():0));
	if (mRoot.valid()) { osg::BoundingSphere bs = mRoot->getBound(); osg::notify(osg::NOTICE) << "[BG-DIAG] root bound center/radius=" << bs.center().x() << "," << bs.center().y() << "," << bs.center().z() << " / " << bs.radius() << std::endl; BG_DIAG_LOG("[BG-DIAG] root bound center/radius=" << bs.center().x() << "," << bs.center().y() << "," << bs.center().z() << " / " << bs.radius()); }

	mViewer->setKeyEventSetsDone(0);
	mViewer->setQuitEventSetsDone(false);
	// Realize the Viewer
	mViewer->realize();
	osg::notify(osg::NOTICE) << "[BG-DIAG] InitCameraConfig end viewer realized" << std::endl;

	
}

void COsgScene::PreFrameUpdate()
{
	// Due any preframe updates in this routine
}

void COsgScene::PostFrameUpdate()
{

	::SendMessageA(m_hWnd, WM_OSG_MY_MSG, 1, 0);
}


void COsgScene::Render(void* ptr)
{
	COsgScene* osg = (COsgScene*)ptr;

	osgViewer::Viewer* viewer = osg->getViewer();

	
	bool bLoggedFirstFrame = false;
	while (!viewer->done())
	{
		if (!bLoggedFirstFrame) { osg::notify(osg::NOTICE) << "[BG-DIAG] first frame render loop entered" << std::endl; BG_DIAG_LOG("[BG-DIAG] first frame render loop entered"); bLoggedFirstFrame = true; }
		osg->PreFrameUpdate();
		viewer->frame();
		osg->PostFrameUpdate();
		//Sleep(10);         // Use this command if you need to allow other processes to have cpu time
	}

	

	_endthread();
}

