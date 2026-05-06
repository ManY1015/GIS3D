#include "pch.h"
#include "ExperimentFeatures.h"

#include <atlimage.h>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Group>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <osgUtil/LineSegmentIntersector>

namespace
{
	CString FormatVector(const osg::Vec3d& v)
	{
		CString str;
		str.Format(_T("(%.3f, %.3f, %.3f)"), v.x(), v.y(), v.z());
		return str;
	}

	bool IsTerrainLayerName(const CString& strLayerName)
	{
		return strLayerName.Find(_T("地形")) >= 0
			|| strLayerName.Find(_T("Terrain")) >= 0
			|| strLayerName.Find(_T("terrain")) >= 0
			|| strLayerName.Find(_T("鍦板舰")) >= 0;
	}

	bool IsBatchModelLayerName(const CString& strLayerName)
	{
		return strLayerName.Left(3) == _T("妯″瀷_");
	}

	const char* const kTerrainNodeMarker = "__GIS3D_TERRAIN__";
	const char* const kTerrainLayerMarker = "__GIS3D_TERRAIN_LAYER__";

	bool IsTerrainNodePath(const osg::NodePath& nodePath)
	{
		for (osg::NodePath::const_reverse_iterator it = nodePath.rbegin(); it != nodePath.rend(); ++it)
		{
			osg::Node* pNode = *it;
			if (!pNode)
				continue;

			std::string strName = pNode->getName();
			if (strName == kTerrainNodeMarker || strName == kTerrainLayerMarker)
				return true;

			CString strLayerName = strName.c_str();
			if (IsTerrainLayerName(strLayerName))
				return true;
		}

		return false;
	}
}

CExperimentPickHandler::CExperimentPickHandler(CExperimentFeatures* pOwner)
	: m_pOwner(pOwner)
{
}

bool CExperimentPickHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
	if (!m_pOwner)
		return false;

	if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH &&
		ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] OSG pick handler received left push x=" << ea.getX() << " y=" << ea.getY() << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] OSG pick handler received left push x=" << ea.getX() << " y=" << ea.getY());
	}

	return m_pOwner->HandlePick(ea, aa);
}

CExperimentFeatures::CExperimentFeatures()
	: m_pScene(NULL)
	, m_hWndOwner(NULL)
	, m_bInspectorEnabled(false)
	, m_bTerrainQueryEnabled(false)
	, m_bWireframeEnabled(false)
{
}

CExperimentFeatures::~CExperimentFeatures()
{
	ClearOverlay();
}

bool CExperimentFeatures::Initialize(COsgScene* pScene, HWND hWndOwner)
{
	m_pScene = pScene;
	m_hWndOwner = hWndOwner;
	if (!EnsureReady())
		return false;

	if (!m_pOverlayRoot.valid())
	{
		m_pOverlayRoot = new osg::Group;
		m_pOverlayRoot->setName("ExperimentOverlay");
		m_pScene->getRoot()->addChild(m_pOverlayRoot.get());
	}

	if (!m_pPickHandler.valid())
	{
		m_pPickHandler = new CExperimentPickHandler(this);
		m_pScene->getViewer()->addEventHandler(m_pPickHandler.get());
	}

	return true;
}

bool CExperimentFeatures::EnsureReady() const
{
	return m_pScene != NULL && m_pScene->getViewer() != NULL && m_pScene->getRoot() != NULL;
}

bool CExperimentFeatures::GetCurrentView(osg::Vec3d& eye, osg::Vec3d& center, osg::Vec3d& up) const
{
	if (!EnsureReady() || !m_pScene->getViewer()->getCamera())
		return false;

	m_pScene->getViewer()->getCamera()->getViewMatrixAsLookAt(eye, center, up);
	return true;
}

void CExperimentFeatures::ApplyView(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up)
{
	if (!EnsureReady())
		return;

	osgViewer::Viewer* pViewer = m_pScene->getViewer();
	osgGA::CameraManipulator* pManipulator = pViewer->getCameraManipulator();
	if (pManipulator)
	{
		pManipulator->setHomePosition(eye, center, up, false);
		pManipulator->home(0.0);
	}
	else if (pViewer->getCamera())
	{
		pViewer->getCamera()->setViewMatrixAsLookAt(eye, center, up);
	}

	pViewer->requestRedraw();
}

