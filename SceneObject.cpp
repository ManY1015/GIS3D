#include "pch.h"
#include "SceneObject.h"

#include <vector>
#include <limits>
#include <algorithm>
#include <math.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/StateSet>
#include <osg/Array>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osgDB/ReadFile>


struct RasterGeoMetadata
{
    bool bValid;
    double adfGeoTransform[6];
    double dOriginX;
    double dOriginY;
    double dOriginZ;
    double dSceneScale;
        double dUnitScaleX;
    double dUnitScaleY;
    double dMinX;
    double dMinY;
    double dMaxX;
    double dMaxY;
    CString strProjection;

    RasterGeoMetadata()
    {
        bValid = false;
        memset(adfGeoTransform, 0x00, sizeof(adfGeoTransform));
        dOriginX = 0.0;
        dOriginY = 0.0;
        dOriginZ = 0.0;
        dSceneScale = 1.0;
                dUnitScaleX = 1.0;
        dUnitScaleY = 1.0;
        dMinX = 0.0;
        dMinY = 0.0;
        dMaxX = 0.0;
        dMaxY = 0.0;
    }
};

struct GdalRuntime
{
    HMODULE hModule;
    void (WINAPI* GDALAllRegisterProc)();
    void* (WINAPI* GDALOpenProc)(const char*, int);
    int (WINAPI* GDALGetGeoTransformProc)(void*, double*);
    const char* (WINAPI* GDALGetProjectionRefProc)(void*);
    void (WINAPI* GDALCloseProc)(void*);
    void* (WINAPI* OSRNewSpatialReferenceProc)(const char*);
    int (WINAPI* OSRImportFromWktProc)(void*, char**);
    int (WINAPI* OSRIsGeographicProc)(void*);
    void (WINAPI* OSRDestroySpatialReferenceProc)(void*);

    GdalRuntime()
    {
        memset(this, 0x00, sizeof(GdalRuntime));
    }

    bool Load()
    {
        if (hModule)
            return true;

        hModule = LoadLibraryA("gdal301.dll");
        if (!hModule)
            hModule = LoadLibraryA("gdal301_d.dll");
        if (!hModule)
            return false;

        GDALAllRegisterProc = (void (WINAPI*)())GetProcAddress(hModule, "GDALAllRegister");
        GDALOpenProc = (void* (WINAPI*)(const char*, int))GetProcAddress(hModule, "GDALOpen");
        GDALGetGeoTransformProc = (int (WINAPI*)(void*, double*))GetProcAddress(hModule, "GDALGetGeoTransform");
        GDALGetProjectionRefProc = (const char* (WINAPI*)(void*))GetProcAddress(hModule, "GDALGetProjectionRef");
        GDALCloseProc = (void (WINAPI*)(void*))GetProcAddress(hModule, "GDALClose");
        OSRNewSpatialReferenceProc = (void* (WINAPI*)(const char*))GetProcAddress(hModule, "OSRNewSpatialReference");
        OSRImportFromWktProc = (int (WINAPI*)(void*, char**))GetProcAddress(hModule, "OSRImportFromWkt");
        OSRIsGeographicProc = (int (WINAPI*)(void*))GetProcAddress(hModule, "OSRIsGeographic");
        OSRDestroySpatialReferenceProc = (void (WINAPI*)(void*))GetProcAddress(hModule, "OSRDestroySpatialReference");

        return GDALAllRegisterProc && GDALOpenProc && GDALGetGeoTransformProc &&
            GDALGetProjectionRefProc && GDALCloseProc;
    }
};

static GdalRuntime& GetGdalRuntime()
{
    static GdalRuntime s_runtime;
    return s_runtime;
}

static bool InitGdalOnce()
{
    static bool s_bRegistered = false;
    GdalRuntime& runtime = GetGdalRuntime();
    if (!runtime.Load())
        return false;

    if (!s_bRegistered)
    {
        runtime.GDALAllRegisterProc();
        s_bRegistered = true;
    }
    return true;
}
static void PixelToGeo(const double gt[6], double pixel, double line, double& geoX, double& geoY)
{
    geoX = gt[0] + pixel * gt[1] + line * gt[2];
    geoY = gt[3] + pixel * gt[4] + line * gt[5];
}

