//
//  OrthoEasyCam.hpp
//  GrayCodeCaptureDecodeUVC_singleWindow
//
//  Created by Roy Macdonald on 26-08-24.
//

#pragma once
#include "ofMain.h"


class OrthoEasyCam: public ofEasyCam {
public:
    
    OrthoEasyCam();
  
    void enableMouse();
    void disableMouse();
    bool isMouseEnabled();
    
    void draw(ofRectangle rect, ofBaseDraws* img);
    
    
    virtual void begin(const ofRectangle & viewport) override;
    
    virtual void begin() override{
        begin(getViewport());
    }
    
    virtual void end() override;
    
    float scrollSpeed = 2.0;
    
    ofRectangle getScreenImgRect(bool bIntersectWithControlArea = true);
    
    
    void setDrawableSize(glm::vec2 size);
    void setDrawableSize(float width, float height);
    glm::vec2 getDrawableSize(){return drawableSize;}
    
    ofEvent<void> transformChanged;
    
protected:
    virtual void onPositionChanged() override{ofEasyCam::onPositionChanged(); ofNotifyEvent(transformChanged, this);}
    virtual void onOrientationChanged() override{ofNotifyEvent(transformChanged, this);}
    virtual void onScaleChanged() override{ofNotifyEvent(transformChanged, this);}
    
private:
    
    glm::vec2 drawableSize;
    void setAutoScale(const ofRectangle& viewport);
    
    
    unsigned long lastTap = 0;

    bool bNeedsSetAutoScale = true;
    
    void mouseMoved(ofMouseEventArgs& args);
    bool mouseScrolled(ofMouseEventArgs & mouse);
    bool mouseReleased(ofMouseEventArgs & mouse);
    void keyPressed(ofKeyEventArgs& args);
    void keyReleased(ofKeyEventArgs& args);
    
    void _enableMouseListeners();
    void _disableMouseListeners();
    
    ofEventListeners mouseListeners;
    ofEventListener mouseMoveListener;
    
    bool isModKeyPressed = false;
    
};

