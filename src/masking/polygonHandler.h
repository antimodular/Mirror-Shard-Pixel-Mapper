//
//  polygonHandler.h
//  camera_dmx
//
//  Created by Stephan Schulz on 2021-10-23.
//

#ifndef polygonHandler_h
#define polygonHandler_h

#include "ofEvents.h"

typedef struct {
    
    float     x;
    float     y;
    bool     bBeingDragged;
    bool     bOver;
    float     radius;
    
}    draggableVertex;

class polygonHandler {
private:
    
public:
    ofParameterGroup parameters;
    ofParameter<bool> bUseCurves;
    ofParameter<bool> bEdit;
    bool old_bEdit;
    
//    ofParameter<bool> bUse;
    ofParameter<bool> applyTo;
    ofParameter<bool> drawOnImage;
    
    ofParameter<int> maskAlpha;
    ofParameter<int> maskGrey;
    ofParameter<int> amtPoints;
    int old_amtPoints;
    
    int nCurveVertices;
    vector<draggableVertex> curveVertices;
    ofPolyline polylineShape;
    
    ofColor polygonColor, normalColor, highlightColor, editColor;
    
    string fileName;
    int lastActive;
    
    bool bHasListener;
    ofRectangle maxRect;
    
    int mainPosX = 0;
    int mainPosY = 0;
    float mainScaler = 1;
    
    bool bUseRectangle;

    
    void setup(string _name, bool _useRect = false){
        //    myPolygon_Font.loadFont(ofToDataPath("Arial.ttf"),12, false, true);
        
        parameters.setName(_name);
        parameters.add(bEdit.set("edit",false));
        
        parameters.add(drawOnImage.set("drawOnImage",false));
        parameters.add(applyTo.set("applyToImage",false));
        
        parameters.add(maskAlpha.set("maskAlpha",200,0,255));
        parameters.add(maskGrey.set("maskGrey",10,0,255));
        
        bUseRectangle = _useRect;
        
        
       
        if(bUseRectangle == false){
            parameters.add(bUseCurves.set("useCurves",false));
            parameters.add(amtPoints.set("amtPoints",10,3,20));
        }else{
            bUseCurves = false;
            amtPoints = 4;
        }
        
        highlightColor.set(255,255,0,80); // 0xFF0000;
        normalColor.set(0,0,0,255); //0xFFFFFF;
        polygonColor.set(0,0,0,255); ///0xFFFFFF;
        editColor.set(100,0,0,255);
        fileName  = _name;
        
//        maxRect = ofRectangle(0,0,1920,1080);
        ofLog()<<"polygonHandler setup() done";
    }
    
    void init(){
        bEdit = false;
        
        old_amtPoints = amtPoints;
        
        nCurveVertices = loadPoints(fileName);
        
        if(amtPoints > nCurveVertices){
            ofLog()<<"not enough mask points loaded. make more polygon points "<<nCurveVertices<<" of "<<amtPoints;
            
            //            curveVertices.resize(amtPoints);
            
            for(int i = nCurveVertices; i < amtPoints; i++){
                //                curveVertices[i].x = 100 + i*30;
                //                curveVertices[i].y = 100 + i*30 + ofRandom(100);
                curveVertices.emplace_back();
                curveVertices.back().x = 100 + i*30;
                curveVertices.back().y = 100 + i*30 + ofRandom(100);
            }
            
        } else if(amtPoints < nCurveVertices){
            curveVertices.resize(amtPoints); // removes elements greater than amtPoints
        }
        
        nCurveVertices = curveVertices.size();
        
        for (int i = 0; i < nCurveVertices; i++){
            curveVertices[i].bOver             = false;
            curveVertices[i].bBeingDragged     = false;
            curveVertices[i].radius = 15;
        }
        
        polylineShape.clear();
        if(bUseRectangle == false){
            for (int i = 0; i < nCurveVertices; i++){
                polylineShape.addVertex(ofVec3f(curveVertices[i].x,curveVertices[i].y,0));
            }
        }else{
            makeRectangle();
        }
        
        polylineShape.close();
        
        ofLog()<<"polygonHandler init() done";
    }
    