static bool ReadRasterGeoMetadata(CString& strDemName, int cols, int rows, RasterGeoMetadata& meta)
{
    if (!InitGdalOnce())
        return false;

    GdalRuntime& runtime = GetGdalRuntime();
    void* pDataset = runtime.GDALOpenProc(strDemName.GetBuffer(0), 0);
    if (!pDataset)
        return false;

    double gt[6] = { 0.0 };
    if (runtime.GDALGetGeoTransformProc(pDataset, gt) != 0)
    {
        runtime.GDALCloseProc(pDataset);
        return false;
    }

    for (int i = 0; i < 6; ++i)
        meta.adfGeoTransform[i] = gt[i];

    const char* pszProjection = runtime.GDALGetProjectionRefProc(pDataset);
    if (pszProjection)
        meta.strProjection = pszProjection;

    double x[4], y[4];
    PixelToGeo(gt, 0.0, 0.0, x[0], y[0]);
    PixelToGeo(gt, (double)cols, 0.0, x[1], y[1]);
    PixelToGeo(gt, 0.0, (double)rows, x[2], y[2]);
    PixelToGeo(gt, (double)cols, (double)rows, x[3], y[3]);

    meta.dMinX = meta.dMaxX = x[0];
    meta.dMinY = meta.dMaxY = y[0];
    for (int i = 1; i < 4; ++i)
    {
        if (x[i] < meta.dMinX) meta.dMinX = x[i];
        if (x[i] > meta.dMaxX) meta.dMaxX = x[i];
        if (y[i] < meta.dMinY) meta.dMinY = y[i];
        if (y[i] > meta.dMaxY) meta.dMaxY = y[i];
    }

    meta.dOriginX = (meta.dMinX + meta.dMaxX) * 0.5;
    meta.dOriginY = (meta.dMinY + meta.dMaxY) * 0.5;

    if (!meta.strProjection.IsEmpty() && runtime.OSRNewSpatialReferenceProc &&
        runtime.OSRImportFromWktProc && runtime.OSRIsGeographicProc && runtime.OSRDestroySpatialReferenceProc)
    {
        void* hSrs = runtime.OSRNewSpatialReferenceProc(NULL);
        char* pszWkt = (char*)(LPCTSTR)meta.strProjection;
        if (hSrs && runtime.OSRImportFromWktProc(hSrs, &pszWkt) == 0 && runtime.OSRIsGeographicProc(hSrs))
        {
            const double metersPerDegree = 111319.49079327358;
            double latRad = meta.dOriginY * PI / 180.0;
            meta.dUnitScaleX = metersPerDegree * cos(latRad);
            meta.dUnitScaleY = metersPerDegree;
        }
        if (hSrs)
            runtime.OSRDestroySpatialReferenceProc(hSrs);
    }

    double width = (meta.dMaxX - meta.dMinX) * meta.dUnitScaleX;
    double height = (meta.dMaxY - meta.dMinY) * meta.dUnitScaleY;
    if (width < 0.0) width = -width;
    if (height < 0.0) height = -height;
    double extent = (width > height) ? width : height;
    meta.dSceneScale = (extent > 1500.0) ? (1500.0 / extent) : 1.0;
    meta.bValid = true;

    runtime.GDALCloseProc(pDataset);
    return true;
}

static void FillSceneInfoFromGeo(SceneObjectInfo* pInfo, const RasterGeoMetadata& meta)
{
    if (!pInfo || !meta.bValid)
        return;

    pInfo->bHasGeoReference = true;
    for (int i = 0; i < 6; ++i)
        pInfo->adfGeoTransform[i] = meta.adfGeoTransform[i];

    pInfo->dGeoOriginX = meta.dOriginX;
    pInfo->dGeoOriginY = meta.dOriginY;
    pInfo->dGeoOriginZ = meta.dOriginZ;
    pInfo->dGeoSceneScale = meta.dSceneScale;
        pInfo->dGeoUnitScaleX = meta.dUnitScaleX;
    pInfo->dGeoUnitScaleY = meta.dUnitScaleY;
    pInfo->dGeoMinX = meta.dMinX;
    pInfo->dGeoMinY = meta.dMinY;
    pInfo->dGeoMaxX = meta.dMaxX;
    pInfo->dGeoMaxY = meta.dMaxY;
    lstrcpyn(pInfo->szProjection, meta.strProjection, sizeof(pInfo->szProjection));
}
CSceneObject::CSceneObject(void)
{
}

CSceneObject::~CSceneObject(void)
{
}

