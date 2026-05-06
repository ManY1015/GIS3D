#pragma once

#include "resource.h"

class CLayerPropertyDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CLayerPropertyDialog)

public:
	CLayerPropertyDialog(CWnd* pParent = NULL);
	virtual ~CLayerPropertyDialog();

	CString m_strLayerName;
	int m_nOpacity;

	enum { IDD = IDD_LAYER_PROPERTY_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