    void makeRectangle(){
     
        polylineShape.addVertex(ofVec3f(maxRect.getLeft(),maxRect.getTop(),0));
        polylineShape.addVertex(ofVec3f(curveVertices[0].x,curveVertices[0].y,0));
        polylineShape.addVertex(ofVec3f(curveVertices[1].x,curveVertices[1].y,0));
        polylineShape.addVertex(ofVec3f(curveVertices[2].x,curveVertices[2].y,0));
        polylineShape.addVertex(ofVec3f(curveVertices[3].x,curveVertices[3].y,0));
        polylineShape.addVertex(ofVec3f(curveVertices[0].x,curveVertices[0].y,0));
        polylineShape.addVertex(ofVec3f(maxRect.getLeft(),maxRect.getTop(),0));
        polylineShape.addVertex(ofVec3f(maxRect.getLeft(),maxRect.getBottom(),0));
        polylineShape.addVertex(ofVec3f(maxRect.getRight(),maxRect.getBottom(),0));
        polylineShape.addVertex(ofVec3f(maxRect.getRight(),0,0));
        
        //top rect
//        polylineShape.addVertex(ofVec3f(maxRect.getRight(),0,0));
//        polylineShape.addVertex(ofVec3f(curveVertices[1].x,curveVertices[1].y,0));
//        polylineShape.addVertex(ofVec3f(curveVertices[0].x,curveVertices[0].y,0));
//
//        //right rect
//        polylineShape.addVertex(ofVec3f(maxRect.getRight(),0,0));
//        polylineShape.addVertex(ofVec3f(maxRect.getRight(),maxRect.getBottom(),0));
//        polylineShape.addVertex(ofVec3f(curveVertices[2].x,curveVertices[2].y,0));
//        polylineShape.addVertex(ofVec3f(curveVertices[1].x,curveVertices[1].y,0));
//
//        //bottom rect
//        polylineShape.addVertex(ofVec3f(maxRect.getRight(),maxRect.getBottom(),0));
//        polylineShape.addVertex(ofVec3f(maxRect.getLeft(),maxRect.getBottom(),0));
//        polylineShape.addVertex(ofVec3f(curveVertices[3].x,curveVertices[3].y,0));
//        polylineShape.addVertex(ofVec3f(curveVertices[2].x,curveVertices[2].y,0));
//
//        //bottom rect
//        polylineShape.addVertex(ofVec3f(maxRect.getLeft(),maxRect.getBottom(),0));
//        polylineShape.addVertex(ofVec3f(0,0,0));
//        polylineShape.addVertex(ofVec3f(curveVertices[0].x,curveVertices[0].y,0));
//        polylineShape.addVertex(ofVec3f(curveVertices[3].x,curveVertices[3].y,0));
    }
    void exit(){
        //        savePoints(fileName);
    }
    
    void update()
    {
        
        checkGui();
        
        if(bEdit){
            ofShowCursor();
            limitPolygon();
            polylineShape.clear();
            if(bUseRectangle == false){
                for (int i = 0; i < nCurveVertices; i++){
                    polylineShape.addVertex(ofVec3f(curveVertices[i].x,curveVertices[i].y,0));
                }
            }else{
                makeRectangle();
            }
            polylineShape.close();
        }
        
        if(old_bEdit != bEdit){
            old_bEdit = bEdit;
            if(bEdit){
                addListeners();
            }else{
                removeListeners();
            }
        }
    }
    
