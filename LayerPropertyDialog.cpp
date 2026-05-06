#include "pch.h"
#include "LayerPropertyDialog.h"

IMPLEMENT_DYNAMIC(CLayerPropertyDialog, CDialogEx)

CLayerPropertyDialog::CLayerPropertyDialog(CWnd* pParent)
	: CDialogEx(IDD_LAYER_PROPERTY_DIALOG, pParent)
	, m_nOpacity(100)
{
}

CLayerPropertyDialog::~CLayerPropertyDialog()
{
}

void CLayerPropertyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_LAYER_NAME, m_strLayerName);
	DDX_Text(pDX, IDC_EDIT_LAYER_OPACITY, m_nOpacity);
	DDV_MinMaxInt(pDX, m_nOpacity, 0, 100);
}

BEGIN_MESSAGE_MAP(CLayerPropertyDialog, CDialogEx)
END_MESSAGE_MAP()

BOOL CLayerPropertyDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("图层属性"));
	return TRUE;
}

void CLayerPropertyDialog::OnOK()
{
	UpdateData(TRUE);
	m_strLayerName.Trim();
	if (m_strLayerName.IsEmpty())
	{
		AfxMessageBox(_T("图层名称不能为空。"), MB_OK | MB_ICONWARNING);
		return;
	}

	CDialogEx::OnOK();
}
