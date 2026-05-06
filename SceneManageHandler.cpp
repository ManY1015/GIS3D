#include "pch.h"
#include "SceneManageHandler.h"
#include <osg/Notify>
#include <osg/MatrixTransform>
#include <osg/Material>
#include <osg/BlendFunc>
#include <vector>

static CString GetNodeLayerName(osg::Node* pNode)
{
    if (!pNode)
        return _T("");

    std::string sName = pNode->getName();
    if (sName.empty())
        return _T("");

    std::string sNew = UTF8ToGB2132(sName);
    CString strName = sNew.c_str();
    if (strName.IsEmpty())
        strName = sName.c_str();
    return strName;
}

static osg::Transform* FindLayerTransform(osg::Group* pRoot, const CString& strLayerName)
{
    if (!pRoot || strLayerName.IsEmpty())
        return NULL;

    int nLevel = pRoot->getNumChildren();
    for (int i = 0; i < nLevel; ++i)
    {
        osg::Transform* pTransform = dynamic_cast<osg::Transform*>(pRoot->getChild(i));
        if (!pTransform)
            continue;

        CString strName = GetNodeLayerName(pTransform);
        CString strRaw = pTransform->getName().c_str();
        if (strLayerName.CompareNoCase(strName) == 0 || strLayerName.CompareNoCase(strRaw) == 0)
            return pTransform;
    }
    return NULL;
}

static void ApplyOpacity(osg::Node* pNode, float fOpacity)
{
    if (!pNode)
        return;

    if (fOpacity < 0.0f) fOpacity = 0.0f;
    if (fOpacity > 1.0f) fOpacity = 1.0f;

    osg::StateSet* pStateSet = pNode->getOrCreateStateSet();
    if (fOpacity < 0.999f)
    {
        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, fOpacity));
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, fOpacity));
        pStateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        pStateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
        pStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        pStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
    else
    {
        pStateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
        pStateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    }

    osg::Group* pGroup = pNode->asGroup();
    if (!pGroup)
        return;

    for (unsigned int i = 0; i < pGroup->getNumChildren(); ++i)
        ApplyOpacity(pGroup->getChild(i), fOpacity);
}

static void ReorderLayer(osg::Group* pRoot, const CString& strLayerName, int nNewPos)
{
    if (!pRoot)
        return;

    std::vector< osg::ref_ptr<osg::Node> > layers;
    for (unsigned int i = 0; i < pRoot->getNumChildren(); ++i)
    {
        osg::Transform* pTransform = dynamic_cast<osg::Transform*>(pRoot->getChild(i));
        if (pTransform && !GetNodeLayerName(pTransform).IsEmpty())
            layers.push_back(pTransform);
    }

    if (layers.empty())
        return;

    int nOldPos = -1;
    for (int i = 0; i < (int)layers.size(); ++i)
    {
        CString strName = GetNodeLayerName(layers[i].get());
        CString strRaw = layers[i]->getName().c_str();
        if (strLayerName.CompareNoCase(strName) == 0 || strLayerName.CompareNoCase(strRaw) == 0)
        {
            nOldPos = i;
            break;
        }
    }

    if (nOldPos < 0)
        return;

    if (nNewPos < 0) nNewPos = 0;
    if (nNewPos >= (int)layers.size()) nNewPos = (int)layers.size() - 1;

    osg::ref_ptr<osg::Node> pMoveNode = layers[nOldPos];
    layers.erase(layers.begin() + nOldPos);
    layers.insert(layers.begin() + nNewPos, pMoveNode);

    for (unsigned int i = 0; i < layers.size(); ++i)
        pRoot->removeChild(layers[i].get());

    for (unsigned int i = 0; i < layers.size(); ++i)
        pRoot->addChild(layers[i].get());
}

static osg::Matrixd CreateGeoModelMatrix(const osg::BoundingSphere& bs)
{
    if (!IsLikelyGeoCoordinate(bs.center().x(), bs.center().y()))
        return osg::Matrixd::identity();

    return osg::Matrixd::translate(-g_dGeoOriginX, -g_dGeoOriginY, -g_dGeoOriginZ) *
        osg::Matrixd::scale(g_dGeoUnitScaleX * g_dGeoSceneScale, g_dGeoUnitScaleY * g_dGeoSceneScale, g_dGeoSceneScale);
}
CSceneManageHandler::CSceneManageHandler(HWND hWnd,osg::ref_ptr<osg::Group> pRoot,osgViewer::Viewer *mViewer)
{
	m_pRoot = pRoot;
	m_pViewer = mViewer;
}


CSceneManageHandler::~CSceneManageHandler(void)
{
}