bool CExperimentFeatures::SaveBookmark(CString& strMessage)
{
	osg::Vec3d eye, center, up;
	if (!GetCurrentView(eye, center, up))
	{
		strMessage = _T("Bookmark save failed: scene view is not ready.");
		return false;
	}

	m_bookmark.bValid = true;
	m_bookmark.eye = eye;
	m_bookmark.center = center;
	m_bookmark.up = up;

	strMessage.Format(_T("Bookmark saved\r\nEye: %s\r\nCenter: %s"),
		FormatVector(eye), FormatVector(center));
	return true;
}

bool CExperimentFeatures::RestoreBookmark(CString& strMessage)
{
	if (!m_bookmark.bValid)
	{
		strMessage = _T("No saved bookmark is available.");
		return false;
	}

	ApplyView(m_bookmark.eye, m_bookmark.center, m_bookmark.up);
	strMessage = _T("Bookmark view restored.");
	return true;
}

bool CExperimentFeatures::FocusBatchModels(CString& strMessage)
{
	if (!EnsureReady())
	{
		strMessage = _T("Batch overview failed: scene is not ready.");
		return false;
	}

	osg::Group* pRoot = m_pScene->getRoot();
	osg::BoundingBoxd bb;
	bool bFound = false;

	for (unsigned int i = 0; i < pRoot->getNumChildren(); ++i)
	{
		osg::Node* pNode = pRoot->getChild(i);
		if (!pNode)
			continue;

		CString strName = GetNodeLayerName(pNode);
		if (!IsBatchModelLayerName(strName))
			continue;

		osg::BoundingSphere bs = pNode->getBound();
		if (bs.radius() <= 0.0)
			continue;

		bb.expandBy(osg::Vec3d(bs.center().x() - bs.radius(), bs.center().y() - bs.radius(), bs.center().z() - bs.radius()));
		bb.expandBy(osg::Vec3d(bs.center().x() + bs.radius(), bs.center().y() + bs.radius(), bs.center().z() + bs.radius()));
		bFound = true;
	}

	if (!bFound)
	{
		strMessage = _T("No batch model layers are available for overview.");
		return false;
	}

	osg::Vec3d center = bb.center();
	osg::Vec3d size(bb.xMax() - bb.xMin(), bb.yMax() - bb.yMin(), bb.zMax() - bb.zMin());
	double radius = size.length() * 0.5;
	if (radius < 100.0)
		radius = 100.0;

	ApplyView(osg::Vec3d(center.x(), center.y() - radius * 1.8, center.z() + radius * 1.2), center, osg::Vec3d(0.0, 0.0, 1.0));
	strMessage = _T("Camera moved to batch model overview.");
	return true;
}

bool CExperimentFeatures::CaptureClientToFile(const CString& strPath) const
{
	if (m_hWndOwner == NULL)
		return false;

	CRect rectClient;
	::GetClientRect(m_hWndOwner, &rectClient);
	int width = rectClient.Width();
	int height = rectClient.Height();
	if (width <= 0 || height <= 0)
		return false;

	HDC hWindowDC = ::GetDC(m_hWndOwner);
	if (!hWindowDC)
		return false;

	HDC hMemoryDC = ::CreateCompatibleDC(hWindowDC);
	if (!hMemoryDC)
	{
		::ReleaseDC(m_hWndOwner, hWindowDC);
		return false;
	}

	CImage image;
	image.Create(width, height, 32);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemoryDC, (HBITMAP)image);
	::BitBlt(hMemoryDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY);
	::SelectObject(hMemoryDC, hOldBitmap);

	HRESULT hr = image.Save(strPath);

	::DeleteDC(hMemoryDC);
	::ReleaseDC(m_hWndOwner, hWindowDC);
	image.Destroy();

	return SUCCEEDED(hr);
}

