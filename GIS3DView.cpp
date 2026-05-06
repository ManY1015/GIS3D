
// GIS3DView.cpp: CGIS3DView ���ʵ��?
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS ������ʵ��Ԥ��������ͼ������ɸѡ�������?
// ATL ��Ŀ�н��ж��壬�����������Ŀ�����ĵ�����?
#ifndef SHARED_HANDLERS
#include "GIS3D.h"
#endif

#include "GIS3DDoc.h"
#include "GIS3DView.h"
#include "ExperimentFeatures.h"

#include "SceneManageHandler.h"
#include "resource.h"
#include "SceneObject.h"
#include "MainFrm.h"
#include <osgGA/CameraManipulator>
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IDC_VIEW_HANDLE_UP       5101
#define IDC_VIEW_HANDLE_DOWN     5102
#define IDC_VIEW_HANDLE_LEFT     5103
#define IDC_VIEW_HANDLE_RIGHT    5104
#define IDC_VIEW_HANDLE_TOP      5105
#define IDC_VIEW_HANDLE_HOME     5106
#define CAMERA_HANDLE_SIZE       34
#define CAMERA_HANDLE_MARGIN     18


// CGIS3DView

IMPLEMENT_DYNCREATE(CGIS3DView, CView)

BEGIN_MESSAGE_MAP(CGIS3DView, CView)
	// ��׼��ӡ����
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CGIS3DView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_LOAD_TERRAIN, &CGIS3DView::OnLoadTerrain)
	ON_COMMAND(ID_LOAD_MODEL, &CGIS3DView::OnLoadModel)
	ON_COMMAND(ID_LOAD_BATCH_MODELS, &CGIS3DView::OnLoadBatchModels)
	ON_COMMAND(ID_EXP_BOOKMARK_SAVE, &CGIS3DView::OnExpSaveBookmark)
	ON_COMMAND(ID_EXP_BOOKMARK_RESTORE, &CGIS3DView::OnExpRestoreBookmark)
	ON_COMMAND(ID_EXP_BATCH_OVERVIEW, &CGIS3DView::OnExpBatchOverview)
	ON_COMMAND(ID_EXP_SCREENSHOT, &CGIS3DView::OnExpScreenshot)
	ON_COMMAND(ID_EXP_OBJECT_INSPECT, &CGIS3DView::OnExpObjectInspect)
	ON_COMMAND(ID_EXP_TERRAIN_QUERY, &CGIS3DView::OnExpTerrainQuery)
	ON_COMMAND(ID_EXP_WIREFRAME, &CGIS3DView::OnExpWireframe)
	ON_BN_CLICKED(IDC_VIEW_HANDLE_UP, &CGIS3DView::OnViewHandleUp)
	ON_BN_CLICKED(IDC_VIEW_HANDLE_DOWN, &CGIS3DView::OnViewHandleDown)
	ON_BN_CLICKED(IDC_VIEW_HANDLE_LEFT, &CGIS3DView::OnViewHandleLeft)
	ON_BN_CLICKED(IDC_VIEW_HANDLE_RIGHT, &CGIS3DView::OnViewHandleRight)
	ON_BN_CLICKED(IDC_VIEW_HANDLE_TOP, &CGIS3DView::OnViewHandleTop)
	ON_BN_CLICKED(IDC_VIEW_HANDLE_HOME, &CGIS3DView::OnViewHandleHome)
END_MESSAGE_MAP()

// CGIS3DView ����/����

CGIS3DView::CGIS3DView() noexcept
{
	// TODO: �ڴ˴���ӹ������

	m_bInit = true;
	m_OSG = NULL;
	m_bCameraHandleCreated = false;
	m_bTerrainStatusCreated = false;
    m_pExperimentFeatures = NULL;
	ResetTerrainWorkflow(true);

}

CGIS3DView::~CGIS3DView()
{
}

BOOL CGIS3DView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ���������?

	return CView::PreCreateWindow(cs);
}

// CGIS3DView ��ͼ

void CGIS3DView::OnDraw(CDC* /*pDC*/)
{
	CGIS3DDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: �ڴ˴�Ϊ����������ӻ��ƴ���?
}


// CGIS3DView ��ӡ


void CGIS3DView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CGIS3DView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// Ĭ��׼��
	return DoPreparePrinting(pInfo);
}

