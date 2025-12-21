#ifndef pixelMapShader_h
#define pixelMapShader_h

#include "ofMain.h"
#include "singleShard.h"

class pixelMapShader {
    
private:
    
public:
    
    // Shader-based warping
    ofShader warpAllShader;

    
    string shaderPath = "shaders/warpAllShader";
    ofParameter<float> shardOpacity{"Shard Opacity", 0.0f, 0.0f, 1.0f};
    
    ofFbo allShardsFbo;
    
    // Quad mapping controls (which source quad maps to which destination quad)
    struct QuadMapping {
        int sourceQuad[4] = {0,1,2,3}; // Default: 1:1 mapping
        bool rotateQuad[4] = {false, false, false, false}; // 180° rotation for each quad
    } quadMapping;
    
    ofVec2f getQuadUV(int x, int y, int quadIndex, bool rotate) {
        // Calculate local quad coordinates (0-1 range within the quad)
        float localU = ofMap(x % (DISPLAY_WIDTH/2), 0, DISPLAY_WIDTH/2-1, 0, 1);
        float localV = ofMap(y % (DISPLAY_HEIGHT/2), 0, DISPLAY_HEIGHT/2-1, 0, 1);
        
        // Apply 180° rotation if needed
        if(rotate) {
            localU = 1.0 - localU;
            localV = 1.0 - localV;
        }
        
        // Map to source quad
        int sourceQuad = quadMapping.sourceQuad[quadIndex];
        float sourceU = (sourceQuad % 2 == 0) ? localU : localU + 1.0;
        float sourceV = (sourceQuad < 2) ? localV : localV + 1.0;
        
        return ofVec2f(sourceU/2.0, sourceV/2.0); // Normalize to 0-1 range
    }
    
    ofxPanel gui_pixelMap;
    ofParameterGroup quadMappingParams;
    // Source quad selection for each destination quad
    ofParameter<int> sourceQuad0{"Quad 0 Source", 0, 0, 3};
    ofParameter<int> sourceQuad1{"Quad 1 Source", 1, 0, 3};
    ofParameter<int> sourceQuad2{"Quad 2 Source", 2, 0, 3};
    ofParameter<int> sourceQuad3{"Quad 3 Source", 3, 0, 3};
    
    // Rotation toggles for each quad
    ofParameter<int> rotateQuad0{"Rotate Quad 0", 0, 0, 360};
    ofParameter<int> rotateQuad1{"Rotate Quad 1", 0, 0, 360};
    ofParameter<int> rotateQuad2{"Rotate Quad 2", 0, 0, 360};
    ofParameter<int> rotateQuad3{"Rotate Quad 3", 0, 0, 360};
    
    ofParameter<bool> mirrorQuad0{"mirror Quad 0", false};
    ofParameter<bool> mirrorQuad1{"mirror Quad 1", false};
    ofParameter<bool> mirrorQuad2{"mirror Quad 2", false};
    ofParameter<bool> mirrorQuad3{"mirror Quad 3", false};
    ofParameter<bool> flipQuad0{"flip Quad 0", false};
    ofParameter<bool> flipQuad1{"flip Quad 1", false};
    ofParameter<bool> flipQuad2{"flip Quad 2", false};
    ofParameter<bool> flipQuad3{"flip Quad 3", false};
    
    ofParameter<ofColor> bgColor{"Background Color", ofColor(0,0,0,0),ofColor(0,0,0,0),ofColor(255,255,255,255)};
    ofParameter<bool> showBgColor{"showBgColor", false};
    ofParameter<bool> showAllShards{"Show All Shards", false};
    ofParameter<int> currentShardIndex{"Current Shard", 0, 0, 10};
    ofParameter<bool> debugShaderView{"Debug Shader View", false};
    // ofParameter<bool> bUseInverse{"inverse homogr", false};
    bool bUseInverse = true;
    
