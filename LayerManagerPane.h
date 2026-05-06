#pragma once

#include <afxcontrolbars.h>
#include <afxcmn.h>

#include "DataType.h"

class CLayerManagerPane : public CDockablePane
{
public:
	CLayerManagerPane();
	virtual ~CLayerManagerPane();

	void AddLayer(const CString& strLayerName);
	void RemoveLayer(const CString& strLayerName);

protected:
	CListCtrl m_wndList;
	CButton m_btnVisible;
	CButton m_btnDelete;
	CButton m_btnUp;
	CButton m_btnDown;
	CSliderCtrl m_sliderOpacity;
	bool m_bInternalUpdate;

	void AdjustLayout();
	void UpdateControls();
	int GetSelectedIndex();
	CString GetSelectedLayerName();
	int FindLayer(const CString& strLayerName);
	void SwapLayerRows(int nFirst, int nSecond);
	int GetLayerOpacity(int nIndex) const;
	bool EditLayerProperties(int nIndex);
	void SendLayerCommand(int nMsg, const CString& strLayerName, bool bVisible, float fOpacity, int nNewPos, const CString& strNewLayerName = _T(""));

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLayerItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLayerRightClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVisibleClicked();
	afx_msg void OnDeleteClicked();
	afx_msg void OnMoveUpClicked();
	afx_msg void OnMoveDownClicked();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	DECLARE_MESSAGE_MAP()
};