void CGIS3DView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: ��Ӷ���Ĵ�ӡǰ���еĳ�ʼ������
}

void CGIS3DView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: ��Ӵ�ӡ����е��������?
}

void CGIS3DView::OnRButtonUp(UINT /* nFlags */, CPoint /* point */)
{
	// �Ҽ���ק���� OSG ԭ����������������ⵯ���˵����ƽ�Ʋ�����
}

void CGIS3DView::OnContextMenu(CWnd* /* pWnd */, CPoint /* point */)
{
	// ������ͼ���Ҽ��������ƽ�ƣ��������������Ϣӳ��仯��?
}


// CGIS3DView ���?

#ifdef _DEBUG
void CGIS3DView::AssertValid() const
{
	CView::AssertValid();
}

void CGIS3DView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGIS3DDoc* CGIS3DView::GetDocument() const // �ǵ��԰汾��������
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGIS3DDoc)));
	return (CGIS3DDoc*)m_pDocument;
}
#endif //_DEBUG


// CGIS3DView ��Ϣ�������?


void CGIS3DView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	TRACE(_T("[BG-DIAG] OnInitialUpdate entered\n"));

	// TODO: �ڴ����ר�ô����/����û���?
    TRACE(_T("[BG-DIAG] InitOSG before m_OSG=%p\n"), m_OSG);
    m_OSG->InitOSG("");
    TRACE(_T("[BG-DIAG] InitOSG after viewer=%p root=%p\n"), m_OSG ? m_OSG->getViewer() : NULL, m_OSG ? m_OSG->getRoot() : NULL);

    mThreadHandle = (HANDLE)_beginthread(&COsgScene::Render, 0, m_OSG);
	SetThreadPriority(mThreadHandle, THREAD_PRIORITY_NORMAL);
}


bool CGIS3DView::EnsureExperimentFeatures()
{
	if (m_pExperimentFeatures == NULL)
		 m_pExperimentFeatures = new CExperimentFeatures;

	return m_pExperimentFeatures->Initialize(m_OSG, m_hWnd);
}

void CGIS3DView::OnDestroy()
{
	CView::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������?
    if (m_pExperimentFeatures != NULL)
    {
        delete m_pExperimentFeatures;
        m_pExperimentFeatures = NULL;
    }

	if (m_OSG != 0)
	{
		delete m_OSG;
		m_OSG = NULL;
	m_bCameraHandleCreated = false;
	m_bTerrainStatusCreated = false;
	ResetTerrainWorkflow(true);
	}


	WaitForSingleObject(mThreadHandle, 1000);
}


int CGIS3DView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  �ڴ������ר�õĴ�������?
	m_OSG = new COsgScene(m_hWnd);
	TRACE(_T("[BG-DIAG] OnCreate m_OSG created=%d hwnd=%p\n"), m_OSG != NULL, m_hWnd);
	// Deprecated terrain workflow UI is no longer created to avoid interfering with the original terrain import path.

	return 0;
}


void CGIS3DView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	TRACE(_T("[BG-DIAG] OnSize type=%u cx=%d cy=%d\n"), nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������?
	if (!m_bInit)
	{
		if (m_OSG)
		{
			osgViewer::Viewer* pView = m_OSG->getViewer();
			if (pView)
				pView->requestRedraw();
		}
	}
	if (m_bInit)
        m_bInit = false;

    LayoutCameraHandle();
    // Deprecated terrain workflow UI is no longer laid out.
    if (m_OSG && m_OSG->getViewer() && m_OSG->getViewer()->getCamera() && cx > 0 && cy > 0)
    {
        osg::Camera* pCamera = m_OSG->getViewer()->getCamera();
        pCamera->setViewport(new osg::Viewport(0, 0, cx, cy));
        pCamera->setProjectionMatrixAsPerspective(45.f, (double)cx / (double)cy, 0.1, 100000.0);
        TRACE(_T("[BG-DIAG] OnSize projection reset fovy=45 aspect=%0.6f near=0.1 far=100000\n"), (double)cx / (double)cy);
    }
}


BOOL CGIS3DView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ���?
	return true;
	//return CView::OnEraseBkgnd(pDC);
}


void CGIS3DView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE)
	{
		GetParent()->SendMessage(WM_CLOSE);
		return;
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}



