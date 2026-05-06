#pragma once
#include <osg/Switch>
#include <osg/TextureCubeMap>

#include <osgText/Text>

#include "SphereSegment.h"
#include <osg/Program>
#include <osg/Uniform>
#include <osg/TextureCubeMap>
#include <osg/PositionAttitudeTransform>


enum DrawMask
{
    CAST_SHADOW             = (0x1<<30),
    RECEIVE_SHADOW          = (0x1<<29),
};




class SkyDome : public SphereSegment
{
public:
	SkyDome( void );
	SkyDome( const SkyDome& copy, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
	SkyDome( float radius, unsigned int longSteps, unsigned int latSteps, osg::TextureCubeMap* cubemap );

protected:
	~SkyDome(void);

public:
	void setupStateSet( osg::TextureCubeMap* cubemap );
	void create( float radius, unsigned int latSteps, unsigned int longSteps, osg::TextureCubeMap* cubemap );

	inline void setCubeMap( osg::TextureCubeMap* cubemap ){
		getOrCreateStateSet()->setTextureAttributeAndModes( 0, cubemap, osg::StateAttribute::ON );
	}

private:
	osg::ref_ptr<osg::Program> createShader(void);

};

//使用回调实现旋转动画
//定义一个不断执行旋转动作的回调，它操作空间变换节点PositionAttitudeTransform不断改变旋转角度，实现动画效果。
class CRotateCallback :public osg::NodeCallback//继承回调基类
{
public:
	CRotateCallback(osg::Vec3 Axis) :_rotateZ(0.0), axis(Axis) ,move(0){}
	~CRotateCallback(void){};

	virtual void operator()(osg::Node*node, osg::NodeVisitor*nv)//重构执行函数
	{
		

		osg::PositionAttitudeTransform* pat = //空间变换节点，暂时先放一下，之后再学习。
			dynamic_cast<osg::PositionAttitudeTransform*>(node);

		if (pat)
		{	
			//pat->setPivotPoint(pos);
			osg::Quat quat(osg::DegreesToRadians(_rotateZ), axis);//设置四元数，沿Z轴转_rotateZ度
			pat->setAttitude(quat);
			_rotateZ += 0.01;
		}
		traverse(node, nv);//向下一级节点推进访问器。
	}
protected:
	double _rotateZ;

	osg::Vec3 axis;

	osg::Vec3 pos;
	int move;//平移标记  0-未平移，  1-以平移
public:
	void setTranslate(osg::Vec3 trans ){ pos = trans; }

	void setAxis(osg::Vec3 Axis) { axis = Axis; }
};

class CScene : public osg::Referenced
{
public:
    enum SCENE_TYPE{ CLEAR, DUSK, CLOUDY };

private:
    SCENE_TYPE _sceneType;
    bool _useVBO;

    osg::ref_ptr<osgText::Text> _modeText;
    osg::ref_ptr<osg::Group> _scene;

    osg::ref_ptr<osg::TextureCubeMap> _cubemap;
    osg::ref_ptr<SkyDome> _skyDome;

    std::vector<std::string> _cubemapDirs;
    std::vector<osg::Vec4f>  _lightColors;
    std::vector<osg::Vec4f>  _fogColors;
    std::vector<osg::Vec3f>  _underwaterAttenuations;
    std::vector<osg::Vec4f>  _underwaterDiffuse;

    osg::ref_ptr<osg::Light> _light;

    std::vector<osg::Vec3f>  _sunPositions;
    std::vector<osg::Vec4f>  _sunDiffuse;
    std::vector<osg::Vec4f>  _waterFogColors;

    osg::ref_ptr<osg::Switch> _islandSwitch;

public:
    CScene( const std::string& strPath = "");

    void build(const std::string& strPath);

    void changeScene( SCENE_TYPE type );

    osg::ref_ptr<osg::TextureCubeMap> loadCubeMapTextures( const std::string& dir );

    osg::Geode* sunDebug( const osg::Vec3f& position );

    inline osg::Vec4f intColor(unsigned r, unsigned g, unsigned b, unsigned a = 255 )
    {
        float div = 1.f/255.f;
        return osg::Vec4f( div*(float)r, div*(float)g, div*float(b), div*(float)a );
    }

 
    inline osg::Group* getScene(void){
        return _scene.get();
    }

	std::string mStrPath;
    osg::Light* getLight() { return _light.get(); }
};