/**
 * 加载三维模型文件（支持 osg、ive、obj、3ds、stl、fbx 等格式）
 * @param filename 模型文件路径（绝对或相对）
 * @return 成功返回 osg::Group 指针，失败返回 nullptr
 */
osg::Group* CSceneObject::LoadModel(CString filename)
{
    if (filename.IsEmpty())
    {
        OSG_WARN << "LoadModel: 文件名为空" << std::endl;
        return nullptr;
    }

    osg::Node* model = osgDB::readNodeFile(filename.GetBuffer(0));
    if (!model)
    {
        OSG_WARN << "LoadModel: 无法加载模型文件 '" << filename.GetBuffer(0) << "'" << std::endl;
        return nullptr;
    }

    osg::Group* pGroup = dynamic_cast<osg::Group*>(model);
    if (pGroup)
    {
        OSG_NOTICE << "LoadModel: 成功加载模型 '" << filename.GetBuffer(0) << "'" << std::endl;
        return pGroup;
    }

    // 如果读出来的不是 Group，就包一层 Group，避免返回空
    osg::Group* pRoot = new osg::Group;
    pRoot->addChild(model);

    OSG_NOTICE << "LoadModel: 成功加载模型（已自动封装为 Group）'" << filename.GetBuffer(0) << "'" << std::endl;
    return pRoot;
}


// 手动裁剪坐标，避免 std::min / std::max 与 Windows 宏冲突
static int ClampInt(int v, int low, int high)
{
    if (v < low)
        return low;
    if (v > high)
        return high;
    return v;
}

static float GetHeightValue(osg::Image* pImage, int x, int y)
{
    if (!pImage)
        return 0.0f;

    x = ClampInt(x, 0, pImage->s() - 1);
    y = ClampInt(y, 0, pImage->t() - 1);

    unsigned char* pData = pImage->data(x, y);
    if (!pData)
        return 0.0f;

    GLenum dataType = pImage->getDataType();

    if (dataType == GL_UNSIGNED_BYTE)
    {
        return static_cast<float>(pData[0]);
    }
    else if (dataType == GL_BYTE)
    {
        return static_cast<float>(*(reinterpret_cast<signed char*>(pData)));
    }
    else if (dataType == GL_UNSIGNED_SHORT)
    {
        return static_cast<float>(*(reinterpret_cast<unsigned short*>(pData)));
    }
    else if (dataType == GL_SHORT)
    {
        return static_cast<float>(*(reinterpret_cast<short*>(pData)));
    }
    else if (dataType == GL_UNSIGNED_INT)
    {
        return static_cast<float>(*(reinterpret_cast<unsigned int*>(pData)));
    }
    else if (dataType == GL_INT)
    {
        return static_cast<float>(*(reinterpret_cast<int*>(pData)));
    }
    else if (dataType == GL_FLOAT)
    {
        return *(reinterpret_cast<float*>(pData));
    }

    return static_cast<float>(pData[0]);
}

static osg::Vec3 ComputeVertexNormal(osg::Vec3Array* pVertices, int rows, int cols, int r, int c)
{
    if (!pVertices || rows < 2 || cols < 2)
        return osg::Vec3(0.0f, 0.0f, 1.0f);

    int leftC = (c > 0) ? (c - 1) : c;
    int rightC = (c < cols - 1) ? (c + 1) : c;
    int downR = (r > 0) ? (r - 1) : r;
    int upR = (r < rows - 1) ? (r + 1) : r;

    const osg::Vec3& left = (*pVertices)[r * cols + leftC];
    const osg::Vec3& right = (*pVertices)[r * cols + rightC];
    const osg::Vec3& down = (*pVertices)[downR * cols + c];
    const osg::Vec3& up = (*pVertices)[upR * cols + c];

    osg::Vec3 dx = right - left;
    osg::Vec3 dy = up - down;

    osg::Vec3 normal = dx ^ dy;   // 叉乘

    if (normal.length2() < 1e-6f)
        return osg::Vec3(0.0f, 0.0f, 1.0f);

    normal.normalize();
    return normal;
}

static bool IsNoDataHeight(float h)
{
    // 你的 DEM 里最明显的问题值是 -32767
    // 这里顺手把 NaN 和极端负值一起排掉
    if (h != h)   // NaN
        return true;

    if (h <= -32000.0f)
        return true;

    return false;
}