void CGIS3DView::StopRenderThread()
{
	// 	if(mThreadHandle->isRunning())
	// 		mThreadHandle->detach();
	return;

	//	return;
	if (mThreadHandle)
	{
		DWORD suspendcount = SuspendThread(mThreadHandle);
		if (suspendcount == -1)
			return;//"�����߳�ʧ��"

	}

	
}
void CGIS3DView::ResumeRenderThread()
{

	if (mThreadHandle)
	{
		int iSuspendCount = ResumeThread(mThreadHandle);
		if (iSuspendCount == -1)
			;//ProcessErrorMessage("������ڼ���߳�");; 

		else
		{
			for (DWORD i = 0; i < static_cast<DWORD>(iSuspendCount); i++)//��������Σ���Ҫ����������Ĵ�������ֻ֤����һ�Σ�������������ʱ�̲߳��������?
			{
				ResumeThread(mThreadHandle);
			}
		}


	}

}

void CGIS3DView::AddEvent(SceneObjectInfo info)
{
	m_OSG->getViewer()->getEventQueue()->userEvent(new SceneManageInfo(info));//2014-3-12
}



void CGIS3DView::CreateTerrainStatusWindow()
{
	if (m_bTerrainStatusCreated)
		return;

	CRect rectDummy(0, 0, 360, 108);
	m_wndTerrainStatus.Create(_T("Terrain workflow ready\r\nL: Load  G: Build mesh  N: Normals\r\nT: Texcoord  S: Show  R: Reload"),
		WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, rectDummy, this);
	m_bTerrainStatusCreated = true;
	LayoutTerrainStatusWindow();
}

void CGIS3DView::LayoutTerrainStatusWindow()
{
	if (!m_bTerrainStatusCreated || !::IsWindow(m_wndTerrainStatus.GetSafeHwnd()))
		return;

	CRect rectClient;
	GetClientRect(rectClient);
	int width = 380;
	int height = 108;
	int left = rectClient.left + 12;
	int top = rectClient.bottom - height - 12;
	if (top < rectClient.top + 12)
		top = rectClient.top + 12;
	m_wndTerrainStatus.MoveWindow(left, top, width, height);
	m_wndTerrainStatus.BringWindowToTop();
}

void CGIS3DView::UpdateTerrainStatus(const CString& strMessage)
{
	if (m_bTerrainStatusCreated && ::IsWindow(m_wndTerrainStatus.GetSafeHwnd()))
	{
		m_wndTerrainStatus.SetWindowText(strMessage);
		m_wndTerrainStatus.Invalidate();
	}
}

void CGIS3DView::ResetTerrainWorkflow(bool bClearFiles)
{
	if (bClearFiles)
	{
		m_strWorkflowDemPath.Empty();
		m_strWorkflowImagePath.Empty();
		m_bTerrainDataSelected = false;
	}

	m_strWorkflowLayerName = _T("����");
	memset(&m_pendingTerrainInfo, 0x00, sizeof(SceneObjectInfo));
	m_bTerrainMeshBuilt = false;
	m_bTerrainNormalsReady = false;
	m_bTerrainTexcoordsReady = false;
	m_bTerrainAdded = false;

	UpdateTerrainStatus(_T("Terrain workflow reset\r\nL: Load DEM/Image  G: Build mesh\r\nN: Normals  T: Texcoord  S: Show"));
}

bool CGIS3DView::SelectTerrainWorkflowFiles()
{
	CString strDemFilter = _T("DEM�ļ�(*.tif;*.tiff;*.img;*.png;*.jpg;*.bmp)|*.tif;*.tiff;*.img;*.png;*.jpg;*.bmp|�����ļ�(*.*)|*.*||");
	CFileDialog dlgDem(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, strDemFilter, NULL);
	dlgDem.m_ofn.lpstrTitle = _T("ѡ��DEM�ļ�");
	if (dlgDem.DoModal() != IDOK)
	{
		UpdateTerrainStatus(_T("DEM load canceled"));
		return false;
	}

	CString strImgFilter = _T("Ӱ���ļ�(*.tif;*.tiff;*.jpg;*.jpeg;*.png;*.bmp)|*.tif;*.tiff;*.jpg;*.jpeg;*.png;*.bmp|�����ļ�(*.*)|*.*||");
	CFileDialog dlgImg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, strImgFilter, NULL);
	dlgImg.m_ofn.lpstrTitle = _T("ѡ��Ӱ���ļ�����ȡ����");

	m_strWorkflowDemPath = dlgDem.GetPathName();
	m_strWorkflowImagePath.Empty();
	if (dlgImg.DoModal() == IDOK)
		m_strWorkflowImagePath = dlgImg.GetPathName();

	ResetTerrainWorkflow(false);
	m_bTerrainDataSelected = true;

	CString strMsg;
	strMsg.Format(_T("Data loaded\r\nDEM: %s\r\nImage: %s"),
		m_strWorkflowDemPath.IsEmpty() ? _T("None") : m_strWorkflowDemPath,
		m_strWorkflowImagePath.IsEmpty() ? _T("None") : m_strWorkflowImagePath);
	UpdateTerrainStatus(strMsg);
	return true;
}