bool CExperimentFeatures::ExportScreenshot(CString& strMessage)
{
	CString strFilter = _T("Bitmap Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png||");
	CFileDialog dlg(FALSE, _T("bmp"), _T("scene_capture.bmp"),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, NULL);
	dlg.m_ofn.lpstrTitle = _T("Export Scene Screenshot");
	if (dlg.DoModal() != IDOK)
	{
		strMessage = _T("Screenshot export canceled.");
		return false;
	}

	if (!CaptureClientToFile(dlg.GetPathName()))
	{
		strMessage = _T("Screenshot export failed.");
		return false;
	}

	strMessage.Format(_T("Screenshot exported\r\n%s"), dlg.GetPathName());
	return true;
}

bool CExperimentFeatures::ToggleInspector(CString& strMessage)
{
	m_bInspectorEnabled = !m_bInspectorEnabled;
	if (m_bInspectorEnabled)
	{
		m_bTerrainQueryEnabled = false;
		strMessage = _T("Inspector enabled\r\nLeft-click a scene object to view its properties.");
	}
	else
	{
		ClearOverlay();
		strMessage = _T("Inspector disabled.");
	}
	return true;
}

bool CExperimentFeatures::ToggleTerrainQuery(CString& strMessage)
{
	m_bTerrainQueryEnabled = !m_bTerrainQueryEnabled;
	osg::notify(osg::NOTICE) << "[ELEV-DIAG] ToggleTerrainQuery called enabled=" << (m_bTerrainQueryEnabled ? "true" : "false") << std::endl;
	BG_DIAG_LOG("[ELEV-DIAG] ToggleTerrainQuery called enabled=" << (m_bTerrainQueryEnabled ? "true" : "false"));
	if (m_bTerrainQueryEnabled)
	{
		m_bInspectorEnabled = false;
		ClearOverlay();
		strMessage = _T("Terrain query enabled\r\nLeft-click a terrain layer to query elevation.");
	}
	else
	{
		strMessage = _T("Terrain query disabled.");
	}
	return true;
}

void CExperimentFeatures::ApplyWireframeToNode(osg::Node* pNode, bool bWireframe)
{
	if (!pNode)
		return;

	osg::StateSet* pStateSet = pNode->getOrCreateStateSet();
	osg::ref_ptr<osg::PolygonMode> pPolygonMode = new osg::PolygonMode;
	pPolygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, bWireframe ? osg::PolygonMode::LINE : osg::PolygonMode::FILL);
	pStateSet->setAttributeAndModes(pPolygonMode.get(), osg::StateAttribute::ON);

	if (bWireframe)
	{
		osg::ref_ptr<osg::LineWidth> pLineWidth = new osg::LineWidth(1.5f);
		pStateSet->setAttributeAndModes(pLineWidth.get(), osg::StateAttribute::ON);
	}
}

bool CExperimentFeatures::ToggleWireframe(CString& strMessage)
{
	if (!EnsureReady())
	{
		strMessage = _T("Wireframe toggle failed: scene is not ready.");
		return false;
	}

	m_bWireframeEnabled = !m_bWireframeEnabled;
	osg::Group* pRoot = m_pScene->getRoot();
	for (unsigned int i = 0; i < pRoot->getNumChildren(); ++i)
	{
		osg::Node* pNode = pRoot->getChild(i);
		if (!pNode)
			continue;

		CString strName = GetNodeLayerName(pNode);
		if (strName.IsEmpty())
			continue;

		ApplyWireframeToNode(pNode, m_bWireframeEnabled);
	}

	m_pScene->getViewer()->requestRedraw();
	strMessage = m_bWireframeEnabled ? _T("Wireframe inspection mode enabled.") : _T("Wireframe inspection mode disabled.");
	return true;
}

bool CExperimentFeatures::PickFirstIntersection(const osgGA::GUIEventAdapter& ea, osgUtil::LineSegmentIntersector::Intersection& hit)
{
	if (!EnsureReady())
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] PickFirstIntersection failed: scene/viewer/root not ready" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] PickFirstIntersection failed: scene/viewer/root not ready");
		return false;
	}

	osgUtil::LineSegmentIntersector::Intersections intersections;
	bool bHit = m_pScene->getViewer()->computeIntersections(ea.getX(), ea.getY(), intersections);
	osg::notify(osg::NOTICE) << "[ELEV-DIAG] computeIntersections x=" << ea.getX() << " y=" << ea.getY() << " hit=" << (bHit ? "true" : "false") << " count=" << intersections.size() << std::endl;
	BG_DIAG_LOG("[ELEV-DIAG] computeIntersections x=" << ea.getX() << " y=" << ea.getY() << " hit=" << (bHit ? "true" : "false") << " count=" << intersections.size());
	if (!bHit)
		return false;

	hit = *intersections.begin();
	return true;
}

