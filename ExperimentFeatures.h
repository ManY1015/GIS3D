#pragma once

#include "OsgScene.h"

#include <osg/ref_ptr>
#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>

class CExperimentFeatures;

class CExperimentPickHandler : public osgGA::GUIEventHandler
{
public:
	explicit CExperimentPickHandler(CExperimentFeatures* pOwner);
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

private:
	CExperimentFeatures* m_pOwner;
};

class CExperimentFeatures
{
public:
	CExperimentFeatures();
	~CExperimentFeatures();

	bool Initialize(COsgScene* pScene, HWND hWndOwner);

	bool SaveBookmark(CString& strMessage);
	bool RestoreBookmark(CString& strMessage);
	bool FocusBatchModels(CString& strMessage);
	bool ExportScreenshot(CString& strMessage);
	bool ToggleInspector(CString& strMessage);
	bool ToggleTerrainQuery(CString& strMessage);
	bool ToggleWireframe(CString& strMessage);

	bool HandlePick(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

private:
	struct ViewBookmark
	{
		bool bValid;
		osg::Vec3d eye;
		osg::Vec3d center;
		osg::Vec3d up;

		ViewBookmark() : bValid(false) {}
	};

	bool EnsureReady() const;
	bool CaptureClientToFile(const CString& strPath) const;
	bool GetCurrentView(osg::Vec3d& eye, osg::Vec3d& center, osg::Vec3d& up) const;
	void ApplyView(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up);
	bool PickFirstIntersection(const osgGA::GUIEventAdapter& ea, osgUtil::LineSegmentIntersector::Intersection& hit);
	CString GetNodeLayerName(osg::Node* pNode) const;
	osg::Node* FindNamedLayerFromPath(const osg::NodePath& nodePath) const;
	void ClearOverlay();
	void ShowBoundingBox(osg::Node* pNode);
	void ApplyWireframeToNode(osg::Node* pNode, bool bWireframe);

private:
	COsgScene* m_pScene;
	HWND m_hWndOwner;
	ViewBookmark m_bookmark;
	bool m_bInspectorEnabled;
	bool m_bTerrainQueryEnabled;
	bool m_bWireframeEnabled;
	osg::ref_ptr<osg::Group> m_pOverlayRoot;
	osg::ref_ptr<CExperimentPickHandler> m_pPickHandler;
};