bool CGIS3DView::BuildTerrainWorkflowModel()
{
	if (!m_bTerrainDataSelected && !SelectTerrainWorkflowFiles())
		return false;

	CWaitCursor wait;
	memset(&m_pendingTerrainInfo, 0x00, sizeof(SceneObjectInfo));
	CSceneObject obj;
	m_pendingTerrainInfo.pObject = obj.CreateTerrain(m_strWorkflowDemPath, m_strWorkflowImagePath, &m_pendingTerrainInfo);
	if (!m_pendingTerrainInfo.pObject)
	{
		UpdateTerrainStatus(_T("Terrain build failed\r\nCheck DEM/Image format and file path."));
		return false;
	}

	lstrcpy(m_pendingTerrainInfo.szLayerName, m_strWorkflowLayerName);
	m_bTerrainMeshBuilt = true;
	m_bTerrainNormalsReady = true;
	m_bTerrainTexcoordsReady = true;
	m_bTerrainAdded = false;

	CString strMsg;
	if (m_pendingTerrainInfo.bHasGeoReference)
	{
		strMsg.Format(_T("Mesh built / Normals ready / Texcoords ready\r\nExtent: %.3f, %.3f - %.3f, %.3f\r\nRes: %.6f x %.6f  Layer: %s"),
			m_pendingTerrainInfo.dGeoMinX, m_pendingTerrainInfo.dGeoMinY,
			m_pendingTerrainInfo.dGeoMaxX, m_pendingTerrainInfo.dGeoMaxY,
			m_pendingTerrainInfo.adfGeoTransform[1], m_pendingTerrainInfo.adfGeoTransform[5],
			m_pendingTerrainInfo.szLayerName);
	}
	else
	{
		osg::BoundingSphere bs = m_pendingTerrainInfo.pObject->getBound();
		strMsg.Format(_T("Mesh built / Normals ready / Texcoords ready\r\nLocal radius: %.3f\r\nLayer: %s"),
			bs.radius(), m_pendingTerrainInfo.szLayerName);
	}
	UpdateTerrainStatus(strMsg);
	return true;
}

void CGIS3DView::MarkTerrainNormalsReady()
{
	if (!m_bTerrainMeshBuilt && !BuildTerrainWorkflowModel())
		return;

	m_bTerrainNormalsReady = true;
	UpdateTerrainStatus(_T("Normal vectors are ready\r\nThe current terrain mesh already contains per-vertex normals."));
}

void CGIS3DView::MarkTerrainTexcoordsReady()
{
	if (!m_bTerrainMeshBuilt && !BuildTerrainWorkflowModel())
		return;

	m_bTerrainTexcoordsReady = true;
	UpdateTerrainStatus(_T("Texture coordinates are ready\r\nDEM/Image georeference mapping has been applied."));
}

void CGIS3DView::AddTerrainWorkflowToScene()
{
	if (!m_bTerrainMeshBuilt && !BuildTerrainWorkflowModel())
		return;

	if (m_bTerrainAdded)
	{
		UpdateTerrainStatus(_T("Terrain layer is already in the scene\r\nPress R to rebuild current DEM/Image."));
		return;
	}

	AddEvent(m_pendingTerrainInfo);
	CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (pFrame)
		pFrame->AddLayerToPane(m_pendingTerrainInfo.szLayerName);

	g_bInit = true;
	m_bTerrainAdded = true;
	SetTopViewForNode(m_pendingTerrainInfo.pObject);

	CString strMsg;
	strMsg.Format(_T("Terrain added to scene\r\nLayer: %s\r\nTop view activated"), m_pendingTerrainInfo.szLayerName);
	UpdateTerrainStatus(strMsg);
}

