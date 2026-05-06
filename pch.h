// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"

#pragma comment(lib,"lib\\gdal_i.lib")
#include "DataType.h"

#include <OpenThreads/Thread>
#include <OpenThreads/Condition>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <sstream>

extern OpenThreads::Mutex g_mutex;

extern Point3 g_ptSceneCenter;//场景中心点
extern bool   g_bInit;

extern bool   g_bGeoReferenceReady;
extern double g_dGeoOriginX;
extern double g_dGeoOriginY;
extern double g_dGeoOriginZ;
extern double g_dGeoSceneScale;
extern double g_dGeoUnitScaleX;
extern double g_dGeoUnitScaleY;
extern double g_dGeoMinX;
extern double g_dGeoMinY;
extern double g_dGeoMaxX;
extern double g_dGeoMaxY;
extern char   g_szSceneProjection[2048];

void SetSceneGeoReference(const SceneObjectInfo& info);
bool IsLikelyGeoCoordinate(double x, double y);

std::string WINAPI UTF8ToGB2132(std::string  strSrc);
void BGDiagLogLine(const std::string& text);

#define BG_DIAG_LOG(expr) \
    do { std::ostringstream _bg_diag_oss; _bg_diag_oss << expr; BGDiagLogLine(_bg_diag_oss.str()); } while(0)

#endif //PCH_H
