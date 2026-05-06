// pch.cpp: 与预编译标头对应的源文件

#include <fstream>
#include "pch.h"


Point3 g_ptSceneCenter;//场景中心点
bool   g_bInit = true;

bool   g_bGeoReferenceReady = false;
double g_dGeoOriginX = 0.0;
double g_dGeoOriginY = 0.0;
double g_dGeoOriginZ = 0.0;
double g_dGeoSceneScale = 1.0;
double g_dGeoUnitScaleX = 1.0;
double g_dGeoUnitScaleY = 1.0;
double g_dGeoMinX = 0.0;
double g_dGeoMinY = 0.0;
double g_dGeoMaxX = 0.0;
double g_dGeoMaxY = 0.0;
char   g_szSceneProjection[2048] = { 0 };

void SetSceneGeoReference(const SceneObjectInfo& info)
{
	if (!info.bHasGeoReference)
		return;

	g_bGeoReferenceReady = true;
	g_dGeoOriginX = info.dGeoOriginX;
	g_dGeoOriginY = info.dGeoOriginY;
	g_dGeoOriginZ = info.dGeoOriginZ;
	g_dGeoSceneScale = (info.dGeoSceneScale > 0.0) ? info.dGeoSceneScale : 1.0;
	g_dGeoUnitScaleX = (info.dGeoUnitScaleX > 0.0) ? info.dGeoUnitScaleX : 1.0;
	g_dGeoUnitScaleY = (info.dGeoUnitScaleY > 0.0) ? info.dGeoUnitScaleY : 1.0;
	g_dGeoMinX = info.dGeoMinX;
	g_dGeoMinY = info.dGeoMinY;
	g_dGeoMaxX = info.dGeoMaxX;
	g_dGeoMaxY = info.dGeoMaxY;
	lstrcpyn(g_szSceneProjection, info.szProjection, sizeof(g_szSceneProjection));
}

bool IsLikelyGeoCoordinate(double x, double y)
{
	if (!g_bGeoReferenceReady)
		return false;

	double width = g_dGeoMaxX - g_dGeoMinX;
	double height = g_dGeoMaxY - g_dGeoMinY;
	double margin = ((width > height) ? width : height) * 0.5;
	if (margin < 100.0)
		margin = 100.0;

	return x >= g_dGeoMinX - margin && x <= g_dGeoMaxX + margin &&
		y >= g_dGeoMinY - margin && y <= g_dGeoMaxY + margin;
}

void BGDiagLogLine(const std::string& text)
{
    OutputDebugStringA((text + "\r\n").c_str());
    std::ofstream ofs("bg_diag_runtime.log", std::ios::out | std::ios::app);
    if (ofs.is_open())
        ofs << text << "\r\n";
}

// 当使用预编译的头时，需要使用此源文件，编译才能成功。
//将UTF8转化为GB2312
std::string WINAPI UTF8ToGB2132(std::string  strSrc)
{
	std::string result;
	WCHAR* wstrSrc = NULL;
	char* szRes = NULL;
	int i;

	// UTF8转换成Unicode
	i = MultiByteToWideChar(CP_UTF8, 0, strSrc.c_str(), -1, NULL, 0);
	wstrSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, strSrc.c_str(), -1, wstrSrc, i);

	// Unicode转换成GB2312
	i = WideCharToMultiByte(CP_ACP, 0, wstrSrc, -1, NULL, 0, NULL, NULL);
	szRes = new char[i + 1];
	WideCharToMultiByte(CP_ACP, 0, wstrSrc, -1, szRes, i, NULL, NULL);

	result = std::string(szRes);
	if (wstrSrc != NULL)
	{
		delete[]wstrSrc;
		wstrSrc = NULL;
	}
	if (szRes != NULL)
	{
		delete[]szRes;
		szRes = NULL;
	}

	return result;
}
