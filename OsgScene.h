#pragma once

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/api/win32/GraphicsWindowWin32>
#include <osgGA/TrackballManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgDB/DatabasePager>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <string>

#include "Scene.h"

class COsgScene
{
public:
    COsgScene(HWND hWnd);
    ~COsgScene(void);

    void InitOSG(std::string filename);
    void InitManipulators(void);
    void InitSceneGraph(void);
    void InitCameraConfig(void);
    void SetupWindow(void);
    void SetupCamera(void);
    void PreFrameUpdate(void);
    void PostFrameUpdate(void);
    void Done(bool value) { mDone = value; }
    bool Done(void) { return mDone; }
    static void Render(void* ptr);
    //渲染,注意这里是静态的,可启用为全局线程
//	static void Render(void* ptr);

    osg::ref_ptr<osg::Node> CreateLand();
    osgViewer::Viewer* getViewer() { return mViewer; }
    osg::Group* getRoot() { return mRoot.get(); }
    //	osg::ref_ptr<osg::Node> CreateTriangles();

private:
    //	osg::ref_ptr<osg::Node> mLandModel;
    osg::ref_ptr<osg::Node> mTerainModel;
    //osg::ref_ptr<osgOcean::OceanScene> scene;
    osg::ref_ptr<CScene> scene;
    bool mDone;
    std::string m_ModelName;
    HWND m_hWnd;
    osgViewer::Viewer* mViewer;
    osg::ref_ptr<osg::Group> mRoot;
    osg::ref_ptr<osg::Node> mModel;
    osg::ref_ptr<osgGA::TrackballManipulator> trackball;
    osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator;
    //对称透视视景体的平裁头的纵横比，初始化时为窗体的尺寸，width/height
    double Ratio;
};

