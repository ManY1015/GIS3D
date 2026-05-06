#ifndef _DATATYPE_H
#define _DATATYPE_H

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/api/win32/GraphicsWindowWin32>
#include <osgGA/TrackballManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgDB/DatabasePager>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>


#define PI  3.14159265358979323846


#define WM_OSG_MY_MSG   WM_USER + 5004


enum SceneManageMessage
{
	SCENE_MSG_ADD_LAYER = 0,
	SCENE_MSG_OPACITY = 1,
	SCENE_MSG_VISIBLE = 2,
	SCENE_MSG_ORDER = 3,
	SCENE_MSG_DELETE_LAYER = 4,
	SCENE_MSG_RENAME_LAYER = 5
};


typedef struct _point3d 
{
	double x;
	double y;
	double z;
	int id;
	_point3d(double x=0,double y=0,double z=0):x(x),y(y),z(z){};
}Point3;


// 层管理信息
// 1-透明度 2-是否显示 3-图层顺序 4-删除图层 5-新增图层
// 扩展：新增图层重命名消息，供图层面板右键编辑属性使用。
typedef struct _sceneobjectinfo
{
	int    nMsg;
	char   szLayerName[200];
	char   szNewLayerName[200];
	float  fOpacity;
	float  fZScale;
	bool   bVisible;
	int    nNewPos;
	int    nObject;
	osg::Group* pObject; 
	bool   bHasGeoReference;
	double adfGeoTransform[6];
	double dGeoOriginX;
	double dGeoOriginY;
	double dGeoOriginZ;
	double dGeoSceneScale;
	double dGeoUnitScaleX;
	double dGeoUnitScaleY;
	double dGeoMinX;
	double dGeoMinY;
	double dGeoMaxX;
	double dGeoMaxY;
	char   szProjection[2048];
}SceneObjectInfo;



#endif //_DATATYPE_H
