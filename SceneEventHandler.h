#pragma once
#include <osgGA/GUIEventHandler>
#include <osgGA/FlightManipulator>
#include <osgGA/TerrainManipulator>
#include <osgViewer/View>
#include <osg/Notify>

#include "TextHUD.h"
#include "Scene.h"

// ----------------------------------------------------
//                   Event Handler
// ----------------------------------------------------

class CSceneEventHandler : public osgGA::GUIEventHandler
{
private:
    osg::ref_ptr<CScene>   _scene;
    osg::ref_ptr<TextHUD> _textHUD;
    osgViewer::View*      _view;
    osg::Vec3             _initialCameraPosition;
    osg::Vec3             _initialCenterPosition;
    enum CameraMode
    {
        FIXED,
        FLIGHT,
        TERRAIN
    };

    CameraMode _currentCameraMode;

public:
    CSceneEventHandler(CScene* scene, TextHUD* textHUD, osgViewer::View* view, const osg::Vec3& initialCameraPosition, const osg::Vec3& initialCenterPosition)
        : _scene(scene)
        , _textHUD(textHUD)
        , _view(view)
        , _currentCameraMode(TERRAIN)
        , _initialCameraPosition(initialCameraPosition)
        , _initialCenterPosition(initialCenterPosition)
    {
        _textHUD->setSceneText("Clear");
        _textHUD->setCameraText("MOUSE");
        osg::notify(osg::NOTICE) << "[BG-DIAG] SceneEventHandler ctor mode=TERRAIN" << std::endl;
        BG_DIAG_LOG("[BG-DIAG] SceneEventHandler ctor mode=TERRAIN");

        osgGA::TerrainManipulator* tm = new osgGA::TerrainManipulator;
        tm->setHomePosition(_initialCameraPosition, _initialCenterPosition, osg::Vec3f(0, 0, 1));
        osg::notify(osg::NOTICE) << "[BG-DIAG] manipulator=TerrainManipulator setHomePosition eye=" << _initialCameraPosition.x() << "," << _initialCameraPosition.y() << "," << _initialCameraPosition.z() << " center=" << _initialCenterPosition.x() << "," << _initialCenterPosition.y() << "," << _initialCenterPosition.z() << std::endl;
        BG_DIAG_LOG("[BG-DIAG] manipulator=TerrainManipulator setHomePosition eye=" << _initialCameraPosition.x() << "," << _initialCameraPosition.y() << "," << _initialCameraPosition.z() << " center=" << _initialCenterPosition.x() << "," << _initialCenterPosition.y() << "," << _initialCenterPosition.z());
        tm->setAllowThrow(false);
        tm->setAnimationTime(0.15);
        tm->setWheelZoomFactor(0.12);
        _view->setCameraManipulator(tm);
        osg::notify(osg::NOTICE) << "[BG-DIAG] ctor does not call home(0.0)" << std::endl;
        BG_DIAG_LOG("[BG-DIAG] ctor does not call home(0.0)");
    }

    void setTopView()
    {
        osg::notify(osg::NOTICE) << "[BG-DIAG] setTopView invoked" << std::endl;
        BG_DIAG_LOG("[BG-DIAG] setTopView invoked");
        if (!_view)
            return;

        osg::Node* sceneData = _view->getSceneData();
        osg::BoundingSphere bs = sceneData ? sceneData->getBound() : osg::BoundingSphere();
        osg::Vec3d center = bs.center();
        double radius = bs.radius();
        if (radius < 100.0)
            radius = 100.0;

        osg::Vec3d eye(center.x(), center.y(), center.z() + radius * 2.8);
        osg::Vec3d up(0.0, 1.0, 0.0);
        osgGA::CameraManipulator* manipulator = _view->getCameraManipulator();
        if (manipulator)
        {
            manipulator->setHomePosition(eye, center, up, false);
            manipulator->home(0.0);
        }
        else if (_view->getCamera())
        {
            _view->getCamera()->setViewMatrixAsLookAt(eye, center, up);
        }
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&)
    {
        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::DOUBLECLICK:
            if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            {
                setTopView();
                return true;
            }
            break;

        case osgGA::GUIEventAdapter::KEYUP:
            osg::notify(osg::NOTICE) << "[BG-DIAG] key up=" << ea.getKey() << std::endl;
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Home)
            {
                setTopView();
                return true;
            }
            else if (ea.getKey() == '3')
            {
                _scene->changeScene(CScene::CLOUDY);
                _textHUD->setSceneText("Pacific Cloudy");
                return false;
            }
            else if (ea.getKey() == 'C' || ea.getKey() == 'c')
            {
                osg::Vec3 eye(_initialCameraPosition);
                if (_currentCameraMode == FIXED)
                {
                    _currentCameraMode = FLIGHT;
                    osgGA::FlightManipulator* flight = new osgGA::FlightManipulator;
                    flight->setHomePosition(eye, _initialCenterPosition, osg::Vec3f(0, 0, 1));
                    flight->setAllowThrow(false);
                    flight->setAnimationTime(0.15);
                    _view->setCameraManipulator(flight);
                    osg::notify(osg::NOTICE) << "[BG-DIAG] camera mode switched to FLIGHT" << std::endl;
                    BG_DIAG_LOG("[BG-DIAG] camera mode switched to FLIGHT");
                    _textHUD->setCameraText("FLIGHT");
                }
                else if (_currentCameraMode == FLIGHT)
                {
                    _currentCameraMode = TERRAIN;
                    osgGA::TerrainManipulator* tm = new osgGA::TerrainManipulator;
                    tm->setHomePosition(eye, _initialCenterPosition, osg::Vec3f(0, 0, 1));
                    tm->setAllowThrow(false);
                    tm->setAnimationTime(0.15);
                    tm->setWheelZoomFactor(0.12);
                    _view->setCameraManipulator(tm);
                    tm->home(0.0);
                    _textHUD->setCameraText("MOUSE");
                    osg::notify(osg::NOTICE) << "[BG-DIAG] camera mode switched to TERRAIN and home(0.0) called" << std::endl;
                    BG_DIAG_LOG("[BG-DIAG] camera mode switched to TERRAIN and home(0.0) called");
                }
                else if (_currentCameraMode == TERRAIN)
                {
                    _currentCameraMode = FIXED;
                    _view->getCamera()->setViewMatrixAsLookAt(eye, _initialCenterPosition, osg::Vec3f(0, 0, 1));
                    _view->setCameraManipulator(NULL);
                    osg::notify(osg::NOTICE) << "[BG-DIAG] camera mode switched to FIXED" << std::endl;
                    BG_DIAG_LOG("[BG-DIAG] camera mode switched to FIXED");
                    _textHUD->setCameraText("FIXED");
                }
            }
            break;

        default:
            break;
        }

        return false;
    }

    void getUsage(osg::ApplicationUsage& usage) const
    {
        usage.addKeyboardMouseBinding("mouse left-drag", "Rotate view around terrain");
        usage.addKeyboardMouseBinding("mouse wheel", "Zoom toward mouse pointer");
        usage.addKeyboardMouseBinding("mouse middle/right-drag", "Pan view");
        usage.addKeyboardMouseBinding("double-click/Home", "Top-down global view");
        usage.addKeyboardMouseBinding("c", "Camera type (cycle through Fixed, Flight, Mouse)");
        usage.addKeyboardMouseBinding("3", "Select scene \"Pacific Cloudy\"");
    }
};