void CGIS3DView::SetTopViewForNode(osg::Node* pNode)
{
	if (!pNode || !m_OSG || !m_OSG->getViewer())
	{
		SetTopView();
		return;
	}

	osg::BoundingSphere bs = pNode->getBound();
	osg::Vec3d center = bs.center();
	double radius = bs.radius();
	if (radius < 100.0)
		radius = 100.0;

	osg::Vec3d eye(center.x(), center.y(), center.z() + radius * 2.8);
	osg::Vec3d up(0.0, 1.0, 0.0);
	ApplyCameraView(eye, center, up);
}
void CGIS3DView::CreateCameraHandle()
{
    if (m_bCameraHandleCreated)
        return;

    CRect rectDummy(0, 0, CAMERA_HANDLE_SIZE, CAMERA_HANDLE_SIZE);
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;

    m_btnViewUp.Create(_T("UP"), dwStyle, rectDummy, this, IDC_VIEW_HANDLE_UP);
    m_btnViewDown.Create(_T("DN"), dwStyle, rectDummy, this, IDC_VIEW_HANDLE_DOWN);
    m_btnViewLeft.Create(_T("LT"), dwStyle, rectDummy, this, IDC_VIEW_HANDLE_LEFT);
    m_btnViewRight.Create(_T("RT"), dwStyle, rectDummy, this, IDC_VIEW_HANDLE_RIGHT);
    m_btnViewTop.Create(_T("TOP"), dwStyle, rectDummy, this, IDC_VIEW_HANDLE_TOP);
    m_btnViewHome.Create(_T("ALL"), dwStyle, rectDummy, this, IDC_VIEW_HANDLE_HOME);

    m_bCameraHandleCreated = true;
    LayoutCameraHandle();
}

void CGIS3DView::LayoutCameraHandle()
{
    if (!m_bCameraHandleCreated || !::IsWindow(m_btnViewUp.GetSafeHwnd()))
        return;

    CRect rectClient;
    GetClientRect(rectClient);

    int size = CAMERA_HANDLE_SIZE;
    int gap = 4;
    int totalWidth = size * 3 + gap * 2;
    int left = rectClient.right - totalWidth - CAMERA_HANDLE_MARGIN;
    int top = rectClient.top + CAMERA_HANDLE_MARGIN;

    if (left < CAMERA_HANDLE_MARGIN)
        left = CAMERA_HANDLE_MARGIN;

    m_btnViewUp.MoveWindow(left + size + gap, top, size, size);
    m_btnViewLeft.MoveWindow(left, top + size + gap, size, size);
    m_btnViewTop.MoveWindow(left + size + gap, top + size + gap, size, size);
    m_btnViewRight.MoveWindow(left + (size + gap) * 2, top + size + gap, size, size);
    m_btnViewDown.MoveWindow(left + size + gap, top + (size + gap) * 2, size, size);
    m_btnViewHome.MoveWindow(left + (size + gap) * 2, top + (size + gap) * 2, size, size);
}

