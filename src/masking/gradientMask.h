//
//  gradientMask.h
//  redundant-array
//
//  Created by Stephan Schulz on 2016-04-22.
//
//

#ifndef redundant_array_gradientMask_h
#define redundant_array_gradientMask_h

#include "ofxCv.h"

using namespace ofxCv;
using namespace cv;

class gradientMask{
public:
    
    ofParameter<int> maskType;
    
    ofParameter<bool> bShowMask,bShowWhite;
    
    //oval
    ofParameter<int> ovalWidth, ovalHeight, ovalX, ovalY;
    int old_ovalWidth, old_ovalHeight, old_ovalX, old_ovalY;
    ofParameter<int> gradientDistance;
    int old_gradientDistance;
    ofParameter<int> imgWidth = 1920;
    ofParameter<int> imgHeight = 1080;
    int old_imgHeight, old_imgWidth;
    //gradient frame
    ofColor startColor, endColor;
    ofMesh gradientMesh_frame;
    ofParameter<float> mask_edgeWidth, mask_innerCornerRadius,mask_outerCornerRadius;

    
    ofImage maskImg;
    
    void setup(){
        maskImg = makeEllipticalMask(ovalX,ovalY,ovalWidth,ovalHeight,imgWidth,imgHeight, gradientDistance);
        old_ovalWidth = ovalWidth;
        old_ovalHeight = ovalHeight;
        old_ovalX = ovalX;
        old_ovalY = ovalY;
        
        bShowWhite = false;
    }
    
    void draw(){
        if(bShowMask){
            if(bShowWhite){
                ofSetColor(255);
                ofDrawRectangle(0,0,ofGetWidth(),ofGetHeight());
            }
            
            ofPushStyle();
            ofSetColor(255);
            if(maskType == 0){
                //TODO: use same mesh method to make main round alpha mask
                //but do not apply it to the main FBO but rather make an actual mask
                
                ofEnableAlphaBlending();
                maskImg.draw(0,0);
//                ofDisableAlphaBlending();
            } else if(maskType == 1){
                ofEnableAlphaBlending();
                makeGradient_frame(mask_edgeWidth, mask_outerCornerRadius, mask_innerCornerRadius,  ofRectangle(0,0,ofGetWidth(),ofGetHeight()), 0,255);
//                ofDisableAlphaBlending();
            }
            ofPopStyle();
            
        }
        
    }
    ofImage makeEllipticalMask(int _x, int _y,int _ovalW, int _ovalH, int _imgW, int _imgH,int _gradientDistance){
        
        Mat mask(_imgH,_imgW, CV_8UC4,Scalar(0,0,0,255));
        
        cv::ellipse(mask, cv::Point(_x, _y),cv::Size( _ovalW, _ovalH ),0,0,360,Scalar( 0, 0, 0,0 ), -1, 8);
        
                blur(mask,mask,_gradientDistance);
        //        GaussianBlur(mask,mask,cv::Size( 0, 0 ), _gradientDistance, _gradientDistance);
        //        GaussianBlur(mask,mask,cv::Size( 553, 553 ), 0, 0);
//        GaussianBlur(mask,mask,_gradientDistance);
        //        medianBlur(mask,mask,_gradientDistance);
        ofImage img;
        copy(mask,img);
        img.update();
        
        return img;
    }
    
    ofImage makeEllipticalMask2(int _x, int _y, int _ovalW, int _ovalH, int _imgW, int _imgH,int _gradientDistance){
        ofLogNotice("gradientMask")<<"start making mask";
        ofPixels pix;
        ofImage img;
        ofPolyline myShape;
        
        pix.allocate(_imgW, _imgH, 4);
        img.allocate(_imgW, _imgH,OF_IMAGE_COLOR); //_ALPHA);

        
        vector<ofPolyline> pLine;
        
        //make ellipsise and pass to polyline
        ofPath myEllipse;
        myEllipse.ellipse(_x, _y, _ovalW, _ovalH);
        myEllipse.setCircleResolution(100);
        pLine = myEllipse.getOutline();
        
        //extract just one polyline
        for(int i=1 ; i<pLine[0].size(); i++){
            myShape.addVertex(pLine[0][i]);
        }
        
        myShape.close();
        
//        int maxDist = _imgW/_gradientDistance;
        ofColor black = ofColor(0,0,0,0);
        ofPoint closestPoint;
        ofPoint myP;
        float dist;
        int alpha ;
        
        //make img
        for(int x = 0; x<_imgW; x++){
            for(int y = 0; y<_imgH; y++){
                myP = ofPoint(x,y);
                
                if(myShape.inside(myP) == false){
                    
                    closestPoint = myShape.getClosestPoint(myP);
                    dist = closestPoint.distance(myP);
                    //            int alpha = ofMap(dist,0,ofGetWidth()/2,0,255,true);
                    alpha = ofMap(dist,0,_gradientDistance,0,255,true);
                    //            ofDrawLine(closestPoint,myP);
                    
                    
                    pix.setColor(x, y, ofColor(0,0,0,alpha));
                    
                    
                }else{
                    pix.setColor(x, y, black);
                }
            }
        }
        
        img.setFromPixels(pix);
        img.save("gradientMask");
        ofLogNotice("gradientMask")<<"done making mask";
        return img;
        
    }
    
    void makeGradient_frame(float band, float outerRadius, float innerRadius,  ofRectangle tt, float _startAlpha, float _endAlpha){
        
        startColor = ofColor(0,0,0,_startAlpha);
        endColor = ofColor(0,0,0,_endAlpha);
        
        float w = tt.getWidth();
        float h = tt.getHeight();
        
        gradientMesh_frame.clear();
        gradientMesh_frame.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        
        
        
        ofPath innerPath, outerPath;
        innerPath.rectRounded(tt.x+band,tt.y+band, 0, w-band*2, h-band*2,innerRadius,innerRadius,innerRadius,innerRadius);
        outerPath.rectRounded(tt.x,tt.y, 0, w,h,outerRadius,outerRadius,outerRadius,outerRadius);
        
        ofPolyline myPline_outer,myPline_inner;
        myPline_outer.addVertices(outerPath.getOutline()[0].getVertices());
        myPline_inner.addVertices(innerPath.getOutline()[0].getVertices());
        
        for(int i=0; i<myPline_outer.size();i++){
            gradientMesh_frame.addVertex(myPline_outer[i]);
            gradientMesh_frame.addColor(endColor);
            
            gradientMesh_frame.addVertex(myPline_inner[i]);
            gradientMesh_frame.addColor(startColor);
        }
        
        gradientMesh_frame.draw();
        
        
    }
    
    void checkGui(){
        if(old_ovalWidth != ovalWidth || 
           old_ovalHeight  != ovalHeight || 
           old_ovalX != ovalX || 
           old_ovalY != ovalY || 
           old_gradientDistance != gradientDistance ||
           old_imgWidth != imgWidth ||
           old_imgHeight != imgHeight){
            old_ovalWidth = ovalWidth;
            old_ovalHeight = ovalHeight;
            old_ovalX = ovalX;
            old_ovalY = ovalY;
            old_gradientDistance = gradientDistance;
            old_imgWidth = imgWidth;
            old_imgHeight = imgHeight;
            
            maskImg = makeEllipticalMask2(ovalX,ovalY,ovalWidth,ovalHeight,imgWidth,imgHeight, gradientDistance);
        }
    }
};

#endif
