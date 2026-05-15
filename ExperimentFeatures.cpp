#include "pch.h"
#include "ExperimentFeatures.h"
#include "resource.h"

#include <atlimage.h>
#include <osg/ComputeBoundsVisitor>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Group>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osg/PolygonOffset>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <float.h>
#include <math.h>
#include <errno.h>

namespace
{

	void ProfileDebug(LPCTSTR pszText)
	{
		OutputDebugString(pszText);
		OutputDebugString(_T("\r\n"));
	}
	class CFloodLevelDlg : public CDialog
	{
	public:
		CFloodLevelDlg(double dDefaultLevel, CWnd* pParent)
			: CDialog(IDD_FLOOD_LEVEL_DIALOG, pParent)
			, m_dWaterLevel(dDefaultLevel)
		{
		}

		double GetWaterLevel() const { return m_dWaterLevel; }

	protected:
		virtual BOOL OnInitDialog()
		{
			CDialog::OnInitDialog();
			CString strValue;
			strValue.Format(_T("%.3f"), m_dWaterLevel);
			SetDlgItemText(IDC_EDIT_WATER_LEVEL, strValue);
			CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_WATER_LEVEL);
			if (pEdit)
			{
				pEdit->SetSel(0, -1);
				pEdit->SetFocus();
				return FALSE;
			}
			return TRUE;
		}

		virtual void OnOK()
		{
			CString strValue;
			GetDlgItemText(IDC_EDIT_WATER_LEVEL, strValue);
			strValue.Trim();
			if (strValue.IsEmpty())
			{
				AfxMessageBox(_T("请输入水位场景高程值。"), MB_OK | MB_ICONWARNING);
				return;
			}

			errno = 0;
			TCHAR* pEnd = NULL;
			double dValue = _tcstod(strValue, &pEnd);
			while (pEnd && *pEnd != _T('\0') && _istspace(*pEnd))
				++pEnd;

			if (errno == ERANGE || pEnd == NULL || *pEnd != _T('\0') || !_finite(dValue))
			{
				AfxMessageBox(_T("水位高程必须是有效数字。"), MB_OK | MB_ICONWARNING);
				return;
			}

			m_dWaterLevel = dValue;
			CDialog::OnOK();
		}

