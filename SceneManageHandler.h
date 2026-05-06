#pragma once

#include <osgGA/GUIEventHandler>
// #include <osgGA/FlightManipulator>
// #include <osgGA/TrackballManipulator>
// #include <osgGA/DriveManipulator>

//自定义消息传递的结构
struct SceneManageInfo : public osg::Referenced
{
	SceneManageInfo(SceneObjectInfo info){memmove(&m_info,&info,sizeof(SceneObjectInfo));}
	SceneObjectInfo m_info;	
};

class CSceneManageHandler : public osgGA::GUIEventHandler
{
public:
	CSceneManageHandler(HWND hWnd,osg::ref_ptr<osg::Group> pRoot,osgViewer::Viewer *mViewer);
	~CSceneManageHandler(void);
	virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa);
	
	osg::ref_ptr<osg::LightSource> createLight(osg::BoundingSphere bs);
	HWND m_hWnd;
	osg::ref_ptr<osg::Group> m_pRoot;
	osgViewer::Viewer *m_pViewer;
};

