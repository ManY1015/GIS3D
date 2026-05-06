#include "pch.h"
#include "Scene.h"
#include "ScopedTimer.h"
#include "gis3d.h"
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/Program>
#include <osg/LightSource>

#include <osgUtil/CullVisitor>
#include <osg/MatrixTransform>
#include <osgdb/ReadFile>
#include <osg/Depth>

// ----------------------------------------------------
//               Camera Track Callback
// ----------------------------------------------------

class CameraTrackCallback: public osg::NodeCallback
{
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if( nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR )
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
            osg::Vec3f centre,up,eye;
            // get MAIN camera eye,centre,up
            cv->getRenderStage()->getCamera()->getViewMatrixAsLookAt(eye,centre,up);
            // update position
            osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(node);
            mt->setMatrix(osg::Matrix::translate(eye.x(), eye.y(), eye.z()));
        }

        traverse(node, nv); 
    }
};


SkyDome::SkyDome( void )
{

}

SkyDome::SkyDome( const SkyDome& copy, const osg::CopyOp& copyop ):
SphereSegment( copy, copyop )
{

}

SkyDome::SkyDome(float radius, unsigned int longSteps, unsigned int latSteps, osg::TextureCubeMap* cubemap)
{
    compute(radius, longSteps, latSteps, 0.f, 180.f, 0.f, 360.f);
    setupStateSet(cubemap);
}

SkyDome::~SkyDome(void)
{
}

void SkyDome::create(float radius, unsigned int latSteps, unsigned int longSteps, osg::TextureCubeMap* cubemap)
{
    compute(radius, longSteps, latSteps, 0.f, 180.f, 0.f, 360.f);
    setupStateSet(cubemap);
}

void SkyDome::setupStateSet(osg::TextureCubeMap* cubemap)
{
    osg::StateSet* ss = new osg::StateSet;

    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    ss->setTextureAttributeAndModes(0, cubemap, osg::StateAttribute::ON);
    ss->setAttributeAndModes(createShader().get(), osg::StateAttribute::ON);
    ss->addUniform(new osg::Uniform("uEnvironmentMap", 0));

    setStateSet(ss);
}

osg::ref_ptr<osg::Program> SkyDome::createShader(void)
{
	osg::ref_ptr<osg::Program> program = new osg::Program;

	// Do not use shaders if they were globally disabled.
	char vertexSource[]=
		"varying vec3 vTexCoord;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = ftransform();\n"
		"    vTexCoord = gl_Vertex.xyz;\n"
		"}\n";

	char fragmentSource[]=
		"uniform samplerCube uEnvironmentMap;\n"
		"varying vec3 vTexCoord;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"   vec3 texcoord = vec3(vTexCoord.x, vTexCoord.y, -vTexCoord.z);\n"
		"   gl_FragData[0] = textureCube( uEnvironmentMap, texcoord.xzy );\n"
		"   gl_FragData[0].a = 1.0;\n"
		"   gl_FragData[1] = vec4(0.0);\n"
		"}\n";

	program->setName( "sky_dome_shader" );
	program->addShader(new osg::Shader(osg::Shader::VERTEX,   vertexSource));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragmentSource));

	return program;
}

// ----------------------------------------------------
//                       Scene 
// ----------------------------------------------------