CString CExperimentFeatures::GetNodeLayerName(osg::Node* pNode) const
{
	if (!pNode)
		return _T("");

	std::string strName = pNode->getName();
	if (strName.empty())
		return _T("");

	std::string strGbk = UTF8ToGB2132(strName);
	CString strLayerName = strGbk.c_str();
	if (strLayerName.IsEmpty())
		strLayerName = strName.c_str();
	return strLayerName;
}

osg::Node* CExperimentFeatures::FindNamedLayerFromPath(const osg::NodePath& nodePath) const
{
	for (osg::NodePath::const_reverse_iterator it = nodePath.rbegin(); it != nodePath.rend(); ++it)
	{
		CString strName = GetNodeLayerName(*it);
		if (!strName.IsEmpty())
			return *it;
	}

	return NULL;
}

void CExperimentFeatures::ClearOverlay()
{
	if (m_pOverlayRoot.valid())
		m_pOverlayRoot->removeChildren(0, m_pOverlayRoot->getNumChildren());
}

void CExperimentFeatures::ShowBoundingBox(osg::Node* pNode)
{
	if (!pNode || !m_pOverlayRoot.valid())
		return;

	ClearOverlay();

	osg::ComputeBoundsVisitor cbv;
	pNode->accept(cbv);
	osg::BoundingBox bb = cbv.getBoundingBox();
	if (!bb.valid())
		return;

	osg::ref_ptr<osg::Vec3Array> pVertices = new osg::Vec3Array;
	pVertices->push_back(osg::Vec3(bb.xMin(), bb.yMin(), bb.zMin()));
	pVertices->push_back(osg::Vec3(bb.xMax(), bb.yMin(), bb.zMin()));
	pVertices->push_back(osg::Vec3(bb.xMax(), bb.yMax(), bb.zMin()));
	pVertices->push_back(osg::Vec3(bb.xMin(), bb.yMax(), bb.zMin()));
	pVertices->push_back(osg::Vec3(bb.xMin(), bb.yMin(), bb.zMax()));
	pVertices->push_back(osg::Vec3(bb.xMax(), bb.yMin(), bb.zMax()));
	pVertices->push_back(osg::Vec3(bb.xMax(), bb.yMax(), bb.zMax()));
	pVertices->push_back(osg::Vec3(bb.xMin(), bb.yMax(), bb.zMax()));

	static const unsigned int lines[] =
	{
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
	};

	osg::ref_ptr<osg::DrawElementsUInt> pIndices = new osg::DrawElementsUInt(GL_LINES);
	for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i)
		pIndices->push_back(lines[i]);

	osg::ref_ptr<osg::Geometry> pGeometry = new osg::Geometry;
	pGeometry->setVertexArray(pVertices.get());
	pGeometry->addPrimitiveSet(pIndices.get());

	osg::ref_ptr<osg::Vec4Array> pColors = new osg::Vec4Array;
	pColors->push_back(osg::Vec4(1.0f, 0.3f, 0.1f, 1.0f));
	pGeometry->setColorArray(pColors.get());
	pGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

	osg::ref_ptr<osg::Geode> pGeode = new osg::Geode;
	pGeode->addDrawable(pGeometry.get());

	osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
	pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	pStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
	osg::ref_ptr<osg::LineWidth> pLineWidth = new osg::LineWidth(2.0f);
	pStateSet->setAttributeAndModes(pLineWidth.get(), osg::StateAttribute::ON);

	m_pOverlayRoot->addChild(pGeode.get());
}