void CGIS3DView::ApplyCameraView(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up)
{
    TRACE(_T("[BG-DIAG] ApplyCameraView eye=(%.3f,%.3f,%.3f) center=(%.3f,%.3f,%.3f) up=(%.3f,%.3f,%.3f)\n"), eye.x(), eye.y(), eye.z(), center.x(), center.y(), center.z(), up.x(), up.y(), up.z());
    if (!m_OSG || !m_OSG->getViewer())
        return;

    osgViewer::Viewer* pViewer = m_OSG->getViewer();
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

void CGIS3DView::OrbitCamera(double dAzimuthDegree, double dElevationDegree)
{
    if (!m_OSG || !m_OSG->getViewer() || !m_OSG->getViewer()->getCamera())
        return;

    osg::Vec3d eye, center, up;
    m_OSG->getViewer()->getCamera()->getViewMatrixAsLookAt(eye, center, up);

    osg::Vec3d offset = eye - center;
    double radius = offset.length();
    if (radius < 1.0)
    {
        osg::BoundingSphere bs = m_OSG->getRoot() ? m_OSG->getRoot()->getBound() : osg::BoundingSphere();
        center = bs.center();
        radius = bs.radius() * 2.5;
        if (radius < 100.0)
            radius = 100.0;
        offset.set(0.0, -radius, radius * 0.35);
    }

    double xy = sqrt(offset.x() * offset.x() + offset.y() * offset.y());
    double azimuth = atan2(offset.y(), offset.x()) + dAzimuthDegree * PI / 180.0;
    double elevation = atan2(offset.z(), xy) + dElevationDegree * PI / 180.0;

    double maxElevation = 89.0 * PI / 180.0;
    if (elevation > maxElevation) elevation = maxElevation;
    if (elevation < -maxElevation) elevation = -maxElevation;

    osg::Vec3d newEye;
    newEye.x() = center.x() + radius * cos(elevation) * cos(azimuth);
    newEye.y() = center.y() + radius * cos(elevation) * sin(azimuth);
    newEye.z() = center.z() + radius * sin(elevation);

    osg::Vec3d newUp(0.0, 0.0, 1.0);
    if (fabs(fabs(elevation) - maxElevation) < 0.001)
        newUp.set(0.0, 1.0, 0.0);

    ApplyCameraView(newEye, center, newUp);
}

void CGIS3DView::SetTopView()
{
    TRACE(_T("[BG-DIAG] SetTopView called\n"));
    if (!m_OSG || !m_OSG->getViewer())
        return;

    osg::BoundingSphere bs = m_OSG->getRoot() ? m_OSG->getRoot()->getBound() : osg::BoundingSphere();
    osg::Vec3d center = bs.center();
    double radius = bs.radius();
    if (radius < 100.0)
        radius = 100.0;

    osg::Vec3d eye(center.x(), center.y(), center.z() + radius * 2.8);
    osg::Vec3d up(0.0, 1.0, 0.0);
    TRACE(_T("[BG-DIAG] SetTopView target eye=(%.3f,%.3f,%.3f) center=(%.3f,%.3f,%.3f) up=(%.3f,%.3f,%.3f)\n"), eye.x(), eye.y(), eye.z(), center.x(), center.y(), center.z(), up.x(), up.y(), up.z());
    ApplyCameraView(eye, center, up);
}

void CGIS3DView::SetHomeView()
{
    TRACE(_T("[BG-DIAG] SetHomeView called\n"));
    if (!m_OSG || !m_OSG->getViewer())
        return;

    osg::BoundingSphere bs = m_OSG->getRoot() ? m_OSG->getRoot()->getBound() : osg::BoundingSphere();
    osg::Vec3d center = bs.center();
    double radius = bs.radius();
    if (radius < 100.0)
        radius = 100.0;

    osg::Vec3d eye(center.x() - radius * 1.4, center.y() - radius * 1.8, center.z() + radius * 0.9);
    osg::Vec3d up(0.0, 0.0, 1.0);
    TRACE(_T("[BG-DIAG] SetHomeView target eye=(%.3f,%.3f,%.3f) center=(%.3f,%.3f,%.3f) up=(%.3f,%.3f,%.3f)\n"), eye.x(), eye.y(), eye.z(), center.x(), center.y(), center.z(), up.x(), up.y(), up.z());
    ApplyCameraView(eye, center, up);
}

void CGIS3DView::OnViewHandleUp()
{
    OrbitCamera(0.0, 12.0);
}

void CGIS3DView::OnViewHandleDown()
{
    OrbitCamera(0.0, -12.0);
}

void CGIS3DView::OnViewHandleLeft()
{
    OrbitCamera(-15.0, 0.0);
}

void CGIS3DView::OnViewHandleRight()
{
    OrbitCamera(15.0, 0.0);
}

void CGIS3DView::OnViewHandleTop()
{
    SetTopView();
}

void CGIS3DView::OnViewHandleHome()
{
    SetHomeView();
}
void CGIS3DView::OnLoadTerrain()
{
	// 1. ѡ�� DEM �ļ�������ѡ��
	CString strDemFilter = _T("DEM Files (*.tif;*.tiff;*.img;*.png;*.jpg;*.bmp)|*.tif;*.tiff;*.img;*.png;*.jpg;*.bmp|All Files (*.*)|*.*||");
	CFileDialog dlgDem(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, strDemFilter, NULL);
	dlgDem.m_ofn.lpstrTitle = _T("Select DEM File");

	if (dlgDem.DoModal() != IDOK)
		return;

	CString strDem = dlgDem.GetPathName();

	// 2. ѡ��Ӱ���ļ�������ȡ����
	CString strImage = _T("");
	CString strImgFilter = _T("Image Files (*.tif;*.tiff;*.jpg;*.jpeg;*.png;*.bmp)|*.tif;*.tiff;*.jpg;*.jpeg;*.png;*.bmp|All Files (*.*)|*.*||");
	CFileDialog dlgImg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, strImgFilter, NULL);
	dlgImg.m_ofn.lpstrTitle = _T("Select Image File (Optional)");

	if (dlgImg.DoModal() == IDOK)
	{
		strImage = dlgImg.GetPathName();
	}

	CWaitCursor wait;

	SceneObjectInfo info;
	memset(&info, 0x00, sizeof(SceneObjectInfo));

	CSceneObject obj;
	info.pObject = obj.CreateTerrain(strDem, strImage, &info);

	if (!info.pObject)
	{
		AfxMessageBox(_T("���μ���ʧ�ܣ����� DEM �ļ���ʽ�Ƿ���ȷ��"), MB_OK | MB_ICONERROR);
		return;
	}

	lstrcpy(info.szLayerName, _T("����"));
	AddEvent(info);
	CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (pFrame)
		pFrame->AddLayerToPane(info.szLayerName);
	// Keep the original camera state after terrain import; do not auto-switch to top view.


	// ��ģ�͵��뱣��һ��
	g_bInit = true;

	if (strImage.IsEmpty())
	{
		AfxMessageBox(_T("No image selected. DEM terrain only."), MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		AfxMessageBox(_T("Terrain and image loaded successfully."), MB_OK | MB_ICONINFORMATION);
	}
}

void CGIS3DView::OnLoadModel()
{
	//�ڴ˼ӶԻ���ѡ��ģ���ļ�

	CString strFilter = _T("3D Model Files (*.osgb)|*.osgb|All Files (*.*)|*.*||");
	CFileDialog fdlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter, NULL);
	fdlg.m_ofn.lpstrTitle = _T("Select Model File");

	if (fdlg.DoModal() == IDCANCEL)
		return;

	CString strModel = fdlg.GetPathName();

	SceneObjectInfo info;
	memset(&info, 0x00, sizeof(SceneObjectInfo));
	CSceneObject obj;	
	info.pObject = obj.LoadModel(strModel);//�˺�����д����Ҫ�Լ���֤������ڸú����ϰ��Ҽ�����ת�����塱������ú���
	if (!info.pObject)
		return;

	lstrcpy(info.szLayerName, "Model");

	g_bInit = true;//��Ҫ�Դ�Ŀ��Ϊ���Ľ������������Ϊ��?

	AddEvent(info);
	CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (pFrame)
		pFrame->AddLayerToPane(info.szLayerName);
}