CScene::CScene( 
    const std::string& strPath)
{
    _sceneType = CLOUDY;//CLEAR;

    _cubemapDirs.push_back( "sky_clear" );
    _cubemapDirs.push_back( "sky_dusk" );
    _cubemapDirs.push_back( "sky_fair_cloudy" );

    _fogColors.push_back( intColor( 199,226,255 ) );
    _fogColors.push_back( intColor( 244,228,179 ) );
    _fogColors.push_back( intColor( 172,224,251 ) );

    _waterFogColors.push_back( intColor(27,57,109) );
    _waterFogColors.push_back( intColor(44,69,106 ) );
    _waterFogColors.push_back( intColor(84,135,172 ) );

    _underwaterAttenuations.push_back( osg::Vec3f(0.015f, 0.0075f, 0.005f) );
    _underwaterAttenuations.push_back( osg::Vec3f(0.015f, 0.0075f, 0.005f) );
    _underwaterAttenuations.push_back( osg::Vec3f(0.008f, 0.003f, 0.002f) );

    _underwaterDiffuse.push_back( intColor(27,57,109) );
    _underwaterDiffuse.push_back( intColor(44,69,106) );
    _underwaterDiffuse.push_back( intColor(84,135,172) );

    _lightColors.push_back( intColor( 105,138,174 ) );
    _lightColors.push_back( intColor( 105,138,174 ) );
    _lightColors.push_back( intColor( 105,138,174 ) );

    _sunPositions.push_back( osg::Vec3f(326.573, 1212.99 ,1275.19) );
    _sunPositions.push_back( osg::Vec3f(520.f, 1900.f, 550.f ) );
    _sunPositions.push_back( osg::Vec3f(-1056.89f, -771.886f, 1221.18f ) );

    _sunDiffuse.push_back( intColor( 191, 191, 191 ) );
    _sunDiffuse.push_back( intColor( 251, 251, 161 ) );
    _sunDiffuse.push_back( intColor( 191, 191, 191 ) );

    build(strPath);
}