bool CExperimentFeatures::HandlePick(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&)
{
	osg::notify(osg::NOTICE) << "[ELEV-DIAG] HandlePick eventType=" << ea.getEventType() << " button=" << ea.getButton() << " terrainQuery=" << (m_bTerrainQueryEnabled ? "true" : "false") << " inspector=" << (m_bInspectorEnabled ? "true" : "false") << std::endl;
	BG_DIAG_LOG("[ELEV-DIAG] HandlePick eventType=" << ea.getEventType() << " button=" << ea.getButton() << " terrainQuery=" << (m_bTerrainQueryEnabled ? "true" : "false") << " inspector=" << (m_bInspectorEnabled ? "true" : "false"));
	if (ea.getEventType() != osgGA::GUIEventAdapter::PUSH || ea.getButton() != osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] HandlePick ignored: not left mouse push" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] HandlePick ignored: not left mouse push");
		return false;
	}

	if (!m_bInspectorEnabled && !m_bTerrainQueryEnabled)
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] HandlePick ignored: no pick mode enabled" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] HandlePick ignored: no pick mode enabled");
		return false;
	}

	osgViewer::Viewer* pViewer = EnsureReady() ? m_pScene->getViewer() : NULL;
	osg::Camera* pCamera = pViewer ? pViewer->getCamera() : NULL;
	if (pCamera && pCamera->getViewport())
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] camera viewport x=" << pCamera->getViewport()->x() << " y=" << pCamera->getViewport()->y() << " w=" << pCamera->getViewport()->width() << " h=" << pCamera->getViewport()->height() << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] camera viewport x=" << pCamera->getViewport()->x() << " y=" << pCamera->getViewport()->y() << " w=" << pCamera->getViewport()->width() << " h=" << pCamera->getViewport()->height());
	}
	else
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] camera or viewport is null" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] camera or viewport is null");
	}

	osgUtil::LineSegmentIntersector::Intersection hit;
	if (!PickFirstIntersection(ea, hit))
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] no intersection hit; user may have clicked empty space or non-pickable surface" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] no intersection hit; user may have clicked empty space or non-pickable surface");
		return false;
	}

	if (m_bInspectorEnabled)
	{
		osg::Node* pLayerNode = FindNamedLayerFromPath(hit.nodePath);
		if (!pLayerNode)
			return false;

		ShowBoundingBox(pLayerNode);

		osg::BoundingSphere bs = pLayerNode->getBound();
		CString strMsg;
		strMsg.Format(_T("Object Inspector\r\nLayer: %s\r\nCenter: (%.3f, %.3f, %.3f)\r\nRadius: %.3f"),
			GetNodeLayerName(pLayerNode), bs.center().x(), bs.center().y(), bs.center().z(), bs.radius());
		AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
		return true;
	}

	if (m_bTerrainQueryEnabled)
	{
		osg::Vec3d point = hit.getWorldIntersectPoint();
		osg::Node* pLayerNode = FindNamedLayerFromPath(hit.nodePath);
		CString strLayerName = GetNodeLayerName(pLayerNode);
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] terrain hit worldPoint=(" << point.x() << "," << point.y() << "," << point.z() << ") layerName=" << (LPCTSTR)strLayerName << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] terrain hit worldPoint=(" << point.x() << "," << point.y() << "," << point.z() << ") layerName=" << (LPCTSTR)strLayerName);
		if (!IsTerrainNodePath(hit.nodePath))
		{
			osg::notify(osg::NOTICE) << "[ELEV-DIAG] terrain recognition failed for layerName=" << (LPCTSTR)strLayerName << std::endl;
			BG_DIAG_LOG("[ELEV-DIAG] terrain recognition failed for layerName=" << (LPCTSTR)strLayerName);
			return false;
		}
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] terrain node marker found" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] terrain node marker found");

		CString strDisplayLayer = IsTerrainLayerName(strLayerName) ? strLayerName : _T("地形");
		CString strMsg;
		strMsg.Format(_T("Terrain Query\r\nLayer: %s\r\nXY: (%.3f, %.3f)\r\nScene Elevation: %.3f"),
			strDisplayLayer, point.x(), point.y(), point.z());
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] showing terrain query message box" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] showing terrain query message box");
		AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
		return true;
	}

	return false;
}