static float Clamp01(float t)
{
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
}

static osg::Vec4 HeightToColor(float t)
{
    t = Clamp01(t);

    // 分层颜色：低处深绿 -> 绿 -> 黄 -> 棕 -> 白
    if (t < 0.25f)
    {
        float k = t / 0.25f;
        return osg::Vec4(
            0.0f + 0.2f * k,
            0.35f + 0.35f * k,
            0.0f,
            1.0f);
    }
    else if (t < 0.5f)
    {
        float k = (t - 0.25f) / 0.25f;
        return osg::Vec4(
            0.2f + 0.4f * k,
            0.7f + 0.15f * k,
            0.0f,
            1.0f);
    }
    else if (t < 0.75f)
    {
        float k = (t - 0.5f) / 0.25f;
        return osg::Vec4(
            0.6f + 0.25f * k,
            0.85f - 0.25f * k,
            0.0f,
            1.0f);
    }
    else
    {
        float k = (t - 0.75f) / 0.25f;
        return osg::Vec4(
            0.85f + 0.15f * k,
            0.6f + 0.2f * k,
            0.3f + 0.7f * k,
            1.0f);
    }
}

static float Min4(float a, float b, float c, float d)
{
    float m = a;
    if (b < m) m = b;
    if (c < m) m = c;
    if (d < m) m = d;
    return m;
}

static float Max4(float a, float b, float c, float d)
{
    float m = a;
    if (b > m) m = b;
    if (c > m) m = c;
    if (d > m) m = d;
    return m;
}

static float ClampValue(float v, float low, float high)
{
    if (v < low) return low;
    if (v > high) return high;
    return v;
}

static bool HasInvalidNeighbor(const std::vector<unsigned char>& mask, int rows, int cols, int r, int c)
{
    // 贴边的一圈直接视为边界危险区
    if (r <= 0 || r >= rows - 1 || c <= 0 || c >= cols - 1)
        return true;

    for (int dr = -1; dr <= 1; ++dr)
    {
        for (int dc = -1; dc <= 1; ++dc)
        {
            int rr = r + dr;
            int cc = c + dc;
            if (mask[rr * cols + cc] == 0)
                return true;
        }
    }

    return false;
}