void CScene::build(const std::string& strPath)
{
    {
        ScopedTimer buildSceneTimer("Building scene... \n", osg::notify(osg::NOTICE));
        osg::notify(osg::NOTICE) << "[BG-DIAG] Scene init start" << std::endl;
        BG_DIAG_LOG("[BG-DIAG] Scene init start");
		mStrPath = strPath;
        _scene = new osg::Group; 

        {
            ScopedTimer cubemapTimer("  . Loading cubemaps: ", osg::notify(osg::NOTICE));
            _cubemap = loadCubeMapTextures( _cubemapDirs[_sceneType] );
        }

            // create sky dome and add to ocean scene
            // set masks so it appears in reflected scene and normal scene
            // 保持初始场景环境尺度，避免破坏启动背景渲染。
            _skyDome = new SkyDome( 1900.f, 16, 16, _cubemap.get() );
            osg::notify(osg::NOTICE) << "[BG-DIAG] sky dome created radius=1900" << std::endl;
            BG_DIAG_LOG("[BG-DIAG] sky dome created radius=1900");
//                                    _oceanScene->getNormalSceneMask()    | 
//                                    _oceanScene->getRefractedSceneMask());

            // add a pat to track the camera
            osg::MatrixTransform* transform = new osg::MatrixTransform;
            transform->setDataVariance( osg::Object::DYNAMIC );

			//////////////////////////////////////////////////////////////////////////
			osg::ref_ptr<osg::PositionAttitudeTransform> pat =new osg::PositionAttitudeTransform;
			pat->setDataVariance(osg::Object::DYNAMIC);
			//pat->setPosition(BoundCenter);
			pat->addChild(_skyDome.get());
			CRotateCallback *rotate=new  CRotateCallback(osg::Vec3(0,0,1.0));
			rotate->setTranslate(osg::Vec3f(0.f, 0.f, 0.f) );
			pat->addUpdateCallback(rotate);
			//////////////////////////////////////////////////////////////////////////
			
            transform->setMatrix( osg::Matrixf::translate( osg::Vec3f(0.f, 0.f, 0.f) ));
            transform->setCullCallback( new CameraTrackCallback );

            transform->addChild( pat.get() );

            osg::notify(osg::NOTICE) << "[BG-DIAG] ground creation code reached" << std::endl;
            BG_DIAG_LOG("[BG-DIAG] ground creation code reached");
            osg::Geode* geode = new osg::Geode();
            osg::StateSet* stateset = new osg::StateSet();

            std::string filenames = mStrPath;
            filenames += "\\resources\\ground5.jpg";
            osg::notify(osg::NOTICE) << "[BG-DIAG] ground texture path=" << filenames << std::endl;
            BG_DIAG_LOG("[BG-DIAG] ground texture path=" << filenames);

            osg::ref_ptr<osg::Image> image = osgDB::readImageFile(filenames);
            if (image)
            {
                osg::Texture2D* texture = new osg::Texture2D;
                texture->setImage(image);
                texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
                texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
                stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
                osg::notify(osg::NOTICE) << "[BG-DIAG] ground texture load success=" << filenames << std::endl;
                BG_DIAG_LOG("[BG-DIAG] ground texture load success=" << filenames);
            }
            else
            {
                osg::notify(osg::NOTICE) << "[BG-DIAG] ground texture load failed" << std::endl;
                BG_DIAG_LOG("[BG-DIAG] ground texture load failed");
            }

            stateset->setMode(GL_LIGHTING, osg::StateAttribute::ON);

            geode->setStateSet(stateset);
            osg::notify(osg::NOTICE) << "[BG-DIAG] ground states lighting=" << ((stateset->getMode(GL_LIGHTING) & osg::StateAttribute::ON)?"ON":"OFF")
                << " cull=" << ((stateset->getMode(GL_CULL_FACE) & osg::StateAttribute::ON)?"ON":"OFF")
                << " blend=" << ((stateset->getMode(GL_BLEND) & osg::StateAttribute::ON)?"ON":"OFF")
                << " depth=" << ((stateset->getMode(GL_DEPTH_TEST) & osg::StateAttribute::ON)?"ON":"OFF")
                << " renderHint=" << (int)stateset->getRenderingHint()
                << " texAttr=" << (stateset->getTextureAttribute(0, osg::StateAttribute::TEXTURE)!=NULL?"YES":"NO") << std::endl;
            BG_DIAG_LOG("[BG-DIAG] ground states lighting=" << ((stateset->getMode(GL_LIGHTING) & osg::StateAttribute::ON)?"ON":"OFF") << " cull=" << ((stateset->getMode(GL_CULL_FACE) & osg::StateAttribute::ON)?"ON":"OFF") << " blend=" << ((stateset->getMode(GL_BLEND) & osg::StateAttribute::ON)?"ON":"OFF") << " depth=" << ((stateset->getMode(GL_DEPTH_TEST) & osg::StateAttribute::ON)?"ON":"OFF") << " renderHint=" << (int)stateset->getRenderingHint() << " texAttr=" << (stateset->getTextureAttribute(0, osg::StateAttribute::TEXTURE)!=NULL?"YES":"NO"));

            float radius = 1910.f;
            float height = 0.05f;
            osg::TessellationHints* hints = new osg::TessellationHints;
            hints->setDetailRatio(0.5f);
            geode->addDrawable(
                new osg::ShapeDrawable(
                    new osg::Cylinder(osg::Vec3(6.0f, 0.0f, -35.0f), radius, height),
                    hints
                )
            );
            osg::notify(osg::NOTICE) << "[BG-DIAG] ground geometry=Cylinder radius=" << radius << " height=" << height << " center=(6,0,-35)" << std::endl;
            BG_DIAG_LOG("[BG-DIAG] ground geometry=Cylinder radius=" << radius << " height=" << height << " center=(6,0,-35)");

            transform->addChild(geode);
            osg::notify(osg::NOTICE) << "[BG-DIAG] ground node added to environment transform childCount=" << transform->getNumChildren() << std::endl;
            BG_DIAG_LOG("[BG-DIAG] ground node added to environment transform childCount=" << transform->getNumChildren());

            osg::BoundingSphere groundBound = geode->getBound();
            osg::notify(osg::NOTICE) << "[BG-DIAG] ground node created: " << geode->getName() << std::endl;
            osg::notify(osg::NOTICE) << "[BG-DIAG] ground bound center/radius: "
                << groundBound.center().x() << ", "
                << groundBound.center().y() << ", "
                << groundBound.center().z() << " / "
                << groundBound.radius() << std::endl;
            BG_DIAG_LOG("[BG-DIAG] ground bound center/radius: " << groundBound.center().x() << ", " << groundBound.center().y() << ", " << groundBound.center().z() << " / " << groundBound.radius());

            _scene->addChild(transform);
            osg::notify(osg::NOTICE) << "[BG-DIAG] environment scene child added" << std::endl;
            osg::notify(osg::NOTICE) << "[BG-DIAG] root child count=" << _scene->getNumChildren() << std::endl;
            BG_DIAG_LOG("[BG-DIAG] root child count=" << _scene->getNumChildren());
            osg::notify(osg::NOTICE) << "[BG-DIAG] Scene init environment section complete" << std::endl;
	//		_scene->getOrCreateStateSet()->setAttributeAndModes(fog.get());
          //  _oceanScene->addChild( transform );

            {
                // Create and add fake texture for use with nodes without any texture
                // since the OceanScene default scene shader assumes that texture unit 
                // 0 is used as a base texture map.
                osg::Image * image = new osg::Image;
                image->allocateImage( 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE );
                *(osg::Vec4ub*)image->data() = osg::Vec4ub( 0xFF, 0xFF, 0xFF, 0xFF );

                osg::Texture2D* fakeTex = new osg::Texture2D( image );
                fakeTex->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::REPEAT);
                fakeTex->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::REPEAT);
                fakeTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
                fakeTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);

                osg::StateSet* stateset = _scene->getOrCreateStateSet();
                stateset->setTextureAttribute(0,fakeTex,osg::StateAttribute::ON);
                stateset->setTextureMode(0,GL_TEXTURE_1D,osg::StateAttribute::OFF);
                stateset->setTextureMode(0,GL_TEXTURE_2D,osg::StateAttribute::ON);
                stateset->setTextureMode(0,GL_TEXTURE_3D,osg::StateAttribute::OFF);
            }

   

        {
          /*  ScopedTimer islandsTimer("  . Loading islands: ", osg::notify(osg::NOTICE));
            osg::ref_ptr<osg::Node> islandModel = loadIslands(terrain_shader_basename);

            if( islandModel.valid() )
            {
                _islandSwitch = new osg::Switch;
                _islandSwitch->addChild( islandModel.get(), true );
                
                _islandSwitch->setNodeMask( _oceanScene->getNormalSceneMask()    | 
                                            _oceanScene->getReflectedSceneMask() | 
                                            _oceanScene->getRefractedSceneMask() |
                                            _oceanScene->getHeightmapMask()      | 
                                            RECEIVE_SHADOW);

                _oceanScene->addChild( _islandSwitch.get() );
            }*/
        }

        {
            ScopedTimer lightingTimer("  . Setting up lighting: ", osg::notify(osg::NOTICE));
            osg::LightSource* lightSource = new osg::LightSource;
            lightSource->setNodeMask(lightSource->getNodeMask() & ~CAST_SHADOW & ~RECEIVE_SHADOW);
            lightSource->setLocalStateSetModes();

            _light = lightSource->getLight();
            _light->setLightNum(0);
            _light->setAmbient( osg::Vec4d(0.3f, 0.3f, 0.3f, 1.0f ));
            _light->setDiffuse( _sunDiffuse[_sceneType] );
            _light->setSpecular(osg::Vec4d( 0.1f, 0.1f, 0.1f, 1.0f ) );
#ifdef POINT_LIGHT
            _light->setPosition( osg::Vec4f(_sunPositions[_sceneType], 1.f) ); // point light
#else
            osg::Vec3f direction(_sunPositions[_sceneType]);
            direction.normalize();
            _light->setPosition( osg::Vec4f(direction, 0.0) );  // directional light
#endif

            _scene->addChild( lightSource );
     //       _scene->addChild( _oceanScene.get() );
            _scene->addChild( sunDebug(_sunPositions[CLOUDY]) );
            osg::notify(osg::NOTICE) << "[BG-DIAG] Scene init end childCount=" << _scene->getNumChildren() << std::endl;
            BG_DIAG_LOG("[BG-DIAG] Scene init end childCount=" << _scene->getNumChildren());
        }

        osg::notify(osg::NOTICE) << "complete.\nTime Taken: ";
    }
}