    void limitPolygon(){
        
        for (int i = 0; i < nCurveVertices; i++){
            
            if(curveVertices[i].x < 0){
                curveVertices[i].x = 0; 
            }else if(curveVertices[i].x > maxRect.getRight()){
                curveVertices[i].x =  maxRect.getRight();
            }
                     
         if(curveVertices[i].y < 0){
                curveVertices[i].y = 0; 
            }else if(curveVertices[i].y > maxRect.getBottom()){
                curveVertices[i].y =  maxRect.getBottom();
            }
        }
    }
    void checkGui(){
        if(old_amtPoints != amtPoints){
            ofLog()<<"old_amtPoints != amtPoints";
            old_amtPoints = amtPoints;
            init();
            bEdit = true;
        }
    }
    void draw()
    {
        
        if(drawOnImage){
            //
            //         ofCurveVertex
            //
            //         because it uses catmul rom splines, we need to repeat the first and last
            //         items so the curve actually goes through those points
            //
            //        ofSetPolyMode(OF_POLY_WINDING_ODD);    // this is the normal mode
            
            if(bUseCurves){
//                polygonColor.a = maskAlpha;
//                polygonColor.r = maskGrey;
                
                ofSetColor(maskGrey,0,0,maskAlpha);
                ofFill();
                
                
                ofBeginShape();
                
                for (int i = 0; i < nCurveVertices; i++){
                    
                    //                    ofLog()<<i<<" "<<curveVertices[i].x<<" , "<<curveVertices[i].y
                    ;        // sorry about all the if/states here, but to do catmull rom curves
                    // we need to duplicate the start and end points so the curve acutally
                    // goes through them.
                    
                    // for i == 0, we just call the vertex twice
                    // for i == nCurveVertices-1 (last point) we call vertex 0 twice
                    // otherwise just normal ofCurveVertex call
                    
                    if (i == 0){
                        ofCurveVertex(curveVertices[0].x, curveVertices[0].y); // we need to duplicate 0 for the curve to start at point 0
                        ofCurveVertex(curveVertices[0].x, curveVertices[0].y); // we need to duplicate 0 for the curve to start at point 0
                    } else if (i == nCurveVertices-1){
                        ofCurveVertex(curveVertices[i].x, curveVertices[i].y);
                        ofCurveVertex(curveVertices[0].x, curveVertices[0].y);    // to draw a curve from pt 6 to pt 0
                        ofCurveVertex(curveVertices[0].x, curveVertices[0].y);    // we duplicate the first point twice
                    } else {
                        ofCurveVertex(curveVertices[i].x, curveVertices[i].y);
                    }
                }
                
                ofEndShape();
            }//end  if(bUseCurves){
            
            // show a faint the non-curve version of the same polygon:
            ofEnableAlphaBlending();
            ofFill();
            if(bEdit){
                ofSetColor(maskGrey,0,0,maskAlpha);
            }else{
                ofSetColor(maskGrey,maskGrey,maskGrey,maskAlpha);
            }
            ofBeginShape();
            if(bUseRectangle == false){
                
                for (int i = 0; i < nCurveVertices; i++){
                    ofVertex(curveVertices[i].x, curveVertices[i].y);
                }
                
            }else{
                for(auto & v : polylineShape.getVertices()){
                    ofVertex(v);
                }
            }
            ofEndShape(true);
            
            if(bEdit){
                polylineShape.draw();
                
                for (int i = 0; i < nCurveVertices; i++){
                    ofSetColor(highlightColor);
                    ofNoFill();
                    ofDrawCircle(curveVertices[i].x, curveVertices[i].y,curveVertices[i].radius);
                    
                    ofSetColor(highlightColor);
                    if (curveVertices[i].bOver == true) ofFill();
                    else ofNoFill();
                    ofDrawCircle(curveVertices[i].x, curveVertices[i].y,curveVertices[i].radius);
                    
                    ofSetColor(255);
                    ofDrawBitmapString(ofToString(i),curveVertices[i].x, curveVertices[i].y);
                }
            }
            ofDisableAlphaBlending();
        }
        
    }
    