	private:
		double m_dWaterLevel;
	};


	class CTerrainProfileResultDlg : public CDialog
	{
	public:
		CTerrainProfileResultDlg(const CString& strReport, const std::vector<osg::Vec3d>& samples, CWnd* pParent)
			: CDialog(IDD_TERRAIN_PROFILE_RESULT_DIALOG, pParent)
			, m_strReport(strReport)
			, m_samples(samples)
		{
		}

	protected:
		afx_msg void OnPaint()
		{
			CPaintDC dc(this);
			CRect rectClient;
			GetClientRect(&rectClient);
			dc.FillSolidRect(rectClient, RGB(250, 250, 250));

			CRect rectReport(rectClient.left + 14, rectClient.top + 16, rectClient.left + 220, rectClient.bottom - 34);
			CRect rectChart(rectReport.right + 18, rectClient.top + 30, rectClient.right - 16, rectClient.bottom - 42);

			CFont fontTitle;
			fontTitle.CreatePointFont(105, _T("MS Shell Dlg"));
			CFont* pOldFont = dc.SelectObject(&fontTitle);
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(RGB(40, 40, 40));
			dc.TextOut(rectChart.left, rectClient.top + 10, _T("地形剖面曲线"));
			dc.SelectObject(pOldFont);

			CFont fontText;
			fontText.CreatePointFont(90, _T("MS Shell Dlg"));
			pOldFont = dc.SelectObject(&fontText);
			dc.SetTextColor(RGB(55, 55, 55));
			dc.DrawText(m_strReport, rectReport, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
			DrawProfileChart(dc, rectChart);
			dc.SelectObject(pOldFont);
		}

		DECLARE_MESSAGE_MAP()

	private:
		void DrawProfileChart(CDC& dc, const CRect& rectChart)
		{
			if (rectChart.Width() < 120 || rectChart.Height() < 100)
				return;

			CRect rectPlot(rectChart.left + 48, rectChart.top + 20, rectChart.right - 14, rectChart.bottom - 42);
			if (rectPlot.Width() <= 10 || rectPlot.Height() <= 10)
				return;

			CPen penGrid(PS_SOLID, 1, RGB(225, 225, 225));
			CPen penAxis(PS_SOLID, 1, RGB(90, 90, 90));
			CPen penLine(PS_SOLID, 2, RGB(60, 135, 210));
			CPen* pOldPen = dc.SelectObject(&penGrid);
			dc.FillSolidRect(rectPlot, RGB(255, 255, 255));
			dc.Rectangle(rectPlot);

			for (int i = 1; i < 4; ++i)
			{
				int x = rectPlot.left + rectPlot.Width() * i / 4;
				dc.MoveTo(x, rectPlot.top);
				dc.LineTo(x, rectPlot.bottom);
				int y = rectPlot.top + rectPlot.Height() * i / 4;
				dc.MoveTo(rectPlot.left, y);
				dc.LineTo(rectPlot.right, y);
			}

			dc.SelectObject(&penAxis);
			dc.MoveTo(rectPlot.left, rectPlot.bottom);
			dc.LineTo(rectPlot.right, rectPlot.bottom);
			dc.MoveTo(rectPlot.left, rectPlot.bottom);
			dc.LineTo(rectPlot.left, rectPlot.top);

			dc.SetTextColor(RGB(70, 70, 70));
			dc.TextOut(rectPlot.left, rectChart.top, _T("场景高程"));
			dc.TextOut(rectPlot.right - 30, rectPlot.bottom + 20, _T("距离"));

			if (m_samples.size() < 2)
			{
				dc.DrawText(_T("剖面采样点不足"), rectPlot, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				dc.SelectObject(pOldPen);
				return;
			}

			std::vector<double> distances;
			distances.reserve(m_samples.size());
			distances.push_back(0.0);
			double minZ = m_samples[0].z();
			double maxZ = m_samples[0].z();
			for (size_t i = 1; i < m_samples.size(); ++i)
			{
				distances.push_back(distances[i - 1] + (m_samples[i] - m_samples[i - 1]).length());
				if (m_samples[i].z() < minZ) minZ = m_samples[i].z();
				if (m_samples[i].z() > maxZ) maxZ = m_samples[i].z();
			}

			double totalDist = distances.back();
			if (totalDist <= 1e-8)
				totalDist = 1.0;
			if (fabs(maxZ - minZ) < 1e-8)
			{
				maxZ += 0.5;
				minZ -= 0.5;
			}
			double zPad = (maxZ - minZ) * 0.08;
			maxZ += zPad;
			minZ -= zPad;

			CString strLabel;
			strLabel.Format(_T("%.2f"), maxZ);
			dc.TextOut(rectChart.left + 2, rectPlot.top - 6, strLabel);
			strLabel.Format(_T("%.2f"), minZ);
			dc.TextOut(rectChart.left + 2, rectPlot.bottom - 8, strLabel);
			strLabel.Format(_T("%.2f"), totalDist);
			dc.TextOut(rectPlot.right - 50, rectPlot.bottom + 4, strLabel);
			dc.TextOut(rectPlot.left - 4, rectPlot.bottom + 4, _T("0"));

			dc.SelectObject(&penLine);
			for (size_t i = 0; i < m_samples.size(); ++i)
			{
				double tx = distances[i] / totalDist;
				double ty = (m_samples[i].z() - minZ) / (maxZ - minZ);
				int x = rectPlot.left + (int)(tx * rectPlot.Width() + 0.5);
				int y = rectPlot.bottom - (int)(ty * rectPlot.Height() + 0.5);
				if (i == 0)
					dc.MoveTo(x, y);
				else
					dc.LineTo(x, y);
			}

			dc.SelectObject(pOldPen);
		}

		CString m_strReport;
		std::vector<osg::Vec3d> m_samples;
	};

	BEGIN_MESSAGE_MAP(CTerrainProfileResultDlg, CDialog)
		ON_WM_PAINT()
	END_MESSAGE_MAP()
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
	bool ContainsTerrainMarker(osg::Node* pNode)
	{
		if (!pNode)
			return false;

		std::string strName = pNode->getName();
		if (strName == kTerrainNodeMarker || strName == kTerrainLayerMarker)
			return true;

		osg::Group* pGroup = pNode->asGroup();
		if (!pGroup)
			return false;

		for (unsigned int i = 0; i < pGroup->getNumChildren(); ++i)
		{
			if (ContainsTerrainMarker(pGroup->getChild(i)))
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
	, m_bTerrainProfileEnabled(false)
	, m_bPendingAnalysisClear(false)
	, m_nProfileClickCount(0)
	, m_bWireframeEnabled(false)
{
}

CExperimentFeatures::~CExperimentFeatures()
{
	ClearOverlay();
}

bool CExperimentFeatures::Initialize(COsgScene* pScene, HWND hWndOwner)
{
	ProfileDebug(_T("[PROFILE] Initialize begin"));
	m_pScene = pScene;
	m_hWndOwner = hWndOwner;
	if (!EnsureReady())
	{
		ProfileDebug(_T("[PROFILE] Initialize scene not ready"));
		return false;
	}

	// Do not modify the OSG scene graph from the MFC menu/UI thread here.
	// Overlay roots are created lazily inside OSG event handling or guarded callers.
	if (!m_pPickHandler.valid())
	{
		m_pPickHandler = new CExperimentPickHandler(this);
		m_pScene->getViewer()->addEventHandler(m_pPickHandler.get());
	}

	ProfileDebug(_T("[PROFILE] Initialize end"));
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
		m_bTerrainProfileEnabled = false;
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
		m_bTerrainProfileEnabled = false;
		ClearOverlay();
		strMessage = _T("Terrain query enabled\r\nLeft-click a terrain layer to query elevation.");
	}
	else
	{
		strMessage = _T("Terrain query disabled.");
	}
	return true;
}


bool CExperimentFeatures::PickTerrainIntersection(const osgGA::GUIEventAdapter& ea, osgUtil::LineSegmentIntersector::Intersection& hit)
{
	if (!EnsureReady())
		return false;

	osgUtil::LineSegmentIntersector::Intersections intersections;
	if (!m_pScene->getViewer()->computeIntersections(ea.getX(), ea.getY(), intersections))
		return false;

	for (osgUtil::LineSegmentIntersector::Intersections::const_iterator it = intersections.begin(); it != intersections.end(); ++it)
	{
		if (IsTerrainNodePath(it->nodePath))
		{
			hit = *it;
			return true;
		}
	}

	return false;
}

void CExperimentFeatures::ClearTerrainProfileOverlay()
{
	ProfileDebug(_T("[PROFILE] ClearTerrainProfileOverlay begin"));
	if (m_pProfileOverlayRoot.valid())
		m_pProfileOverlayRoot->removeChildren(0, m_pProfileOverlayRoot->getNumChildren());
	m_nProfileClickCount = 0;
	ProfileDebug(_T("[PROFILE] ClearTerrainProfileOverlay end"));
}
void CExperimentFeatures::ClearFloodOverlay()
{
	ProfileDebug(_T("[PROFILE] ClearFloodOverlay begin"));
	if (m_pFloodOverlayRoot.valid())
		m_pFloodOverlayRoot->removeChildren(0, m_pFloodOverlayRoot->getNumChildren());
	ProfileDebug(_T("[PROFILE] ClearFloodOverlay end"));
}
bool CExperimentFeatures::ClearAnalysisOverlay(CString& strMessage)
{
	ProfileDebug(_T("[PROFILE] ClearAnalysisOverlay begin"));
	if (!EnsureReady())
	{
		strMessage = _T("Scene is not ready.");
		ProfileDebug(_T("[PROFILE] ClearAnalysisOverlay scene invalid"));
		return false;
	}

	// Menu handlers run on the MFC UI thread. Defer scene-graph removal to the
	// OSG event handler to avoid mutating children while the render thread traverses.
	m_bPendingAnalysisClear = true;
	m_bTerrainProfileEnabled = false;
	m_nProfileClickCount = 0;
	if (m_pScene->getViewer())
		m_pScene->getViewer()->requestRedraw();
	strMessage = _T("分析叠加结果已请求清除。");
	AfxMessageBox(strMessage, MB_OK | MB_ICONINFORMATION);
	ProfileDebug(_T("[PROFILE] ClearAnalysisOverlay end"));
	return true;
}
bool CExperimentFeatures::HasTerrainLayer() const
{
	if (!EnsureReady())
		return false;

	osg::Group* pRoot = m_pScene->getRoot();
	for (unsigned int i = 0; i < pRoot->getNumChildren(); ++i)
	{
		if (ContainsTerrainMarker(pRoot->getChild(i)))
			return true;
	}
	return false;
}

bool CExperimentFeatures::GetTerrainBoundingBox(osg::BoundingBox& bbox) const
{
	if (!EnsureReady())
		return false;

	bbox.init();
	osg::Group* pRoot = m_pScene->getRoot();
	for (unsigned int i = 0; i < pRoot->getNumChildren(); ++i)
	{
		osg::Node* pNode = pRoot->getChild(i);
		if (!ContainsTerrainMarker(pNode))
			continue;

		osg::ComputeBoundsVisitor cbv;
		pNode->accept(cbv);
		osg::BoundingBox nodeBox = cbv.getBoundingBox();
		if (!nodeBox.valid())
			continue;

		bbox.expandBy(nodeBox);
	}

	return bbox.valid();
}

void CExperimentFeatures::AddProfileMarker(const osg::Vec3d& pos, const std::string& name)
{
	ProfileDebug(_T("[PROFILE] AddProfileMarker begin"));
	if (!EnsureAnalysisOverlayRoot() || !m_pProfileOverlayRoot.valid())
	{
		ProfileDebug(_T("[PROFILE] AddProfileMarker overlay invalid"));
		return;
	}

	osg::ref_ptr<osg::Vec3Array> pVertices = new osg::Vec3Array;
	pVertices->push_back(osg::Vec3(pos.x(), pos.y(), pos.z()));
	if (pVertices->size() == 0)
		return;

	osg::ref_ptr<osg::Geometry> pGeometry = new osg::Geometry;
	pGeometry->setVertexArray(pVertices.get());
	pGeometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, (GLsizei)pVertices->size()));

	osg::ref_ptr<osg::Vec4Array> pColors = new osg::Vec4Array;
	if (name == "ProfileStart")
		pColors->push_back(osg::Vec4(0.10f, 0.85f, 0.30f, 0.95f));
	else
		pColors->push_back(osg::Vec4(1.0f, 0.55f, 0.12f, 0.95f));
	pGeometry->setColorArray(pColors.get());
	pGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

	osg::ref_ptr<osg::Geode> pGeode = new osg::Geode;
	pGeode->setName(name);
	pGeode->addDrawable(pGeometry.get());

	osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
	if (pStateSet)
	{
		pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		pStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
		pStateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
		osg::ref_ptr<osg::Point> pPoint = new osg::Point(9.0f);
		pStateSet->setAttributeAndModes(pPoint.get(), osg::StateAttribute::ON);
	}

	if (pGeode.valid())
		m_pProfileOverlayRoot->addChild(pGeode.get());
	ProfileDebug(_T("[PROFILE] AddProfileMarker end"));
}
void CExperimentFeatures::DrawTerrainProfileLine(const osg::Vec3d& start, const osg::Vec3d& end)
{
	ProfileDebug(_T("[PROFILE] DrawTerrainProfileLine begin"));
	if (!EnsureAnalysisOverlayRoot() || !m_pProfileOverlayRoot.valid())
	{
		ProfileDebug(_T("[PROFILE] DrawTerrainProfileLine overlay invalid"));
		return;
	}

	osg::BoundingBox terrainBBox;
	double dOffset = 0.05;
	if (GetTerrainBoundingBox(terrainBBox))
	{
		double dx = terrainBBox.xMax() - terrainBBox.xMin();
		double dy = terrainBBox.yMax() - terrainBBox.yMin();
		double dz = terrainBBox.zMax() - terrainBBox.zMin();
		dOffset = sqrt(dx * dx + dy * dy + dz * dz) * 0.0015;
		if (dOffset < 0.03)
			dOffset = 0.03;
		if (dOffset > 2.0)
			dOffset = 2.0;
	}

	osg::ref_ptr<osg::Vec3Array> pVertices = new osg::Vec3Array;
	pVertices->push_back(osg::Vec3(start.x(), start.y(), start.z() + dOffset));
	pVertices->push_back(osg::Vec3(end.x(), end.y(), end.z() + dOffset));
	if (pVertices->size() < 2)
		return;

	osg::ref_ptr<osg::Geometry> pGeometry = new osg::Geometry;
	pGeometry->setVertexArray(pVertices.get());
	pGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, (GLsizei)pVertices->size()));

	osg::ref_ptr<osg::Vec4Array> pColors = new osg::Vec4Array;
	pColors->push_back(osg::Vec4(1.0f, 0.78f, 0.18f, 0.95f));
	pGeometry->setColorArray(pColors.get());
	pGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

	osg::ref_ptr<osg::Geode> pGeode = new osg::Geode;
	pGeode->setName("TerrainProfileLine");
	pGeode->addDrawable(pGeometry.get());

	osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
	if (pStateSet)
	{
		pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		pStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
		pStateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
		osg::ref_ptr<osg::LineWidth> pLineWidth = new osg::LineWidth(3.0f);
		pStateSet->setAttributeAndModes(pLineWidth.get(), osg::StateAttribute::ON);
	}

	if (pGeode.valid())
		m_pProfileOverlayRoot->addChild(pGeode.get());
	ProfileDebug(_T("[PROFILE] DrawTerrainProfileLine end"));
}
std::vector<osg::Vec3d> CExperimentFeatures::SampleProfilePoints(const osg::Vec3d& start, const osg::Vec3d& end, int sampleCount) const
{
	ProfileDebug(_T("[PROFILE] SampleProfilePoints begin"));
	std::vector<osg::Vec3d> samples;
	if (sampleCount < 2)
		sampleCount = 2;

	samples.reserve(sampleCount);
	osg::BoundingBox terrainBBox;
	bool bUseTerrainRay = GetTerrainBoundingBox(terrainBBox) && EnsureReady();
	double zHigh = start.z();
	double zLow = end.z();
	if (zLow > zHigh)
	{
		double tmp = zLow;
		zLow = zHigh;
		zHigh = tmp;
	}

	if (bUseTerrainRay)
	{
		double dx = terrainBBox.xMax() - terrainBBox.xMin();
		double dy = terrainBBox.yMax() - terrainBBox.yMin();
		double dz = terrainBBox.zMax() - terrainBBox.zMin();
		double padding = sqrt(dx * dx + dy * dy + dz * dz) * 0.25;
		if (padding < 10.0)
			padding = 10.0;
		zHigh = terrainBBox.zMax() + padding;
		zLow = terrainBBox.zMin() - padding;
	}

	for (int i = 0; i < sampleCount; ++i)
	{
		double t = (double)i / (double)(sampleCount - 1);
		osg::Vec3d linearPoint = start * (1.0 - t) + end * t;
		osg::Vec3d samplePoint = linearPoint;

		if (bUseTerrainRay)
		{
			osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(
				osgUtil::Intersector::MODEL,
				osg::Vec3d(linearPoint.x(), linearPoint.y(), zHigh),
				osg::Vec3d(linearPoint.x(), linearPoint.y(), zLow));
			osgUtil::IntersectionVisitor visitor(intersector.get());
			m_pScene->getRoot()->accept(visitor);

			if (intersector->containsIntersections())
			{
				const osgUtil::LineSegmentIntersector::Intersections& hits = intersector->getIntersections();
				for (osgUtil::LineSegmentIntersector::Intersections::const_iterator it = hits.begin(); it != hits.end(); ++it)
				{
					if (IsTerrainNodePath(it->nodePath))
					{
						samplePoint = it->getWorldIntersectPoint();
						break;
					}
				}
			}
		}

		samples.push_back(samplePoint);
	}
	return samples;
}
CString CExperimentFeatures::BuildTerrainProfileReport(const std::vector<osg::Vec3d>& samples)
{
	if (samples.size() < 2)
		return _T("地形剖面分析结果\r\n\r\n剖面采样点不足。\r\n说明: 当前结果使用场景坐标系下的场景高程。");

	const osg::Vec3d& start = samples.front();
	const osg::Vec3d& end = samples.back();
	double dMinZ = DBL_MAX;
	double dMaxZ = -DBL_MAX;
	double dSumZ = 0.0;
	double dProfileLength = 0.0;

	for (size_t i = 0; i < samples.size(); ++i)
	{
		double z = samples[i].z();
		if (z < dMinZ) dMinZ = z;
		if (z > dMaxZ) dMaxZ = z;
		dSumZ += z;
		if (i > 0)
			dProfileLength += (samples[i] - samples[i - 1]).length();
	}

	double dAvgZ = dSumZ / (double)samples.size();
	double dDx = end.x() - start.x();
	double dDy = end.y() - start.y();
	double dDz = end.z() - start.z();
	double dHorizontalLength = sqrt(dDx * dDx + dDy * dDy);
	double dSlopePercent = dHorizontalLength > 1e-8 ? fabs(dDz) / dHorizontalLength * 100.0 : 0.0;

	CString strReport;
	strReport.Format(_T("地形剖面分析结果\r\n\r\n起点场景高程: %.3f\r\n终点场景高程: %.3f\r\n最大场景高程: %.3f\r\n最小场景高程: %.3f\r\n平均场景高程: %.3f\r\n剖面线长度: %.3f\r\n水平距离: %.3f\r\n高差: %.3f\r\n平均坡度: %.3f%%\r\n\r\n说明: 当前统计基于场景坐标系下的剖面采样点。"),
		start.z(), end.z(), dMaxZ, dMinZ, dAvgZ, dProfileLength, dHorizontalLength, dDz, dSlopePercent);
	return strReport;
}
bool CExperimentFeatures::EnterTerrainProfileMode(CString& strMessage)
{
	ProfileDebug(_T("[PROFILE] EnterTerrainProfileMode begin"));
	if (!EnsureReady())
	{
		strMessage = _T("Scene is not ready.");
		ProfileDebug(_T("[PROFILE] EnterTerrainProfileMode scene invalid"));
		return false;
	}

	if (!HasTerrainLayer())
	{
		strMessage = _T("请先加载 DEM 地形，再进行地形剖面分析。");
		AfxMessageBox(strMessage, MB_OK | MB_ICONWARNING);
		ProfileDebug(_T("[PROFILE] EnterTerrainProfileMode no terrain"));
		return false;
	}

	// Entering profile mode must not touch the OSG scene graph. The render
	// thread may be traversing it while this menu command runs on the UI thread.
	m_bTerrainProfileEnabled = true;
	m_bInspectorEnabled = false;
	m_bTerrainQueryEnabled = false;
	m_nProfileClickCount = 0;
	m_profileStart = osg::Vec3d();
	m_profileEnd = osg::Vec3d();
	strMessage = _T("已进入地形剖面分析模式，请在地形上点击剖面起点。");
	AfxMessageBox(strMessage, MB_OK | MB_ICONINFORMATION);
	ProfileDebug(_T("[PROFILE] EnterTerrainProfileMode end"));
	return true;
}
void CExperimentFeatures::CreateWaterSurface(double waterLevel, const osg::BoundingBox& terrainBBox)
{
	ProfileDebug(_T("[PROFILE] CreateWaterSurface begin"));
	if (!EnsureAnalysisOverlayRoot() || !m_pFloodOverlayRoot.valid())
	{
		ProfileDebug(_T("[PROFILE] CreateWaterSurface overlay invalid"));
		return;
	}

	ClearFloodOverlay();
	float z = (float)(waterLevel + 0.02);
	osg::ref_ptr<osg::Vec3Array> pVertices = new osg::Vec3Array;
	pVertices->push_back(osg::Vec3(terrainBBox.xMin(), terrainBBox.yMin(), z));
	pVertices->push_back(osg::Vec3(terrainBBox.xMax(), terrainBBox.yMin(), z));
	pVertices->push_back(osg::Vec3(terrainBBox.xMax(), terrainBBox.yMax(), z));
	pVertices->push_back(osg::Vec3(terrainBBox.xMin(), terrainBBox.yMax(), z));
	if (pVertices->size() < 4)
		return;

	osg::ref_ptr<osg::Geometry> pGeometry = new osg::Geometry;
	pGeometry->setVertexArray(pVertices.get());
	pGeometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, (GLsizei)pVertices->size()));

	osg::ref_ptr<osg::Vec4Array> pColors = new osg::Vec4Array;
	pColors->push_back(osg::Vec4(0.05f, 0.55f, 0.95f, 0.42f));
	pGeometry->setColorArray(pColors.get());
	pGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

	osg::ref_ptr<osg::Geode> pGeode = new osg::Geode;
	pGeode->setName("FloodWaterSurface");
	pGeode->addDrawable(pGeometry.get());

	osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
	if (pStateSet)
	{
		pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		pStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
		pStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
		pStateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
		osg::ref_ptr<osg::Depth> pDepth = new osg::Depth;
		pDepth->setWriteMask(false);
		pStateSet->setAttributeAndModes(pDepth.get(), osg::StateAttribute::ON);
		osg::ref_ptr<osg::PolygonOffset> pOffset = new osg::PolygonOffset(-1.0f, -1.0f);
		pStateSet->setAttributeAndModes(pOffset.get(), osg::StateAttribute::ON);
	}

	if (pGeode.valid())
		m_pFloodOverlayRoot->addChild(pGeode.get());
	ProfileDebug(_T("[PROFILE] CreateWaterSurface end"));
}
bool CExperimentFeatures::RunFloodSimulation(CString& strMessage)
{
	if (!EnsureReady())
	{
		strMessage = _T("Scene is not ready.");
		return false;
	}

	osg::BoundingBox terrainBBox;
	if (!GetTerrainBoundingBox(terrainBBox))
	{
		strMessage = _T("请先加载 DEM 地形，再进行水位淹没模拟。");
		AfxMessageBox(strMessage, MB_OK | MB_ICONWARNING);
		return false;
	}

	double dDefaultLevel = (terrainBBox.zMin() + terrainBBox.zMax()) * 0.5;
	CFloodLevelDlg dlg(dDefaultLevel, CWnd::FromHandle(m_hWndOwner));
	if (dlg.DoModal() != IDOK)
	{
		strMessage = _T("水位淹没模拟已取消。");
		return false;
	}

	double dWaterLevel = dlg.GetWaterLevel();
	CreateWaterSurface(dWaterLevel, terrainBBox);
	if (m_pScene->getViewer())
		m_pScene->getViewer()->requestRedraw();

	strMessage.Format(_T("水位淹没模拟已生成。\r\n当前水位场景高程：%.3f"), dWaterLevel);
	CString strBox;
	strBox.Format(_T("水位淹没模拟已生成。\r\n当前水位场景高程：%.3f\r\n\r\n说明：本功能基于 DEM 场景高程生成半透明水面，用于三维淹没可视化表达。"), dWaterLevel);
	AfxMessageBox(strBox, MB_OK | MB_ICONINFORMATION);
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


bool CExperimentFeatures::EnsureOverlayRoot()
{
	ProfileDebug(_T("[PROFILE] EnsureOverlayRoot begin"));
	if (!EnsureReady())
	{
		ProfileDebug(_T("[PROFILE] EnsureOverlayRoot scene invalid"));
		return false;
	}

	if (!m_pOverlayRoot.valid())
	{
		m_pOverlayRoot = new osg::Group;
		if (!m_pOverlayRoot.valid())
			return false;
		m_pOverlayRoot->setName("ExperimentOverlay");
		m_pScene->getRoot()->addChild(m_pOverlayRoot.get());
	}

	ProfileDebug(_T("[PROFILE] EnsureOverlayRoot end"));
	return m_pOverlayRoot.valid();
}

bool CExperimentFeatures::EnsureAnalysisOverlayRoot()
{
	ProfileDebug(_T("[PROFILE] EnsureAnalysisOverlayRoot begin"));
	if (!EnsureReady())
	{
		ProfileDebug(_T("[PROFILE] EnsureAnalysisOverlayRoot scene invalid"));
		return false;
	}

	osg::Group* pRoot = m_pScene->getRoot();
	if (!pRoot)
	{
		ProfileDebug(_T("[PROFILE] EnsureAnalysisOverlayRoot root null"));
		return false;
	}

	if (!m_pAnalysisOverlayRoot.valid())
	{
		m_pAnalysisOverlayRoot = new osg::Group;
		if (!m_pAnalysisOverlayRoot.valid())
			return false;
		m_pAnalysisOverlayRoot->setName("AnalysisOverlay");
		pRoot->addChild(m_pAnalysisOverlayRoot.get());
	}

	if (!m_pProfileOverlayRoot.valid())
	{
		m_pProfileOverlayRoot = new osg::Group;
		if (!m_pProfileOverlayRoot.valid())
			return false;
		m_pProfileOverlayRoot->setName("TerrainProfileOverlay");
		m_pAnalysisOverlayRoot->addChild(m_pProfileOverlayRoot.get());
	}

	if (!m_pFloodOverlayRoot.valid())
	{
		m_pFloodOverlayRoot = new osg::Group;
		if (!m_pFloodOverlayRoot.valid())
			return false;
		m_pFloodOverlayRoot->setName("FloodSimulationOverlay");
		m_pAnalysisOverlayRoot->addChild(m_pFloodOverlayRoot.get());
	}

	ProfileDebug(_T("[PROFILE] EnsureAnalysisOverlayRoot end"));
	return m_pAnalysisOverlayRoot.valid();
}

void CExperimentFeatures::ProcessPendingAnalysisClear()
{
	if (!m_bPendingAnalysisClear)
		return;

	ProfileDebug(_T("[PROFILE] ProcessPendingAnalysisClear begin"));
	m_bPendingAnalysisClear = false;
	ClearTerrainProfileOverlay();
	ClearFloodOverlay();
	ProfileDebug(_T("[PROFILE] ProcessPendingAnalysisClear end"));
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
	if (!pNode || !EnsureOverlayRoot() || !m_pOverlayRoot.valid())
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
	ProcessPendingAnalysisClear();
	osg::notify(osg::NOTICE) << "[ELEV-DIAG] HandlePick eventType=" << ea.getEventType() << " button=" << ea.getButton() << " terrainQuery=" << (m_bTerrainQueryEnabled ? "true" : "false") << " inspector=" << (m_bInspectorEnabled ? "true" : "false") << std::endl;
	BG_DIAG_LOG("[ELEV-DIAG] HandlePick eventType=" << ea.getEventType() << " button=" << ea.getButton() << " terrainQuery=" << (m_bTerrainQueryEnabled ? "true" : "false") << " inspector=" << (m_bInspectorEnabled ? "true" : "false"));
	if (ea.getEventType() != osgGA::GUIEventAdapter::PUSH || ea.getButton() != osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
	{
		osg::notify(osg::NOTICE) << "[ELEV-DIAG] HandlePick ignored: not left mouse push" << std::endl;
		BG_DIAG_LOG("[ELEV-DIAG] HandlePick ignored: not left mouse push");
		return false;
	}

	if (!m_bInspectorEnabled && !m_bTerrainQueryEnabled && !m_bTerrainProfileEnabled)
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

	if (m_bTerrainProfileEnabled)
	{
		osgUtil::LineSegmentIntersector::Intersection terrainHit;
		if (!PickTerrainIntersection(ea, terrainHit))
		{
			AfxMessageBox(_T("未命中地形，请重新点击 DEM 地形表面。"), MB_OK | MB_ICONWARNING);
			return true;
		}

		osg::Vec3d point = terrainHit.getWorldIntersectPoint();
		if (m_nProfileClickCount == 0)
		{
			m_profileStart = point;
			m_nProfileClickCount = 1;
			AddProfileMarker(m_profileStart, "ProfileStart");
			if (m_pScene->getViewer())
				m_pScene->getViewer()->requestRedraw();
			AfxMessageBox(_T("已记录起点，请点击剖面终点。"), MB_OK | MB_ICONINFORMATION);
			return true;
		}

		m_profileEnd = point;
		AddProfileMarker(m_profileEnd, "ProfileEnd");
		DrawTerrainProfileLine(m_profileStart, m_profileEnd);
		m_bTerrainProfileEnabled = false;
		m_nProfileClickCount = 0;
		if (m_pScene->getViewer())
			m_pScene->getViewer()->requestRedraw();

		std::vector<osg::Vec3d> profileSamples = SampleProfilePoints(m_profileStart, m_profileEnd, 120);
		CString strReport = BuildTerrainProfileReport(profileSamples);
		CTerrainProfileResultDlg dlg(strReport, profileSamples, CWnd::FromHandle(m_hWndOwner));
		dlg.DoModal();
		return true;
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