void CGIS3DView::OnLoadBatchModels()
{
	// 1. ���жϳ����Ƿ��Ѿ���ʼ�����?
	if (!m_OSG || !m_OSG->getViewer())
	{
		AfxMessageBox(_T("��ά������δ��ʼ������ʱ������������ģ�͡�"), MB_OK | MB_ICONWARNING);
		return;
	}

	CString strFilter = _T("3D Model Files (*.osgb)|*.osgb|All Files (*.*)|*.*||");

	// 2. ��ѡ�Ի��򣬼��� OFN_EXPLORER ����
	CFileDialog fdlg(TRUE, _T("osgb"), NULL,
		OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_EXPLORER,
		strFilter, NULL);
	fdlg.m_ofn.lpstrTitle = _T("Select Models");

	// 3. ��ѡʱ�����ṩ�㹻������
	const int FILE_LIST_BUFFER_SIZE = 8192;
	TCHAR* pBuffer = new TCHAR[FILE_LIST_BUFFER_SIZE];
	memset(pBuffer, 0, sizeof(TCHAR) * FILE_LIST_BUFFER_SIZE);
	fdlg.m_ofn.lpstrFile = pBuffer;
	fdlg.m_ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

	// 4. �û�ȡ��ʱֱ���˳�
	if (fdlg.DoModal() != IDOK)
	{
		delete[] pBuffer;
		return;
	}

	// 5. �����ڼ���ʾ�ȴ����?
	CWaitCursor wait;

	bool bLoaded = false;
	int nModelIndex = 1;
	int nSuccessCount = 0;
	int nFailCount = 0;
	int nTotalCount = 0;

	// ����¼ǰ 5 ��ʧ���ļ�����������ʾ��̫��
	CString strFailedList;

	POSITION pos = fdlg.GetStartPosition();
	while (pos != NULL)
	{
		CString path = fdlg.GetNextPathName(pos);
		nTotalCount++;

		SceneObjectInfo info;
		memset(&info, 0x00, sizeof(SceneObjectInfo));

		CSceneObject obj;
		info.pObject = obj.LoadModel(path);

		if (info.pObject)
		{
			// �����ȼ���ʹ�ö̱�����������ڲ�֪��?szLayerName ����ʱд���ַ���
			CString strLayerName;
			strLayerName.Format(_T("ģ��_%03d"), nModelIndex);

			lstrcpy(info.szLayerName, strLayerName);

			AddEvent(info);

			CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
			if (pFrame)
				pFrame->AddLayerToPane(info.szLayerName);

			bLoaded = true;
			nSuccessCount++;
			nModelIndex++;
		}
		else
		{
			nFailCount++;

			int nPos = path.ReverseFind(_T('\\'));
			CString strFileName = (nPos >= 0) ? path.Mid(nPos + 1) : path;

			if (nFailCount <= 5)
			{
				if (!strFailedList.IsEmpty())
					strFailedList += _T("\n");

				strFailedList += strFileName;
			}
		}
	}

	// 6. ֻ�����ٳɹ�������һ��ģ�ͣ��Ŵ�����Ŀ��Ϊ�������?
	if (bLoaded)
	{
		g_bInit = true;
	}

	// 7. ��������������
	CString strMsg;
	if (nSuccessCount > 0 && nFailCount == 0)
	{
		strMsg.Format(_T("����������ɣ�\n��ѡ�� %d ���ļ�\n�ɹ����� %d ��\nʧ�� %d ��"),
			nTotalCount, nSuccessCount, nFailCount);
		AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
	}
	else if (nSuccessCount > 0 && nFailCount > 0)
	{
		strMsg.Format(_T("����������ɣ����в����ļ�����ʧ�ܡ�\n��ѡ�� %d ���ļ�\n�ɹ����� %d ��\nʧ�� %d ��"),
			nTotalCount, nSuccessCount, nFailCount);

		if (!strFailedList.IsEmpty())
		{
			strMsg += _T("\n\nʧ���ļ��������ʾ�?������\n");
			strMsg += strFailedList;
		}

		AfxMessageBox(strMsg, MB_OK | MB_ICONWARNING);
	}
	else
	{
		strMsg.Format(_T("��������ʧ�ܣ�\n��ѡ�� %d ���ļ�\n�ɹ����� %d ��\nʧ�� %d ��"),
			nTotalCount, nSuccessCount, nFailCount);

		if (!strFailedList.IsEmpty())
		{
			strMsg += _T("\n\nʧ���ļ��������ʾ�?������\n");
			strMsg += strFailedList;
		}

		AfxMessageBox(strMsg, MB_OK | MB_ICONERROR);
	}

    delete[] pBuffer;
}