    std::vector<std::vector<cv::Point> > getCVpolyline(){
        std::vector<cv::Point> fillContSingle;
        std::vector<std::vector<cv::Point> > fillContAll;
        if(bUseRectangle == false){
            for (int i = 0; i < nCurveVertices; i++){
                //add all points of the contour to the vector
                fillContSingle.push_back(cv::Point(curveVertices[i].x, curveVertices[i].y));
            }
           
        }else{
            for(auto & v : polylineShape.getVertices()){
                //add all points of the contour to the vector
                fillContSingle.push_back(cv::Point(v.x, v.y));
            }
        }
        //fill the single contour
        //(one could add multiple other similar contours to the vector)
        fillContAll.push_back(fillContSingle);
        return fillContAll;
    }
    
    void addListeners(){
        
        if(bHasListener == false){
            ofLog()<<"bHasListener()";
            ofAddListener(ofEvents().mousePressed, this, &polygonHandler::mousePressed);
            ofAddListener(ofEvents().mouseDragged, this, &polygonHandler::mouseDragged);
            ofAddListener(ofEvents().mouseReleased, this, &polygonHandler::mouseReleased);
            ofAddListener(ofEvents().mouseMoved, this, &polygonHandler::mouseMoved);
            
            ofAddListener(ofEvents().keyPressed, this, &polygonHandler::keyPressed);
            ofAddListener(ofEvents().keyReleased, this, &polygonHandler::keyReleased);
        }
        bHasListener = true;
    }
    void removeListeners(){
        
        if(bHasListener == true){
            ofLog()<<"removeListeners()";
            ofRemoveListener(ofEvents().mousePressed, this, &polygonHandler::mousePressed);
            ofRemoveListener(ofEvents().mouseDragged, this, &polygonHandler::mouseDragged);
            ofRemoveListener(ofEvents().mouseReleased, this, &polygonHandler::mouseReleased);
            ofRemoveListener(ofEvents().mouseMoved, this, &polygonHandler::mouseMoved);
            
            ofRemoveListener(ofEvents().keyPressed, this, &polygonHandler::keyPressed);
            ofRemoveListener(ofEvents().keyReleased, this, &polygonHandler::keyReleased);
            
            savePoints(fileName);
        }
        bHasListener = false;
    }
    
    void  keyPressed(ofKeyEventArgs& keyArgs){
        
    }
    void  keyReleased(ofKeyEventArgs& keyArgs){
        if(bEdit){
            //        for (int i = 0; i < nCurveVertices; i++){
            int i = lastActive;
            //            if (curveVertices[i].bBeingDragged == true){
            if(keyArgs.key == OF_KEY_LEFT){
                curveVertices[i].x -= 1;
            } else if(keyArgs.key == OF_KEY_RIGHT){
                curveVertices[i].x += 1;
            } else if(keyArgs.key == OF_KEY_UP){
                curveVertices[i].y -= 1;
            } else if(keyArgs.key == OF_KEY_DOWN){
                curveVertices[i].y += 1;
            }
            
            //            }
            //        }
        }
        
    }
    
    //------------- -------------------------------------------------
    void mouseMoved(ofMouseEventArgs& mouseArgs){
        //printf("mouseMoved");
        
        if(bEdit){
            float x0 = (mouseArgs.x - mainPosX) / mainScaler;
            float y0 = (mouseArgs.y - mainPosY) / mainScaler;
            
            for (int i = 0; i < nCurveVertices; i++){
                float dist = ofVec2f(x0,y0).distance(ofVec2f(curveVertices[i].x,curveVertices[i].y));
//                float dist = sqrt(diffx*diffx + diffy*diffy);
                if (dist < curveVertices[i].radius){
                    curveVertices[i].bOver = true;
                    //                    ofLog()<<i<<" mouseMoved bOver == true";
                } else {
                    curveVertices[i].bOver = false;
                }
            }
        }
    }
    
