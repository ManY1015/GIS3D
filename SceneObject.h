#pragma once
class CSceneObject
{

public:
	CSceneObject(void);
	~CSceneObject(void);

public:
	osg::Group* CreateTerrain(CString& strDemName, CString& strImageName, SceneObjectInfo* pInfo = NULL);
	osg::Group* LoadModel(CString filename);
};