void CGIS3DView::OnExpSaveBookmark()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->SaveBookmark(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("�ӽ���ǩ����ʧ�ܡ�") : strMsg);
}

void CGIS3DView::OnExpRestoreBookmark()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->RestoreBookmark(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("�ӽ���ǩ�ָ�ʧ�ܡ�") : strMsg);
}

void CGIS3DView::OnExpBatchOverview()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->FocusBatchModels(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("����ģ������ʧ�ܡ�") : strMsg);
}

void CGIS3DView::OnExpScreenshot()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->ExportScreenshot(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("������ͼ����ʧ�ܡ�") : strMsg);
}

void CGIS3DView::OnExpObjectInspect()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->ToggleInspector(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("�������Բ�ѯ�л�ʧ�ܡ�") : strMsg);
}

void CGIS3DView::OnExpTerrainQuery()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->ToggleTerrainQuery(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("���θ̲߳�ѯ�л�ʧ�ܡ�") : strMsg);
}

void CGIS3DView::OnExpWireframe()
{
	CString strMsg;
	if (EnsureExperimentFeatures() && m_pExperimentFeatures->ToggleWireframe(strMsg))
		UpdateTerrainStatus(strMsg);
	else
		UpdateTerrainStatus(strMsg.IsEmpty() ? _T("�߿�ģʽ�л�ʧ�ܡ�") : strMsg);
}

