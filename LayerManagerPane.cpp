#include "pch.h"
#include "LayerManagerPane.h"
#include "LayerPropertyDialog.h"
#include "MainFrm.h"

#define IDC_LAYER_LIST      4001
#define IDC_LAYER_VISIBLE   4002
#define IDC_LAYER_DELETE    4003
#define IDC_LAYER_UP        4004
#define IDC_LAYER_DOWN      4005
#define IDC_LAYER_OPACITY   4006
#define ID_LAYER_EDIT_PROPERTIES  45001

BEGIN_MESSAGE_MAP(CLayerManagerPane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LAYER_LIST, &CLayerManagerPane::OnLayerItemChanged)
	ON_NOTIFY(NM_RCLICK, IDC_LAYER_LIST, &CLayerManagerPane::OnLayerRightClick)
	ON_BN_CLICKED(IDC_LAYER_VISIBLE, &CLayerManagerPane::OnVisibleClicked)
	ON_BN_CLICKED(IDC_LAYER_DELETE, &CLayerManagerPane::OnDeleteClicked)
	ON_BN_CLICKED(IDC_LAYER_UP, &CLayerManagerPane::OnMoveUpClicked)
	ON_BN_CLICKED(IDC_LAYER_DOWN, &CLayerManagerPane::OnMoveDownClicked)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

CLayerManagerPane::CLayerManagerPane()
{
	m_bInternalUpdate = false;
}

CLayerManagerPane::~CLayerManagerPane()
{
}

int CLayerManagerPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy(0, 0, 0, 0);

	m_wndList.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		rectDummy, this, IDC_LAYER_LIST);
	m_wndList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_wndList.InsertColumn(0, _T("图层"), LVCFMT_LEFT, 120);
	m_wndList.InsertColumn(1, _T("显示"), LVCFMT_CENTER, 50);
	m_wndList.InsertColumn(2, _T("透明度"), LVCFMT_CENTER, 70);

	m_btnVisible.Create(_T("显示"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		rectDummy, this, IDC_LAYER_VISIBLE);
	m_btnDelete.Create(_T("删除"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectDummy, this, IDC_LAYER_DELETE);
	m_btnUp.Create(_T("上移"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectDummy, this, IDC_LAYER_UP);
	m_btnDown.Create(_T("下移"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		rectDummy, this, IDC_LAYER_DOWN);
	m_sliderOpacity.Create(WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
		rectDummy, this, IDC_LAYER_OPACITY);
	m_sliderOpacity.SetRange(0, 100);
	m_sliderOpacity.SetPos(100);

	UpdateControls();
	return 0;
}

void CLayerManagerPane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CLayerManagerPane::AdjustLayout()
{
	if (!::IsWindow(m_wndList.GetSafeHwnd()))
		return;

	CRect rectClient;
	GetClientRect(rectClient);

	const int margin = 8;
	const int buttonHeight = 24;
	const int sliderHeight = 30;
	const int bottomHeight = buttonHeight * 2 + sliderHeight + margin * 4;

	CRect rectList(rectClient.left + margin, rectClient.top + margin,
		rectClient.right - margin, rectClient.bottom - bottomHeight);
	m_wndList.MoveWindow(rectList);

	int top = rectList.bottom + margin;
	int width = rectClient.Width() - margin * 2;
	int halfWidth = (width - margin) / 2;

	m_btnVisible.MoveWindow(rectClient.left + margin, top, halfWidth, buttonHeight);
	m_btnDelete.MoveWindow(rectClient.left + margin + halfWidth + margin, top, halfWidth, buttonHeight);

	top += buttonHeight + margin;
	m_btnUp.MoveWindow(rectClient.left + margin, top, halfWidth, buttonHeight);
	m_btnDown.MoveWindow(rectClient.left + margin + halfWidth + margin, top, halfWidth, buttonHeight);

	top += buttonHeight + margin;
	m_sliderOpacity.MoveWindow(rectClient.left + margin, top, width, sliderHeight);
}

void CLayerManagerPane::AddLayer(const CString& strLayerName)
{
	if (strLayerName.IsEmpty())
		return;

	int nIndex = FindLayer(strLayerName);
	if (nIndex < 0)
	{
		nIndex = m_wndList.InsertItem(m_wndList.GetItemCount(), strLayerName);
		m_wndList.SetItemText(nIndex, 1, _T("是"));
		m_wndList.SetItemText(nIndex, 2, _T("100"));
	}

	m_wndList.SetItemState(nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	UpdateControls();
}

void CLayerManagerPane::RemoveLayer(const CString& strLayerName)
{
	int nIndex = FindLayer(strLayerName);
	if (nIndex >= 0)
		m_wndList.DeleteItem(nIndex);

	UpdateControls();
}

int CLayerManagerPane::FindLayer(const CString& strLayerName)
{
	for (int i = 0; i < m_wndList.GetItemCount(); ++i)
	{
		if (m_wndList.GetItemText(i, 0).CompareNoCase(strLayerName) == 0)
			return i;
	}
	return -1;
}

int CLayerManagerPane::GetSelectedIndex()
{
	POSITION pos = m_wndList.GetFirstSelectedItemPosition();
	if (!pos)
		return -1;
	return m_wndList.GetNextSelectedItem(pos);
}

CString CLayerManagerPane::GetSelectedLayerName()
{
	int nIndex = GetSelectedIndex();
	if (nIndex < 0)
		return _T("");
	return m_wndList.GetItemText(nIndex, 0);
}

int CLayerManagerPane::GetLayerOpacity(int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_wndList.GetItemCount())
		return 100;

	int nOpacity = _ttoi(m_wndList.GetItemText(nIndex, 2));
	if (nOpacity < 0)
		nOpacity = 0;
	if (nOpacity > 100)
		nOpacity = 100;
	return nOpacity;
}

void CLayerManagerPane::UpdateControls()
{
	int nIndex = GetSelectedIndex();
	bool bEnabled = nIndex >= 0;

	m_btnVisible.EnableWindow(bEnabled);
	m_btnDelete.EnableWindow(bEnabled);
	m_btnUp.EnableWindow(bEnabled && nIndex > 0);
	m_btnDown.EnableWindow(bEnabled && nIndex < m_wndList.GetItemCount() - 1);
	m_sliderOpacity.EnableWindow(bEnabled);

	m_bInternalUpdate = true;
	if (bEnabled)
	{
		m_btnVisible.SetCheck(m_wndList.GetItemText(nIndex, 1) == _T("是") ? BST_CHECKED : BST_UNCHECKED);
		m_sliderOpacity.SetPos(GetLayerOpacity(nIndex));
	}
	else
	{
		m_btnVisible.SetCheck(BST_UNCHECKED);
		m_sliderOpacity.SetPos(100);
	}
	m_bInternalUpdate = false;
}

void CLayerManagerPane::OnLayerItemChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	UpdateControls();
	*pResult = 0;
}

void CLayerManagerPane::OnLayerRightClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	DWORD dwPos = ::GetMessagePos();
	CPoint pointScreen(GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos));
	CPoint pointClient = pointScreen;
	m_wndList.ScreenToClient(&pointClient);

	LVHITTESTINFO hitInfo = {};
	hitInfo.pt = pointClient;
	int nIndex = m_wndList.SubItemHitTest(&hitInfo);
	if (nIndex < 0)
	{
		*pResult = 0;
		return;
	}

	m_wndList.SetItemState(nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	UpdateControls();

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, ID_LAYER_EDIT_PROPERTIES, _T("修改名称/透明度..."));

	UINT nCmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pointScreen.x, pointScreen.y, this);
	if (nCmd == ID_LAYER_EDIT_PROPERTIES)
		EditLayerProperties(nIndex);

	*pResult = 0;
}

void CLayerManagerPane::OnVisibleClicked()
{
	if (m_bInternalUpdate)
		return;

	int nIndex = GetSelectedIndex();
	if (nIndex < 0)
		return;

	bool bVisible = m_btnVisible.GetCheck() == BST_CHECKED;
	m_wndList.SetItemText(nIndex, 1, bVisible ? _T("是") : _T("否"));
	SendLayerCommand(SCENE_MSG_VISIBLE, m_wndList.GetItemText(nIndex, 0), bVisible, 1.0f, nIndex);
}

void CLayerManagerPane::OnDeleteClicked()
{
	int nIndex = GetSelectedIndex();
	if (nIndex < 0)
		return;

	CString strLayerName = m_wndList.GetItemText(nIndex, 0);
	SendLayerCommand(SCENE_MSG_DELETE_LAYER, strLayerName, false, 1.0f, nIndex);
	m_wndList.DeleteItem(nIndex);
	UpdateControls();
}

void CLayerManagerPane::SwapLayerRows(int nFirst, int nSecond)
{
	CString firstName = m_wndList.GetItemText(nFirst, 0);
	CString firstVisible = m_wndList.GetItemText(nFirst, 1);
	CString firstOpacity = m_wndList.GetItemText(nFirst, 2);

	m_wndList.SetItemText(nFirst, 0, m_wndList.GetItemText(nSecond, 0));
	m_wndList.SetItemText(nFirst, 1, m_wndList.GetItemText(nSecond, 1));
	m_wndList.SetItemText(nFirst, 2, m_wndList.GetItemText(nSecond, 2));

	m_wndList.SetItemText(nSecond, 0, firstName);
	m_wndList.SetItemText(nSecond, 1, firstVisible);
	m_wndList.SetItemText(nSecond, 2, firstOpacity);
}