void CScene::changeScene( SCENE_TYPE type )
{
    _sceneType = type;

    _cubemap = loadCubeMapTextures( _cubemapDirs[_sceneType] );
    _skyDome->setCubeMap( _cubemap.get() );

//     _FFToceanSurface->setEnvironmentMap( _cubemap.get() );
//     _FFToceanSurface->setLightColor( _lightColors[type] );

//     _oceanScene->setAboveWaterFog(0.0012f, _fogColors[_sceneType] );
//     _oceanScene->setUnderwaterFog(0.002f,  _waterFogColors[_sceneType] );
//     _oceanScene->setUnderwaterDiffuse( _underwaterDiffuse[_sceneType] );
//     _oceanScene->setUnderwaterAttenuation( _underwaterAttenuations[_sceneType] );

    osg::Vec3f sunDir = -_sunPositions[_sceneType];
    sunDir.normalize();

 //   _oceanScene->setSunDirection( sunDir );

#ifdef POINT_LIGHT
    _light->setPosition( osg::Vec4f(_sunPositions[_sceneType], 1.f) );
#else
    _light->setPosition( osg::Vec4f(-sunDir, 0.f) );
#endif
    _light->setDiffuse( _sunDiffuse[_sceneType] ) ;

    if(_islandSwitch.valid() )
    {
        if(_sceneType == CLEAR || _sceneType == CLOUDY)
            _islandSwitch->setAllChildrenOn();
        else
            _islandSwitch->setAllChildrenOff();
    }
}

