
// GIS3DView.h: CGIS3DView ��Ľӿ�
//

#pragma once

#include "OsgScene.h"

class CExperimentFeatures;

class CGIS3DView : public CView
{
protected: // �������л�����
	CGIS3DView() noexcept;
	DECLARE_DYNCREATE(CGIS3DView)

// ����
public:
	CGIS3DDoc* GetDocument() const;

	COsgScene* m_OSG;
	HANDLE mThreadHandle;
	//	CRenderingThread* mThreadHandle;

	void StopRenderThread();
	void ResumeRenderThread();

	bool m_bInit;

	CButton m_btnViewUp;
	CButton m_btnViewDown;
	CButton m_btnViewLeft;
	CButton m_btnViewRight;
	CButton m_btnViewTop;
	CButton m_btnViewHome;
	bool m_bCameraHandleCreated;
	void CreateCameraHandle();
	void LayoutCameraHandle();
	void OrbitCamera(double dAzimuthDegree, double dElevationDegree);
	void SetTopView();
	void SetHomeView();
	void ApplyCameraView(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up);

	CStatic m_wndTerrainStatus;
	CString m_strWorkflowDemPath;
	CString m_strWorkflowImagePath;
	CString m_strWorkflowLayerName;
	SceneObjectInfo m_pendingTerrainInfo;
	bool m_bTerrainStatusCreated;
	bool m_bTerrainDataSelected;
	bool m_bTerrainMeshBuilt;
	bool m_bTerrainNormalsReady;
	bool m_bTerrainTexcoordsReady;
	bool m_bTerrainAdded;
	void CreateTerrainStatusWindow();
	void LayoutTerrainStatusWindow();
	void UpdateTerrainStatus(const CString& strMessage);
	void ResetTerrainWorkflow(bool bClearFiles = true);
	bool SelectTerrainWorkflowFiles();
	bool BuildTerrainWorkflowModel();
	void MarkTerrainNormalsReady();
	void MarkTerrainTexcoordsReady();
	void AddTerrainWorkflowToScene();
	void SetTopViewForNode(osg::Node* pNode);

	CExperimentFeatures* m_pExperimentFeatures;
	bool EnsureExperimentFeatures();
// ����
public:

// ��д
public:
	virtual void OnDraw(CDC* pDC);  // ��д�Ի��Ƹ���ͼ
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// ʵ��
public:
	virtual ~CGIS3DView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// ���ɵ���Ϣӳ�亯��
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnViewHandleUp();
	afx_msg void OnViewHandleDown();
	afx_msg void OnViewHandleLeft();
	afx_msg void OnViewHandleRight();
	afx_msg void OnViewHandleTop();
	afx_msg void OnViewHandleHome();
	void AddEvent(SceneObjectInfo info);
	afx_msg void OnLoadTerrain();
	afx_msg void OnLoadModel();
	afx_msg void OnLoadBatchModels();
	afx_msg void OnExpSaveBookmark();
	afx_msg void OnExpRestoreBookmark();
	afx_msg void OnExpBatchOverview();
	afx_msg void OnExpScreenshot();
	afx_msg void OnExpObjectInspect();
	afx_msg void OnExpTerrainQuery();
	afx_msg void OnExpWireframe();
};

#ifndef _DEBUG  // GIS3DView.cpp �еĵ��԰汾
inline CGIS3DDoc* CGIS3DView::GetDocument() const
   { return reinterpret_cast<CGIS3DDoc*>(m_pDocument); }
#endif