//向场景中添加光源
osg::ref_ptr<osg::LightSource> CSceneManageHandler::createLight(osg::BoundingSphere bs)
{
	//开启光�?
	//lightRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
	//lightRoot->getOrCreateStateSet()->setMode(GL_LIGHT0, osg::StateAttribute::ON);

	//计算包围�?
	//osg::BoundingSphere bs;
	//node->computeBound();
	//bs = node->getBound();

	//创建一个Light对象
	osg::ref_ptr<osg::Light> light = new osg::Light();
	light->setLightNum(0);
	//设置方向
	light->setDirection(osg::Vec3(0.0f, 0.0f, -1.0f));
	//设置位置
	light->setPosition(osg::Vec4(bs.center().x(), bs.center().y(), bs.center().z() + bs.radius(), 1.0f));
	//设置环境光的颜色
	light->setAmbient(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	//设置散射光颜�?
	light->setDiffuse(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

	//设置恒衰减指�?
	light->setConstantAttenuation(1.0f);
	//设置线形衰减指数
	light->setLinearAttenuation(0.0f);
	//设置二次方衰减指�?
	light->setQuadraticAttenuation(0.0f);

	//创建光源
	osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource();
	lightSource->setLight(light.get());


	return lightSource;

}

bool CSceneManageHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa)
{
    if(ea.getEventType()==osgGA::GUIEventAdapter::USER)
    {
        osg::notify(osg::NOTICE) << "[BG-DIAG] SceneManageHandler received USER event" << std::endl;
        const SceneManageInfo* info =dynamic_cast<const SceneManageInfo*>( ea.getUserData());
        if(!info)
            return false;

        CString strLayerName = info->m_info.szLayerName;
        osg::Transform* pLayer = FindLayerTransform(m_pRoot.get(), strLayerName);

        if (info->m_info.nMsg == SCENE_MSG_VISIBLE)
        {
            if (pLayer)
                pLayer->setNodeMask(info->m_info.bVisible ? 0xffffffff : 0x0);
            aa.requestRedraw();
            return false;
        }
        else if (info->m_info.nMsg == SCENE_MSG_OPACITY)
        {
            if (pLayer)
                ApplyOpacity(pLayer, info->m_info.fOpacity);
            aa.requestRedraw();
            return false;
        }
        else if (info->m_info.nMsg == SCENE_MSG_DELETE_LAYER)
        {
            if (pLayer)
                m_pRoot->removeChild(pLayer);
            aa.requestRedraw();
            return false;
        }
        else if (info->m_info.nMsg == SCENE_MSG_ORDER)
        {
            ReorderLayer(m_pRoot.get(), strLayerName, info->m_info.nNewPos);
            aa.requestRedraw();
            return false;
        }
        else if (info->m_info.nMsg == SCENE_MSG_RENAME_LAYER)
        {
            if (pLayer && info->m_info.szNewLayerName[0] != '\0')
            {
                // ֻ���³�����ڵ����ƣ���������ͼ�����ṹ����Ⱦ���̲��䡣
                CStringA strNewName(info->m_info.szNewLayerName);
                pLayer->setName(std::string(strNewName));
            }
            aa.requestRedraw();
            return false;
        }
        osg::Group* pLevel = info->m_info.pObject;
        if (!pLevel)
            return false;

        int i;
        if(pLayer)//已存在层，目标加在后�?
        {
            int nObject = pLevel->getNumChildren();
            for (i=0;i<nObject;i++)
                pLayer->addChild(pLevel->getChild(i));
        }
        else
        {
            osg::Vec3 Center ,eye;
            osg::BoundingSphere bs = pLevel->getBound();
            Center = bs.center();

            Point3 ptDelta;
            if(g_bInit)
            {
                g_ptSceneCenter.x = Center.x();
                g_ptSceneCenter.y = Center.y();
                g_ptSceneCenter.z = Center.z();
                ptDelta.x = 0;
                ptDelta.y = 0;
                ptDelta.z = 0;
                g_bInit = false;
            }

            Center.set(0.0f, 0.0f, 0.0f);

            osg::MatrixTransform* transform = new osg::MatrixTransform;
            transform->setDataVariance( osg::Object::DYNAMIC );

            osg::Matrixd matrix = osg::Matrixd::translate(Center);
            // 修复批量模型被错误套用地理配准矩阵后飞出视域的问题：
            // 仅当当前对象自身携带地理参考信息时才应用地理配准矩阵，普通模型保持原始局部坐标�?
            if (g_bGeoReferenceReady && info->m_info.bHasGeoReference && strLayerName.CompareNoCase(_T("地形")) != 0)
                matrix = CreateGeoModelMatrix(bs);
            transform->setMatrix(matrix);

            int nObject = pLevel->getNumChildren();
            for (i=0;i<nObject;i++)
                transform->addChild(pLevel->getChild(i));

            osg::notify(osg::NOTICE) << "[BG-DIAG] add layer name=" << info->m_info.szLayerName << " pObject=" << (info->m_info.pObject ? "YES" : "NO") << " rootChildBefore=" << m_pRoot->getNumChildren() << std::endl;
            transform->setName(info->m_info.szLayerName);
            m_pRoot->addChild(transform);
            osg::notify(osg::NOTICE) << "[BG-DIAG] root child after add=" << m_pRoot->getNumChildren() << std::endl;

            if (info->m_info.bHasGeoReference)
                SetSceneGeoReference(info->m_info);

            osg::ref_ptr<osg::LightSource> Light = createLight(m_pRoot->getBound());
            m_pRoot->getOrCreateStateSet()->setMode(GL_LIGHT0, osg::StateAttribute::ON);
            m_pRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
            m_pRoot->addChild(Light.get());
        }
        aa.requestRedraw();
    }
    return false;
}