    bool bDebug = false;
    void setup(){
        // Setup parameter group
        gui_pixelMap.setup("pixelMap", "GUIs/gui_pixelMap.json");
        gui_pixelMap.setHeaderBackgroundColor(ofColor(255, 0, 0));
        gui_pixelMap.add(bgColor);
        gui_pixelMap.add(showBgColor);
        gui_pixelMap.add(showAllShards);
        gui_pixelMap.add(currentShardIndex);
        gui_pixelMap.add(debugShaderView);
//        gui_pixelMap.add(bUseInverse);
        
        quadMappingParams.setName("quadMapping");
        quadMappingParams.add(sourceQuad0);
        quadMappingParams.add(sourceQuad1);
        quadMappingParams.add(sourceQuad2);
        quadMappingParams.add(sourceQuad3);
        quadMappingParams.add(rotateQuad0);
        quadMappingParams.add(rotateQuad1);
        quadMappingParams.add(rotateQuad2);
        quadMappingParams.add(rotateQuad3);
        quadMappingParams.add(mirrorQuad0);
        quadMappingParams.add(mirrorQuad1);
        quadMappingParams.add(mirrorQuad2);
        quadMappingParams.add(mirrorQuad3);
        quadMappingParams.add(flipQuad0);
        quadMappingParams.add(flipQuad1);
        quadMappingParams.add(flipQuad2);
        quadMappingParams.add(flipQuad3);
        gui_pixelMap.add(quadMappingParams);
        gui_pixelMap.loadFromFile("GUIs/gui_pixelMap.json");

        // Use the same FBO settings as customContentFbo
//        ofFboSettings settings;
//        settings.width = DISPLAY_WIDTH;
//        settings.height = DISPLAY_HEIGHT;
//        settings.internalformat = GL_RGBA;  // Use 32-bit float for better precision
//        settings.useDepth = true;
//        settings.useStencil = true;
//        settings.numSamples = 4;
//        settings.textureTarget = GL_TEXTURE_2D;
//
        allShardsFbo.allocate(DISPLAY_WIDTH, DISPLAY_HEIGHT, GL_RGBA);
        // Set high-quality texture filtering
//        allShardsFbo.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
//        allShardsFbo.getTexture().setTextureWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        
        allShardsFbo.begin();
        ofClear(0, 0, 0, 0);
        allShardsFbo.end();
        
        initializeShader();
    }
    
    
    
    void initializeShader() {
        ofLog()<<"initializeShader()";
    
        warpAllShader.load("shaders/warpAllShader");
        warpAllShader.printActiveUniforms();
        warpAllShader.printActiveAttributes();
    }
    
