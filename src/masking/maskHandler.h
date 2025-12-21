//
//  masking.h
//  camera_dmx
//
//  Created by Stephan Schulz on 2021-10-23.
//

#ifndef masking_h
#define masking_h

#include "polygonHandler.h"
#include "gradientMask.h"

class maskHandler {
private:
    
public:
    bool bShowGUI;
    
    ofRectangle maxRect;
    int myID;
    //    ofParameter<bool> bEnableEdit;
    bool old_bEnableEdit;
    
    ofxPanel gui_mask;
    ofParameterGroup group_polygon;
    
    //-----custom polygon mask shapes
    vector<polygonHandler> allPolygons;
    
    polygonHandler rectPolygon;
    
    vector<ofVec3f> closestPoints;
    //    vector<bool> applyToImage;
    //    vector<bool> drawOnImage;
    
    gradientMask gradient_object;
    ofParameterGroup group_oval;
    
    int mainPosX = 0;
    int mainPosY = 0;
    float mainScaler = 1;
    
    void setup(int _id, int _amt){
        myID = _id;
        
        allPolygons.resize(_amt);
        //        applyToImage.resize(_amt);
        //        drawOnImage.resize(_amt);
        
        for(int i=0; i<allPolygons.size(); i++){
            allPolygons[i].setup("polygonMask_"+ofToString(i));
        }
        rectPolygon.setup("rectMask",true);
        
        group_polygon.setName("masking "+ofToString(myID));
        
        for(int i=0; i<allPolygons.size(); i++){
            //            group_polygon.add(applyToImage[i].set("apply to image",false));
            //            group_polygon.add(drawOnImage[i].set("draw on top",false));
            group_polygon.add(allPolygons[i].parameters);
        }
        group_polygon.add(rectPolygon.parameters);
        
//        oval_object.setup();
        
        group_oval.setName("gradientMask");
        group_oval.add(gradient_object.maskType.set("maskType",0,0,1));
        group_oval.add(gradient_object.bShowMask.set("showMask",true));
        group_oval.add(gradient_object.bShowWhite.set("showWhite",false));
        group_oval.add(gradient_object.ovalX.set("maskX",1080/2,0,1200));
        group_oval.add(gradient_object.ovalY.set("maskY",1920/2,0,1920));
        group_oval.add(gradient_object.ovalWidth.set("maskWidth",390,0,1200));
        group_oval.add(gradient_object.ovalHeight.set("maskHeight",1000,0,1920));
        group_oval.add(gradient_object.imgWidth.set("imgWidth",1000,0,1920));
        group_oval.add(gradient_object.imgHeight.set("imgHeight",1000,0,1920));

        group_oval.add(gradient_object.gradientDistance.set("gradientDistance",340,0,600));
        
        group_oval.add(gradient_object.mask_edgeWidth.set("mask_edgeWidth",100,1,500));
        group_oval.add(gradient_object.mask_outerCornerRadius.set("mask_outerRadius",1,1,100));
        group_oval.add(gradient_object.mask_innerCornerRadius.set("mask_innerRadius",100,1,1000));
        
        gui_mask.setup("masking","GUIs/gui_mask.json");
        gui_mask.setPosition(10,10);
        gui_mask.setHeaderBackgroundColor(ofColor(255,0,0));
        gui_mask.add(group_oval);
        gui_mask.add(group_polygon);
        gui_mask.loadFromFile("GUIs/gui_mask.json");
        gui_mask.minimizeAll();
        
        gradient_object.setup();
        gradient_object.bShowWhite = false;
        
        maxRect = ofRectangle(0,0,1920,1920);
        
    }
    
    void init(){
        for(auto & aPolygon : allPolygons){
            aPolygon.maxRect = maxRect;
            aPolygon.init();
        }
        rectPolygon.maxRect = maxRect;
        rectPolygon.init();
       
    }
    
    void exit(){
        for(auto & aPolygon : allPolygons){
            aPolygon.exit();
        }
        rectPolygon.exit();
    }
    
    void update(){
        
        gradient_object.checkGui();
        checkGui();
        for(auto & aPolygon : allPolygons){
            aPolygon.maxRect = maxRect;
            aPolygon.update();
        }
        rectPolygon.maxRect = maxRect;
        rectPolygon.update();
    }
    
    void checkGui(){
        
        
        
        for(auto & aPolygon : allPolygons){
            aPolygon.mainPosX = mainPosX;
            aPolygon.mainPosY = mainPosY;
            aPolygon.mainScaler = mainScaler;
        }
        rectPolygon.mainPosX = mainPosX;
        rectPolygon.mainPosY = mainPosY;
        rectPolygon.mainScaler = mainScaler;
        //        if(old_bEnableEdit != bEnableEdit){
        //            old_bEnableEdit = bEnableEdit;
        //            if(bEnableEdit){
        //                onePolygon.addListeners();
        //            }else{
        //                onePolygon.removeListeners();
        //            }
        //        }
        
    }
    
    void saveGui(){
        //        gui_masking.saveToFile("GUIs/gui_masking.json");
    }
    
    
    void draw(int _x, int _y){
        
        gradient_object.draw();
        
        for(auto & aPolygon : allPolygons){
            aPolygon.draw();
        }
        rectPolygon.draw();
        //        if(bShowGUI){
        //            gui_masking.draw();
        //        }
        
        
        
        
        //        for(auto & aPolygon : allPolygons){
        //            if(aPolygon.bE == true){
        //                aPolygon.polylineShape.draw();
        //            }
        //        }
        
        for(auto & v : closestPoints){
            ofFill();
            ofDrawCircle(v.x,v.y,6);
        }
    }
    
    void applyPolygonsToMat(cv::Mat & _mat){
        for(auto & aPolygon : allPolygons){
            if(aPolygon.applyTo) cv::fillPoly(_mat, aPolygon.getCVpolyline(), cv::Scalar(0));
        }
        if(rectPolygon.applyTo) cv::fillPoly(_mat, rectPolygon.getCVpolyline(), cv::Scalar(0));
    }
    
    float getDistanceToOutline(ofVec2f _pos){
        for(auto & aPolygon : allPolygons){
            
        }
        
        return 0;
    }
    
    void getClosestPoint(ofVec3f _pos, float & _distance){
        //TODO: should this live inside polygon?
        ofVec3f closestPoint;
        float minDistance = INT_MAX;
        closestPoints.clear();
        for(auto & aPolygon : allPolygons){
            
            if(aPolygon.bEdit == true){
                //                ofLog()<<"aPolygon.polylineShape "<<aPolygon.polylineShape.size();
                ofVec3f temp_point = aPolygon.polylineShape.getClosestPoint(_pos);
                closestPoints.push_back(temp_point);
                float temp_dist = _pos.distance(temp_point);
                if(temp_dist < minDistance){
                    minDistance = temp_dist;
                    _distance = temp_dist;
                    closestPoint = temp_point;
                }
                //                ofLog()<<_pos<<" -> closestPoints "<<temp_point<<" dist "<<temp_dist;
            }
        }
        
        
    }
    //    std::vector<std::vector<cv::Point> > getCVpolyline(){
    //        return onePolygon.getCVpolyline();
    //    }
};
#endif /* masking_h */