osg::Group* CSceneObject::CreateTerrain(CString& strDemName, CString& strImageName, SceneObjectInfo* pInfo)
{
    if (strDemName.IsEmpty())
    {
        OSG_WARN << "CreateTerrain: DEM 文件名为空" << std::endl;
        return nullptr;
    }

    osg::ref_ptr<osg::Image> pDemImage = osgDB::readImageFile(strDemName.GetBuffer(0));
    if (!pDemImage)
    {
        OSG_WARN << "CreateTerrain: 无法读取 DEM 文件 '" << strDemName.GetBuffer(0) << "'" << std::endl;
        return nullptr;
    }

    int srcCols = pDemImage->s();
    int srcRows = pDemImage->t();

    RasterGeoMetadata geoMeta;
    bool bHasGeoMetadata = ReadRasterGeoMetadata(strDemName, srcCols, srcRows, geoMeta);
    if (bHasGeoMetadata)
    {
        OSG_NOTICE << "CreateTerrain: GDAL geo extent = [" << geoMeta.dMinX << ", " << geoMeta.dMinY << "] - ["
            << geoMeta.dMaxX << ", " << geoMeta.dMaxY << "], sceneScale = " << geoMeta.dSceneScale << std::endl;
    }

    if (srcCols < 2 || srcRows < 2)
    {
        OSG_WARN << "CreateTerrain: DEM 尺寸过小，无法构建地形" << std::endl;
        return nullptr;
    }

    // ---------------------------
    // 1. 对大尺寸 DEM 做降采样
    // ---------------------------
    int maxDim = (srcCols > srcRows) ? srcCols : srcRows;
    int sampleStep = 1;

    while ((maxDim / sampleStep) > 512)
    {
        sampleStep *= 2;
    }

    int cols = (srcCols - 1) / sampleStep + 1;
    int rows = (srcRows - 1) / sampleStep + 1;

    if (cols < 2 || rows < 2)
    {
        OSG_WARN << "CreateTerrain: 降采样后尺寸过小" << std::endl;
        return nullptr;
    }

    OSG_NOTICE << "CreateTerrain: 原始DEM尺寸 = " << srcCols << " x " << srcRows << std::endl;
    OSG_NOTICE << "CreateTerrain: 降采样步长 = " << sampleStep << std::endl;
    OSG_NOTICE << "CreateTerrain: 建模网格尺寸 = " << cols << " x " << rows << std::endl;
    OSG_NOTICE << "CreateTerrain: 数据类型 = " << pDemImage->getDataType() << std::endl;

    // ---------------------------
// 2. 采样高程，记录有效点
// ---------------------------
    std::vector<float> heights(rows * cols, 0.0f);
    std::vector<unsigned char> validMask(rows * cols, 0);
    std::vector<float> validHeights;
    validHeights.reserve(rows * cols);

    float minH = 0.0f;
    float maxH = 0.0f;
    bool bFirstValid = true;
    int invalidCount = 0;

    for (int r = 0; r < rows; ++r)
    {
        int srcR = r * sampleStep;
        if (srcR >= srcRows)
            srcR = srcRows - 1;

        for (int c = 0; c < cols; ++c)
        {
            int srcC = c * sampleStep;
            if (srcC >= srcCols)
                srcC = srcCols - 1;

            float h = GetHeightValue(pDemImage.get(), srcC, srcRows - 1 - srcR);
            int idx = r * cols + c;

            if (IsNoDataHeight(h))
            {
                heights[idx] = 0.0f;
                validMask[idx] = 0;
                invalidCount++;
                continue;
            }

            heights[idx] = h;
            validMask[idx] = 1;
            validHeights.push_back(h);

            if (bFirstValid)
            {
                minH = maxH = h;
                bFirstValid = false;
            }
            else
            {
                if (h < minH) minH = h;
                if (h > maxH) maxH = h;
            }
        }
    }

    if (bFirstValid || validHeights.empty())
    {
        OSG_WARN << "CreateTerrain: 没有读取到任何有效高程值" << std::endl;
        return nullptr;
    }

    // 用 1% ~ 99% 分位数做稳健裁剪，去掉边缘异常高值/低值
    std::sort(validHeights.begin(), validHeights.end());

    int nValid = (int)validHeights.size();
    int lowIndex = (int)(nValid * 0.01f);
    int highIndex = (int)(nValid * 0.99f);

    if (lowIndex < 0) lowIndex = 0;
    if (highIndex >= nValid) highIndex = nValid - 1;
    if (highIndex < lowIndex) highIndex = lowIndex;

    float clipMinH = validHeights[lowIndex];
    float clipMaxH = validHeights[highIndex];

    if (clipMaxH - clipMinH < 1e-6f)
    {
        clipMinH = minH;
        clipMaxH = maxH;
    }

    // 把超出稳健范围的点也视作异常点
    int clippedCount = 0;
    for (size_t i = 0; i < heights.size(); ++i)
    {
        if (!validMask[i])
            continue;

        if (heights[i] < clipMinH || heights[i] > clipMaxH)
        {
            validMask[i] = 0;
            clippedCount++;
        }
    }

    float heightRange = clipMaxH - clipMinH;
    if (heightRange < 1e-6f)
        heightRange = 1.0f;

    OSG_NOTICE << "CreateTerrain: 原始有效高程范围 = [" << minH << ", " << maxH << "]" << std::endl;
    OSG_NOTICE << "CreateTerrain: 裁剪后高程范围 = [" << clipMinH << ", " << clipMaxH << "]" << std::endl;
    OSG_NOTICE << "CreateTerrain: 无效高程点数量 = " << invalidCount << std::endl;
    OSG_NOTICE << "CreateTerrain: 分位数裁剪点数量 = " << clippedCount << std::endl;

    // ---------------------------
// 2.5 再做一次边界腐蚀：把靠近无效区的点整体去掉
// 防止边缘残留点继续组成“漂浮边带”和“幕墙”
// ---------------------------
    int erodedCount = 0;

    // 做两轮腐蚀，基本够用；如果你觉得裁得太狠，可以改成 1 轮
    for (int pass = 0; pass < 2; ++pass)
    {
        std::vector<unsigned char> nextMask = validMask;

        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int idx = r * cols + c;
                if (!validMask[idx])
                    continue;

                if (HasInvalidNeighbor(validMask, rows, cols, r, c))
                {
                    nextMask[idx] = 0;
                }
            }
        }

        for (size_t i = 0; i < validMask.size(); ++i)
        {
            if (validMask[i] == 1 && nextMask[i] == 0)
                erodedCount++;
        }

        validMask.swap(nextMask);
    }

    OSG_NOTICE << "CreateTerrain: 边界腐蚀去除点数量 = " << erodedCount << std::endl;

    int finalValidCount = 0;
    for (size_t i = 0; i < validMask.size(); ++i)
    {
        if (validMask[i])
            finalValidCount++;
    }

    OSG_NOTICE << "CreateTerrain: 最终有效点数量 = " << finalValidCount << std::endl;

    if (finalValidCount <= 0)
    {
        OSG_WARN << "CreateTerrain: 边界腐蚀后没有剩余有效点" << std::endl;
        return nullptr;
    }


    // ---------------------------
    // 3. 自动设置平面尺度和高程拉伸
    // ---------------------------
    float xyScale = 5.0f;

    // 比之前拉得更明显一些，便于课程演示
    float terrainWidth = (cols - 1) * xyScale;
    float targetZRange = terrainWidth * 0.35f;
    if (targetZRange < 30.0f)
        targetZRange = 30.0f;

    float zScale = targetZRange / heightRange;
    if (bHasGeoMetadata)
    {
        geoMeta.dOriginZ = clipMinH;
        zScale = (float)geoMeta.dSceneScale;
    }

    OSG_NOTICE << "CreateTerrain: xyScale = " << xyScale << ", zScale = " << zScale << std::endl;

    // ---------------------------
    // 4. 创建顶点/纹理/法向量/颜色/索引
    // ---------------------------
    osg::ref_ptr<osg::Vec3Array> pVertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> pTexCoords = new osg::Vec2Array;
    osg::ref_ptr<osg::Vec3Array> pNormals = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> pColors = new osg::Vec4Array;
    osg::ref_ptr<osg::DrawElementsUInt> pIndices = new osg::DrawElementsUInt(GL_TRIANGLES);

    pVertices->reserve(rows * cols);
    pTexCoords->reserve(rows * cols);
    pNormals->reserve(rows * cols);
    pColors->reserve(rows * cols);

    float halfWidth = (cols - 1) * xyScale * 0.5f;
    float halfHeight = (rows - 1) * xyScale * 0.5f;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            int idx = r * cols + c;

            float h = heights[idx];
            if (!validMask[idx])
            {
                h = clipMinH;
            }
            else
            {
                h = ClampValue(h, clipMinH, clipMaxH);
            }

            float zOffset = -5.0f;   // 让地形底部靠近绿色背景板
            float z = (h - clipMinH) * zScale + zOffset;

            float x = c * xyScale - halfWidth;
            float y = r * xyScale - halfHeight;
            if (bHasGeoMetadata)
            {
                int srcC = c * sampleStep;
                int srcR = r * sampleStep;
                if (srcC >= srcCols) srcC = srcCols - 1;
                if (srcR >= srcRows) srcR = srcRows - 1;

                double geoX = 0.0;
                double geoY = 0.0;
                PixelToGeo(geoMeta.adfGeoTransform, (double)srcC, (double)srcR, geoX, geoY);
                x = (float)((geoX - geoMeta.dOriginX) * geoMeta.dUnitScaleX * geoMeta.dSceneScale);
                y = (float)((geoY - geoMeta.dOriginY) * geoMeta.dUnitScaleY * geoMeta.dSceneScale);
            }

            pVertices->push_back(osg::Vec3(x, y, z));

            float u = (cols > 1) ? (float)c / (float)(cols - 1) : 0.0f;
            float v = (rows > 1) ? 1.0f - (float)r / (float)(rows - 1) : 0.0f;
            pTexCoords->push_back(osg::Vec2(u, v));

            float t = (h - clipMinH) / heightRange;
            pColors->push_back(HeightToColor(t));
        }
    }

    // 法向量
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            osg::Vec3 normal = ComputeVertexNormal(pVertices.get(), rows, cols, r, c);
            pNormals->push_back(normal);
        }
    }

    // 三角网
    float jumpThreshold = heightRange * 0.25f;
    if (jumpThreshold < 80.0f)
        jumpThreshold = 80.0f;

    for (int r = 0; r < rows - 1; ++r)
    {
        for (int c = 0; c < cols - 1; ++c)
        {
            unsigned int i0 = r * cols + c;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = (r + 1) * cols + c;
            unsigned int i3 = i2 + 1;

            // 只要这个网格块里有无效点，就不建三角形
            if (!validMask[i0] || !validMask[i1] || !validMask[i2] || !validMask[i3])
                continue;

            float h0 = ClampValue(heights[i0], clipMinH, clipMaxH);
            float h1 = ClampValue(heights[i1], clipMinH, clipMaxH);
            float h2 = ClampValue(heights[i2], clipMinH, clipMaxH);
            float h3 = ClampValue(heights[i3], clipMinH, clipMaxH);

            // 如果四个角之间高差过于夸张，也跳过，防止形成“尖刺”和“幕墙”
            float localMin = Min4(h0, h1, h2, h3);
            float localMax = Max4(h0, h1, h2, h3);
            if ((localMax - localMin) > jumpThreshold)
                continue;

            pIndices->push_back(i0);
            pIndices->push_back(i2);
            pIndices->push_back(i1);

            pIndices->push_back(i1);
            pIndices->push_back(i2);
            pIndices->push_back(i3);
        }
    }

    osg::ref_ptr<osg::Geometry> pGeometry = new osg::Geometry;
    pGeometry->setVertexArray(pVertices.get());
    pGeometry->setTexCoordArray(0, pTexCoords.get());
    pGeometry->setNormalArray(pNormals.get());
    pGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    pGeometry->setColorArray(pColors.get());
    pGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    pGeometry->addPrimitiveSet(pIndices.get());

    pGeometry->setUseDisplayList(false);
    pGeometry->setUseVertexBufferObjects(true);

    osg::ref_ptr<osg::Geode> pGeode = new osg::Geode;
    pGeode->addDrawable(pGeometry.get());

    bool bTextureLoaded = false;

    // ---------------------------
    // 5. 可选叠加影像
    // ---------------------------
    if (!strImageName.IsEmpty())
    {
        osg::ref_ptr<osg::Image> pTexImage = osgDB::readImageFile(strImageName.GetBuffer(0));
        if (pTexImage && pTexImage->s() > 0 && pTexImage->t() > 0)
        {
            OSG_NOTICE << "CreateTerrain: 影像读取成功 = "
                << strImageName.GetBuffer(0)
                << ", 尺寸 = " << pTexImage->s() << " x " << pTexImage->t()
                << ", 像素格式 = " << pTexImage->getPixelFormat()
                << std::endl;

            osg::ref_ptr<osg::Texture2D> pTexture = new osg::Texture2D;
            pTexture->setImage(pTexImage.get());
            pTexture->setDataVariance(osg::Object::STATIC);
            pTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            pTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            pTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            pTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            pTexture->setResizeNonPowerOfTwoHint(false);

            osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
            pStateSet->setTextureAttributeAndModes(0, pTexture.get(), osg::StateAttribute::ON);

            // 关键 1：强制纹理直接显示，不再和高程颜色做调制混合
            osg::ref_ptr<osg::TexEnv> pTexEnv = new osg::TexEnv;
            pTexEnv->setMode(osg::TexEnv::REPLACE);
            pStateSet->setTextureAttributeAndModes(0, pTexEnv.get(), osg::StateAttribute::ON);

            // 关键 2：贴图成功时关闭光照，先保证影像绝对可见
            pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

            // 关键 3：关闭背面剔除，避免因为面朝向问题导致看不到纹理
            pStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

            // 关键 4：把几何颜色改成纯白，彻底避免顶点颜色干扰贴图显示
            osg::ref_ptr<osg::Vec4Array> pWhiteColors = new osg::Vec4Array;
            pWhiteColors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
            pGeometry->setColorArray(pWhiteColors.get());
            pGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

            bTextureLoaded = true;
        }
        else
        {
            OSG_WARN << "CreateTerrain: 影像读取失败，继续以彩色高程方式显示地形" << std::endl;
        }
    }

    // 没有贴图时，关闭光照，保证高程颜色一定明显可见
    if (!bTextureLoaded)
    {
        osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
        pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        OSG_NOTICE << "CreateTerrain: 当前未使用影像贴图，采用彩色高程显示" << std::endl;
    }

    osg::Group* pTerrain = new osg::Group;
    pTerrain->addChild(pGeode.get());

    FillSceneInfoFromGeo(pInfo, geoMeta);

    OSG_NOTICE << "CreateTerrain: 地形创建成功" << std::endl;
    return pTerrain;
}