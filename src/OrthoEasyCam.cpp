//
//  OrthoEasyCam.cpp
//  GrayCodeCaptureDecodeUVC_singleWindow
//
//  Created by Roy Macdonald on 26-08-24.
//

#include "OrthoEasyCam.h"


//------------------------------------------------------------------------------------------------------------------------
OrthoEasyCam::OrthoEasyCam():ofEasyCam()
{
    removeAllInteractions();
//    addInteraction(ofEasyCam::TRANSFORM_TRANSLATE_XY, OF_MOUSE_BUTTON_LEFT);
//    addInteraction(ofEasyCam::TRANSFORM_TRANSLATE_Z, OF_MOUSE_BUTTON_RIGHT);

    enableOrtho();
    setNearClip(-1000000);
    setFarClip(1000000);
    setVFlip(true);
    disableMouseInput();
    
    
    
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::_enableMouseListeners(){
    if(!isMouseEnabled()){
        mouseListeners.push(ofEvents().mouseScrolled.newListener(this, &OrthoEasyCam::mouseScrolled, OF_EVENT_ORDER_BEFORE_APP));
        mouseListeners.push(ofEvents().mouseReleased.newListener(this, &OrthoEasyCam::mouseReleased, OF_EVENT_ORDER_BEFORE_APP));
        
        mouseListeners.push(ofEvents().keyPressed.newListener(this, &OrthoEasyCam::keyPressed, OF_EVENT_ORDER_BEFORE_APP));
        mouseListeners.push(ofEvents().keyReleased.newListener(this, &OrthoEasyCam::keyReleased, OF_EVENT_ORDER_BEFORE_APP));
        enableMouseInput();
        isModKeyPressed = false;
    }
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::_disableMouseListeners(){
    mouseListeners.unsubscribeAll();
    disableMouseInput();
    isModKeyPressed = false;
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::enableMouse(){
    mouseMoveListener = ofEvents().mouseMoved.newListener(this, &OrthoEasyCam::mouseMoved, OF_EVENT_ORDER_BEFORE_APP);
    
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::disableMouse(){
    mouseMoveListener.unsubscribe();
    
    _disableMouseListeners();
    
}

//------------------------------------------------------------------------------------------------------------------------
bool OrthoEasyCam::isMouseEnabled(){
    return !mouseListeners.empty();
}

//------------------------------------------------------------------------------------------------------------------------
bool OrthoEasyCam::mouseScrolled(ofMouseEventArgs& args){
    
    if(getControlArea().inside(args)){
        if(isModKeyPressed){
            return false;
        }
        
        float mx = args.scrollX * scrollSpeed * getScale().x;
        float my = args.scrollY * scrollSpeed * getScale().y;
    
//        ofLogNotice("OrthoEasyCam::mouseScrolled") << "getViewport().inside(args) " << std::boolalpha << getViewport().inside(args) << " vp: " << getViewport() << " mx: " << mx << "  my: " << my ;
        
        move(mx, my, 0);
//        move(args.scrollX * scrollSpeed * getScale().x, args.scrollY * scrollSpeed * getScale().y, 0);
//        if(_keepInsideViewport && _img){
//            ofRectangle screenImgRect(worldToScreen({0,0,0}), worldToScreen({_img->getWidth(),_img->getHeight(), 0}));
//            if(!getViewport().inside(screenImgRect)){
//                
//            }
//        }
        
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------------------------------
bool OrthoEasyCam::mouseReleased(ofMouseEventArgs & mouse){
    if(getControlArea().inside(mouse)){
        // Check if it's double click
        unsigned long curTap = ofGetElapsedTimeMillis();
        if(lastTap != 0 && curTap - lastTap < 200){
            bNeedsSetAutoScale = true;
            return true;
        }
        lastTap = curTap;
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::keyPressed(ofKeyEventArgs& args){
    if(args.key == OF_KEY_ALT){
        isModKeyPressed = true;
    }
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::keyReleased(ofKeyEventArgs& args){
    if(args.key == OF_KEY_ALT){
        isModKeyPressed = false;
    }
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::mouseMoved(ofMouseEventArgs& args){
    bool bInside = getControlArea().inside(args);
    bool camEn = isMouseEnabled();
    if(bInside && !camEn){
        _enableMouseListeners();
    }else if(!bInside && camEn){
        _disableMouseListeners();
    }
}
//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::setDrawableSize(float width, float height){
    setDrawableSize({width, height});
}
//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::setDrawableSize(glm::vec2 size){
    if(!ofIsFloatEqual(size.x, drawableSize.x) || !ofIsFloatEqual(size.y, drawableSize.y)){
        drawableSize = size;
        bNeedsSetAutoScale = true;
    }
    
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::begin(const ofRectangle & viewport){
//    _img = nullptr;
    setAutoScale(viewport);
    ofPushView();
    ofViewport(viewport);
    ofSetupScreen();
    ofEasyCam::begin(viewport);
    setControlArea(viewport);
    
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::setAutoScale(const ofRectangle& viewport){
    if(bNeedsSetAutoScale){
        bNeedsSetAutoScale = false;
        ofRectangle imgRect(0,0, drawableSize.x, drawableSize.y);
        imgRect.scaleTo(viewport);
        
        float s  =  drawableSize.x/imgRect.width;
        setPosition({drawableSize.x/2, drawableSize.y/2, 0});
        setScale( s);
    }
    
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::draw(ofRectangle rect, ofBaseDraws* img){
    if(img){

        setDrawableSize({img->getWidth(),img->getHeight()});
        
        begin(rect);

        img->draw(0,0);
        
        end();
        
    }
}

//------------------------------------------------------------------------------------------------------------------------
void OrthoEasyCam::end(){
    ofEasyCam::end();
    ofPopView();
}

//------------------------------------------------------------------------------------------------------------------------
ofRectangle OrthoEasyCam::getScreenImgRect(bool bIntersectWithControlArea ){
    ofRectangle r(worldToScreen(glm::vec3(0,0,0), getControlArea()), worldToScreen(glm::vec3(drawableSize,0), getControlArea()));
    if(bIntersectWithControlArea){
        return getControlArea().getIntersection(r);
    }
    return r;
    
}