#define USE_CUSTOM_SHADER


osg::ref_ptr<osg::TextureCubeMap> CScene::loadCubeMapTextures( const std::string& dir )
{
    enum {POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z};

    std::string filenames[6];
	//std::string strPath = theApp.GetExePath();

	filenames[POS_X] = mStrPath+"\\resources\\textures\\" + dir + "\\east.png";
	filenames[NEG_X] = mStrPath+"\\resources\\textures\\" + dir + "\\west.png";
	filenames[POS_Z] = mStrPath+"\\resources\\textures\\" + dir + "\\north.png";
	filenames[NEG_Z] = mStrPath+"\\resources\\textures\\" + dir + "\\south.png";
	filenames[POS_Y] = mStrPath+"\\resources\\textures\\" + dir + "\\down.png";
	filenames[NEG_Y] = mStrPath+"\\resources\\textures\\" + dir + "\\up.png";

//     filenames[POS_X] = "F:\\2018镇江\\mfcOsg-2018-12-30-2\\Debug\\resources\\textures\\" + dir + "\\east.png";
//     filenames[NEG_X] = "F:\\2018镇江\\mfcOsg-2018-12-30-2\\Debug\\resources\\textures\\" + dir + "\\west.png";
//     filenames[POS_Z] = "F:\\2018镇江\\mfcOsg-2018-12-30-2\\Debug\\resources\\textures\\" + dir + "\\north.png";
//     filenames[NEG_Z] = "F:\\2018镇江\\mfcOsg-2018-12-30-2\\Debug\\resources\\textures\\" + dir + "\\south.png";
//     filenames[POS_Y] = "F:\\2018镇江\\mfcOsg-2018-12-30-2\\Debug\\resources\\textures\\" + dir + "\\down.png";
//     filenames[NEG_Y] = "F:\\2018镇江\\mfcOsg-2018-12-30-2\\Debug\\resources\\textures\\" + dir + "\\up.png";

    osg::ref_ptr<osg::TextureCubeMap> cubeMap = new osg::TextureCubeMap;
    cubeMap->setInternalFormat(GL_RGBA);

    cubeMap->setFilter( osg::Texture::MIN_FILTER,    osg::Texture::LINEAR_MIPMAP_LINEAR);
    cubeMap->setFilter( osg::Texture::MAG_FILTER,    osg::Texture::LINEAR);
    cubeMap->setWrap  ( osg::Texture::WRAP_S,        osg::Texture::CLAMP_TO_EDGE);
    cubeMap->setWrap  ( osg::Texture::WRAP_T,        osg::Texture::CLAMP_TO_EDGE);

    cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_X, osgDB::readImageFile( filenames[NEG_X] ) );
    cubeMap->setImage(osg::TextureCubeMap::POSITIVE_X, osgDB::readImageFile( filenames[POS_X] ) );
    cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Y, osgDB::readImageFile( filenames[NEG_Y] ) );
    cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Y, osgDB::readImageFile( filenames[POS_Y] ) );
    cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Z, osgDB::readImageFile( filenames[NEG_Z] ) );
    cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Z, osgDB::readImageFile( filenames[POS_Z] ) );

    return cubeMap;
}

osg::Geode* CScene::sunDebug( const osg::Vec3f& position )
{
    osg::ShapeDrawable* sphereDraw = new osg::ShapeDrawable( new osg::Sphere( position, 15.f ) );
    sphereDraw->setColor(osg::Vec4f(1.f,0.f,0.f,1.f));

    osg::Geode* sphereGeode = new osg::Geode;
    sphereGeode->addDrawable( sphereDraw );

    return sphereGeode;
}