void CLayerManagerPane::OnMoveUpClicked()
{
	int nIndex = GetSelectedIndex();
	if (nIndex <= 0)
		return;

	CString strLayerName = m_wndList.GetItemText(nIndex, 0);
	SwapLayerRows(nIndex, nIndex - 1);
	m_wndList.SetItemState(nIndex - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SendLayerCommand(SCENE_MSG_ORDER, strLayerName, true, 1.0f, nIndex - 1);
	UpdateControls();
}

void CLayerManagerPane::OnMoveDownClicked()
{
	int nIndex = GetSelectedIndex();
	if (nIndex < 0 || nIndex >= m_wndList.GetItemCount() - 1)
		return;

	CString strLayerName = m_wndList.GetItemText(nIndex, 0);
	SwapLayerRows(nIndex, nIndex + 1);
	m_wndList.SetItemState(nIndex + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SendLayerCommand(SCENE_MSG_ORDER, strLayerName, true, 1.0f, nIndex + 1);
	UpdateControls();
}

void CLayerManagerPane::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDockablePane::OnHScroll(nSBCode, nPos, pScrollBar);

	if (m_bInternalUpdate || pScrollBar == NULL || pScrollBar->GetSafeHwnd() != m_sliderOpacity.GetSafeHwnd())
		return;

	int nIndex = GetSelectedIndex();
	if (nIndex < 0)
		return;

	int nOpacity = m_sliderOpacity.GetPos();
	CString strOpacity;
	strOpacity.Format(_T("%d"), nOpacity);
	m_wndList.SetItemText(nIndex, 2, strOpacity);
	SendLayerCommand(SCENE_MSG_OPACITY, m_wndList.GetItemText(nIndex, 0), true, nOpacity / 100.0f, nIndex);
}

bool CLayerManagerPane::EditLayerProperties(int nIndex)
{
	if (nIndex < 0 || nIndex >= m_wndList.GetItemCount())
		return false;

	CString strOldLayerName = m_wndList.GetItemText(nIndex, 0);
	CLayerPropertyDialog dlg;
	dlg.m_strLayerName = strOldLayerName;
	dlg.m_nOpacity = GetLayerOpacity(nIndex);
	if (dlg.DoModal() != IDOK)
		return false;

	if (strOldLayerName.CompareNoCase(dlg.m_strLayerName) != 0)
	{
		int nExist = FindLayer(dlg.m_strLayerName);
		if (nExist >= 0 && nExist != nIndex)
		{
			AfxMessageBox(_T("图层名称已存在，请使用其他名称。"), MB_OK | MB_ICONWARNING);
			return false;
		}

		// 同步列表与场景节点名称，避免出现列表已改名但场景仍保留旧节点名。
		m_wndList.SetItemText(nIndex, 0, dlg.m_strLayerName);
		SendLayerCommand(SCENE_MSG_RENAME_LAYER, strOldLayerName, true, 1.0f, nIndex, dlg.m_strLayerName);
	}

	if (dlg.m_nOpacity != GetLayerOpacity(nIndex))
	{
		CString strOpacity;
		strOpacity.Format(_T("%d"), dlg.m_nOpacity);
		m_wndList.SetItemText(nIndex, 2, strOpacity);
		m_bInternalUpdate = true;
		m_sliderOpacity.SetPos(dlg.m_nOpacity);
		m_bInternalUpdate = false;
		SendLayerCommand(SCENE_MSG_OPACITY, m_wndList.GetItemText(nIndex, 0), true, dlg.m_nOpacity / 100.0f, nIndex);
	}

	UpdateControls();
	return true;
}

void CLayerManagerPane::SendLayerCommand(int nMsg, const CString& strLayerName, bool bVisible, float fOpacity, int nNewPos, const CString& strNewLayerName)
{
	CMainFrame* pFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (!pFrame)
		return;

	SceneObjectInfo info;
	memset(&info, 0x00, sizeof(SceneObjectInfo));
	info.nMsg = nMsg;
	info.bVisible = bVisible;
	info.fOpacity = fOpacity;
	info.nNewPos = nNewPos;
	lstrcpy(info.szLayerName, strLayerName);
	if (!strNewLayerName.IsEmpty())
		lstrcpy(info.szNewLayerName, strNewLayerName);

	pFrame->SendLayerCommand(info);
}