    void updateAllShardsFbo(ofFbo &customContentFbo, ofFbo &fbo_live, vector<singleShard>& shards) {
        if (customContentFbo.isAllocated() == false || fbo_live.isAllocated() == false || shards.empty()) return;
//
//        // Log FBO settings
//        ofLog() << "FBO Settings:";
//        ofLog() << "  customFbo:";
//        ofLog() << "    Width: " << customContentFbo.getWidth();
//        ofLog() << "    Height: " << customContentFbo.getHeight();
//        ofLog() << "    Texture format: " << customContentFbo.getTexture().getTextureData().glInternalFormat;
//        ofLog() << "    Texture ID: " << customContentFbo.getTexture().getTextureData().textureID;
//        ofLog() << "    Texture type: " << customContentFbo.getTexture().getTextureData().textureTarget;
//
//        ofLog() << "  allShardsFbo:";
//        ofLog() << "    Width: " << allShardsFbo.getWidth();
//        ofLog() << "    Height: " << allShardsFbo.getHeight();
//        ofLog() << "    Texture format: " << allShardsFbo.getTexture().getTextureData().glInternalFormat;
//        ofLog() << "    Texture ID: " << allShardsFbo.getTexture().getTextureData().textureID;
//        ofLog() << "    Texture type: " << allShardsFbo.getTexture().getTextureData().textureTarget;
//
//
        allShardsFbo.begin();
        ofClear(0, 0, 0, 0);
        
        if(showBgColor) {
            ofSetColor(bgColor);
            ofFill();
            ofDrawRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        }else{

            ofSetColor(bgColor);
            ofFill();
            ofDrawRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
            
             // Draw the live camera feed with opacity based on shardOpacity
             ofSetColor(255, 255, 255, (shardOpacity) * 255);
             
             // Draw each quadrant with its respective rotation and source quad mapping
             // Quadrant 0 (Top Left)
             ofPushMatrix();
             ofTranslate(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/4); //place it back in 1st quadrant
             ofRotateZDeg(rotateQuad0); //*90); //, 0, 0, 1); //rotate it
             int sourceQuad = sourceQuad0;
             int sourceX = (sourceQuad % 2) * (DISPLAY_WIDTH/2);
             int sourceY = (sourceQuad < 2) ? 0 : (DISPLAY_HEIGHT/2);
             ofScale(mirrorQuad0 ? -1 : 1,flipQuad0 ? -1 : 1,1);
             fbo_live.getTexture().drawSubsection(-DISPLAY_WIDTH/4, -DISPLAY_HEIGHT/4, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2,sourceX, sourceY, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2); // place it with HD center at 0,0
             ofPopMatrix();
             
             // Quadrant 1 (Top Right)
             ofPushMatrix();
             ofTranslate(DISPLAY_WIDTH/2, 0);//place it back in 2nd quadrant
             ofTranslate(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/4);//place it back in 1st quadrant
             ofRotateZDeg(rotateQuad1); //rotate it
             sourceQuad = sourceQuad1;
             sourceX = (sourceQuad % 2) * (DISPLAY_WIDTH/2);
             sourceY = (sourceQuad < 2) ? 0 : (DISPLAY_HEIGHT/2);
             ofScale(mirrorQuad1 ? -1 : 1,flipQuad1 ? -1 : 1,1);
             fbo_live.getTexture().drawSubsection(-DISPLAY_WIDTH/4, -DISPLAY_HEIGHT/4, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2,sourceX, sourceY, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2); //place it with HD center at 0,0
             ofPopMatrix();
             
             // Quadrant 2 (Bottom Left)
             ofPushMatrix();
             ofTranslate(0, DISPLAY_HEIGHT/2);
             ofTranslate(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/4);//place it back in 1st quadrant
             ofRotateZDeg(rotateQuad2); //*90, 0, 0, 1);
             sourceQuad = sourceQuad2;
             sourceX = (sourceQuad % 2) * (DISPLAY_WIDTH/2);
             sourceY = (sourceQuad < 2) ? 0 : (DISPLAY_HEIGHT/2);
             ofScale(mirrorQuad2 ? -1 : 1,flipQuad2 ? -1 : 1,1);
             fbo_live.getTexture().drawSubsection(-DISPLAY_WIDTH/4, -DISPLAY_HEIGHT/4, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2,sourceX, sourceY, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2); //place it with HD center at 0,0
             ofPopMatrix();
             
             // Quadrant 3 (Bottom Right)
             ofPushMatrix();
             ofTranslate(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2);
             ofTranslate(DISPLAY_WIDTH/4, DISPLAY_HEIGHT/4);//place it back in 1st quadrant
             ofRotateZDeg(rotateQuad3);//*90, 0, 0, 1);
             sourceQuad = sourceQuad3;
             sourceX = (sourceQuad % 2) * (DISPLAY_WIDTH/2);
             sourceY = (sourceQuad < 2) ? 0 : (DISPLAY_HEIGHT/2);
             ofScale(mirrorQuad3 ? -1 : 1,flipQuad3 ? -1 : 1,1);
             fbo_live.getTexture().drawSubsection(-DISPLAY_WIDTH/4, -DISPLAY_HEIGHT/4, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2,sourceX, sourceY, DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2); //place it with HD center at 0,0
             ofPopMatrix();
             
        } //end of if showBgColor or live camera as background
        
        ofSetColor(255, 255, 255);
        
        // Enable stencil buffer
        glEnable(GL_STENCIL_TEST);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        
        // First pass: Draw all shard masks to the stencil buffer
        ofPushStyle();
        
        // For each shard, draw its outline to the stencil buffer with its index as value
        for (int s = 0; s < shards.size(); s++) {
            int shardIndex = showAllShards ? s : currentShardIndex.get();
            if (shardIndex >= shards.size()) continue;
            
            singleShard &aShard = shards[shardIndex];
            aShard.updateTransformedMaskPoints();
            aShard.computeHomography(bDebug);
            if (aShard.transformedMaskPoints.size() < 3) continue;
            
            // Set stencil operation to replace with shard index + 1 (0 is background)
            glStencilFunc(GL_ALWAYS, s + 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            
            // Draw to stencil buffer without affecting color buffer
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            
            // Draw the shard mask to the stencil buffer
            ofFill();
            ofBeginShape();
            for (const auto &point : aShard.transformedMaskPoints) {
                ofVertex(point.x, point.y);
            }
            ofEndShape(true);
            
            if (!showAllShards) break;
        }
        
        // Re-enable color buffer drawing
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        
        // Configure stencil test to only process fragments where stencil is not 0
        glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        
        ofPopStyle();
        
        // Now start the shader for the second pass
        warpAllShader.begin();
        
        // Bind the texture and set parameters
        ofTexture &tex = customContentFbo.getTexture();
        tex.bind(0);
        
        // Set proper texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        warpAllShader.setUniformTexture("sourceTex", tex, 0);
        warpAllShader.setUniform2f("resolution", customContentFbo.getWidth(), customContentFbo.getHeight());
        warpAllShader.setUniform1i("debugView", debugShaderView ? 1 : 0);
        warpAllShader.setUniform1f("shardOpacity", 1-shardOpacity);
        // Convert ofColor to vec4 and pass to shader
        warpAllShader.setUniform4f("bgColor", 
            bgColor->r/255.0f, 
            bgColor->g/255.0f, 
            bgColor->b/255.0f, 
            bgColor->a/255.0f);
        
        // Set number of shards (max 14)
        int numShardsToUse = min(14, (int)shards.size());
        if (!showAllShards && currentShardIndex < shards.size()) {
            numShardsToUse = 1;
        }
        warpAllShader.setUniform1i("numShards", numShardsToUse);
        
        // Fill arrays with data
        for (int s = 0; s < numShardsToUse; s++) {
            int shardIndex = showAllShards ? s : currentShardIndex.get();
            if (shardIndex >= shards.size()) continue;
            
            const auto &shard = shards[shardIndex];
            
            // Use the proper homography matrix instead of its inverse based on bUseInverse setting
            ofLog()<<"pixelMapShader::updateShader()";
            if (bUseInverse) {
                if(shard.index == 0)  ofLog()<<"pixelMapShader "<<shard.name<<" shard.inverseHomography "<<shard.homography;
                
                warpAllShader.setUniformMatrix4f("invH" + ofToString(s), shard.inverseHomography);
            } else {
                if(shard.index == 0)  ofLog()<<"pixelMapShader "<<shard.name<<" shard.homography "<<shard.homography;
                warpAllShader.setUniformMatrix4f("invH" + ofToString(s), shard.homography);
            }
        }
        
        // Draw shards one by one with the correct stencil test
        for (int s = 0; s < numShardsToUse; s++) {
            int shardIndex = showAllShards ? s : currentShardIndex.get();
            if (shardIndex >= shards.size()) continue;
            
            // Set the current shard index for the shader
            warpAllShader.setUniform1i("currentShardIndex", s);
            
            // Enable stencil test to only draw on this shard's area
            glStencilFunc(GL_EQUAL, s + 1, 0xFF);
            
            // Draw full screen quad for this shard
            ofSetColor(255);
            ofFill();
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0);
            glVertex2f(0, 0);
            glTexCoord2f(1.0, 0.0);
            glVertex2f(DISPLAY_WIDTH, 0);
            glTexCoord2f(1.0, 1.0);
            glVertex2f(DISPLAY_WIDTH, DISPLAY_HEIGHT);
            glTexCoord2f(0.0, 1.0);
            glVertex2f(0, DISPLAY_HEIGHT);
            glEnd();
        }
        
        // Unbind the texture
        tex.unbind();
        
        warpAllShader.end();
        
        // Disable stencil test after drawing
        glDisable(GL_STENCIL_TEST);
        
        allShardsFbo.end();
    }
    
   
    
    ~pixelMapShader() {
    }
    
};


#endif /*pixelMapShader_h*/