    //--------------------------------------------------------------
    void mouseDragged(ofMouseEventArgs &mouseArgs){
        
        if(bEdit){
            float x0 = (mouseArgs.x - mainPosX) / mainScaler;
            float y0 = (mouseArgs.y - mainPosY) / mainScaler;
            for (int i = 0; i < nCurveVertices; i++){
                if (curveVertices[i].bBeingDragged == true){
                    //                    ofLog()<<i<<" mouseDragged bBeingDragged == true";
                    curveVertices[i].x = x0; //mouseArgs.x;
                    curveVertices[i].y = y0; //mouseArgs.y;
                }
            }
        }
    }
    
    //--------------------------------------------------------------
    void mousePressed(ofMouseEventArgs& mouseArgs){
        if(bEdit){
            float x0 = (mouseArgs.x - mainPosX) / mainScaler;
            float y0 = (mouseArgs.y - mainPosY) / mainScaler;
            
            for (int i = 0; i < nCurveVertices; i++){
//                float diffx = mouseArgs.x - curveVertices[i].x;
//                float diffy = mouseArgs.y - curveVertices[i].y;
//                float dist = sqrt(diffx*diffx + diffy*diffy);
                
                float dist = ofVec2f(x0,y0).distance(ofVec2f(curveVertices[i].x,curveVertices[i].y));
                
                if (dist < curveVertices[i].radius){
                    //                    ofLog()<<i<<" mousePressed bBeingDragged == true";
                    curveVertices[i].bBeingDragged = true;
                    lastActive = i;
                } else {
                    curveVertices[i].bBeingDragged = false;
                }
            }
        }
    }
    
    //--------------------------------------------------------------
    void mouseReleased(ofMouseEventArgs &mouseArgs){
        
        if(bEdit){
            for (int i = 0; i < nCurveVertices; i++){
                curveVertices[i].bBeingDragged = false;
            }
        }
    }
    
    int loadPoints(string _name){
        ofLog()<<"polygonHandler loadPoints() "<<_name;
        ifstream textFile;
        
        textFile.open(ofToDataPath("masking/"+_name+".txt").c_str()); //ios::in);
        int lineCount = 0;
        curveVertices.clear();
        bool bDoneLoading = false;
        
        if(textFile.is_open() == false){
            cout<<"no text to load from"<<endl;
            return 0;
        }else{
            
            string line;
            //        while(textFile.is_open() && getline(textFile,line)){
            //            if(line.length() >0){
            //                lineCount ++;
            //            }
            //        }
            
            while(textFile.is_open() && getline(textFile,line)){
                
                if(line.length() >0){
                    
                    curveVertices.resize(curveVertices.size()+1);
                    vector<string> splitLine;
                    
                    splitLine = ofSplitString(line, ",", true);
                    
                    //                draggableVertex temp_drag;
                    //                temp_drag.x = ofToFloat(splitLine[0]);
                    //                temp_drag.y = ofToFloat(splitLine[1]);
                    //                curveVertices.push_back(temp_drag);
                    curveVertices.back().x = ofToFloat(splitLine[0]);
                    curveVertices.back().y = ofToFloat(splitLine[1]);
                    
                    ofLog()<<"load polygon point "<< curveVertices.back().x<<" , "<< curveVertices.back().y;
                    lineCount ++;
                    
                    
                }
            }
            
            
        }
        textFile.close();
        
        ofLog()<<"load "<<lineCount<<" mask points";
        return lineCount;
        
    }
    void savePoints(string _name){
        ofLog()<<"polygonHandler savePoints() "<<_name;
        
        ofstream textFile;
        string outString;
        
        //ofstream textFile;
        textFile.open(ofToDataPath("masking/"+_name+".txt").c_str()); //ios::in);
        //this also creates a text file if non exits
        
        outString = "";
        for(int i=0; i<curveVertices.size(); i++){
            ofLog()<<i<<" save polygons "<<ofToString(curveVertices[i].x)+ "," +ofToString(curveVertices[i].y);
            outString += ofToString(curveVertices[i].x)+ "," +ofToString(curveVertices[i].y)+ "\n";
        }
        
        textFile.write(outString.c_str(), outString.size());
        textFile.close();
        
    }
    
    
};
#endif /* polygonHandler_h */
