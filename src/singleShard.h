#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "glm/gtc/matrix_inverse.hpp"

// Class to represent a single mirror shard with its own homography and
// calibration points
class singleShard {
 public:

   ofFboSettings fbo_settings;

  singleShard() {
    homographyReady = false;
    maskReady = false;
    
    // Allocate FBOs
  
    fbo_settings.width = 3840;  // 4K resolution
    fbo_settings.height = 2160;
    fbo_settings.internalformat = GL_RGBA;
    warpFbo.allocate(fbo_settings);
    maskFbo.allocate(fbo_settings);
    customFbo.allocate(fbo_settings); 
    customFbo_masked.allocate(fbo_settings);
    // Initialize quadrant info
    quadrant = ' ';
    quadrantColor = ofColor(255, 255, 255);
    quadrantOffset = ofVec2f(0, 0);
    
    // Initialize 3D orientation
    orientation3D = glm::vec3(0, 0, 0);
    position3D = glm::vec3(0, 0, 0);
  }

  // Points for calibration
  vector<ofVec2f> livePoints;     // Points in camera/mirror view
  vector<ofVec2f> displayPoints;  // Points in display/target view

  // Cache previous points to check for changes
  vector<ofVec2f> previousLivePoints;
  vector<ofVec2f> previousDisplayPoints;

  // Points for mask perimeter
  vector<ofVec2f> maskPoints;  // Points defining the perimeter of the shard in camera view
  vector<ofVec2f> previousMaskPoints;
  vector<ofVec2f> transformedMaskPoints;
  ofPath transformedPath;

  // 3D transformed points and wall transform matrix
  vector<glm::vec3> transformedMaskPoints3D;  // 3D transformed mask points

  // FBOs for warping and masking
  ofFbo warpFbo;
  ofFbo maskFbo;
  
  // Homography matrices
  glm::mat4 homography;
  glm::mat4 inverseHomography;
  bool homographyReady;
  
  glm::mat4 quadrantTransform;
  // 3D orientation and position
  glm::vec3 orientation3D; // Rotation angles in degrees (x, y, z)
  glm::vec3 position3D;    // Position in 3D space

  // Mask texture
  ofTexture maskTexture;
  bool maskReady;
  bool useMask = true;
  bool maskChanged = false;

  // Warped images
  ofImage warpedColor;         // Display image warped to live view
  ofImage customImage;         // Custom image to be warped
  ofFbo customFbo;
  ofFbo customFbo_masked;

  // Name for identification
  string name;
  int index;

  // Color for UI elements
  ofColor color;
  
  // Quadrant information
  char quadrant;               // 'A', 'B', 'C', or 'D'
  ofColor quadrantColor;       // Color associated with the quadrant
  ofVec2f quadrantOffset;      // Offset to move from 4K to HD position

  // Add these member variables after other declarations
  bool maskTextureChanged = true;

ofRectangle boundingBox;
ofRectangle prev_boundingBox;
bool bUseBoundingBox = true;

  // Helper function to check if points have changed
  bool pointsChanged() {
    if (livePoints.size() != previousLivePoints.size() ||
        displayPoints.size() != previousDisplayPoints.size()) {
      return true;
    }

    for (size_t i = 0; i < livePoints.size(); i++) {
      if (livePoints[i] != previousLivePoints[i] ||
          displayPoints[i] != previousDisplayPoints[i]) {
        return true;
      }
    }

    return false;
  }

  // Check if mask points have changed
  bool maskPointsChanged() {
    if (maskPoints.size() != previousMaskPoints.size()) {
      ofLog() << "maskPointsChanged: size changed from " << previousMaskPoints.size() 
              << " to " << maskPoints.size();
      return true;
    }

    for (size_t i = 0; i < maskPoints.size(); i++) {
      if (maskPoints[i] != previousMaskPoints[i]) {
        ofLog() << "maskPointsChanged: point " << i << " changed from " 
                << previousMaskPoints[i].x << "," << previousMaskPoints[i].y 
                << " to " << maskPoints[i].x << "," << maskPoints[i].y;
        return true;
      }
    }

    return false;
  }

  void reallocateFbos() {
    ofLog() << "Reallocating FBOs for shard: " << name<<" with bounding box: " << boundingBox;
    fbo_settings.width = boundingBox.width;  // 4K resolution
    fbo_settings.height = boundingBox.height;
    fbo_settings.internalformat = GL_RGBA;
    warpFbo.allocate(fbo_settings);
    maskFbo.allocate(fbo_settings);
    customFbo.allocate(fbo_settings); 
    customFbo_masked.allocate(fbo_settings);
  }
  void updatePointCache() {
    previousLivePoints = livePoints;
    previousDisplayPoints = displayPoints;
    
    // Check if mask points changed before updating the cache
    bool maskChanged = maskPointsChanged();
    previousMaskPoints = maskPoints;
    
    // Set the flag if points changed
    if (maskChanged) {
      this->maskChanged = true;
      ofLog() << "Mask points changed for shard: " << name << " - Setting maskChanged flag";
    }
  }

  // Create mask from perimeter points
  void createMask() {
    if (maskPoints.size() < 3) {
      cout << "Need at least 3 points to create a mask" << endl;
      maskReady = false;
      return;
    }

    if (!isHomographyValid()) {
      cout << "Cannot create mask: invalid homography" << endl;
      maskReady = false;
      return;
    }

    ofLog() << "Creating mask for shard: " << name << " with " << maskPoints.size() << " points";
    
    // Transform and cache all points using inverseHomography to get display space coordinates
    transformedMaskPoints.clear();

    for (const auto& point : maskPoints) {
      transformedMaskPoints.push_back(transformPoint(point, inverseHomography));
    }
      
    // ofLog() << "Transformed " << transformedMaskPoints.size() << " mask points";

    // Determine which quadrant this shard belongs to based on its centroid
    ofVec2f minPoint(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    ofVec2f maxPoint(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    
    for (const auto& point : transformedMaskPoints) {
        minPoint.x = std::min(minPoint.x, point.x);
        minPoint.y = std::min(minPoint.y, point.y);
        maxPoint.x = std::max(maxPoint.x, point.x);
        maxPoint.y = std::max(maxPoint.y, point.y);
    }
     
    // boundingBox = ofRectangle(minPoint.x, minPoint.y, maxPoint.x - minPoint.x, maxPoint.y - minPoint.y);
    // ofLog() << name << " boundingBox " << boundingBox;
    
//    if (boundingBox != prev_boundingBox) {
      
      // if(bUseBoundingBox) {
      //     fbo_settings.width = boundingBox.width;
      //     fbo_settings.height = boundingBox.height;
      //     fbo_settings.internalformat = GL_RGBA;
      // } else {
              fbo_settings.width = 3840;
              fbo_settings.height = 2160;
              fbo_settings.internalformat = GL_RGBA;
      // }
      
      if(warpFbo.getWidth() != fbo_settings.width){
          warpFbo.allocate(fbo_settings);
          maskFbo.allocate(fbo_settings);
          customFbo.allocate(fbo_settings);
          customFbo_masked.allocate(fbo_settings);
      }
      
      prev_boundingBox = boundingBox;
//    }

    // Update mask FBO
    maskFbo.begin();
    ofClear(0, 0, 0, 0);
    ofSetColor(255);
    ofFill();
    ofBeginShape();
    for (const auto& point : transformedMaskPoints) {
      // if(bUseBoundingBox) {
      //   ofVertex(point.x - boundingBox.x, point.y - boundingBox.y);
      // } else {
        ofVertex(point.x, point.y);
      // }
    }
    ofEndShape(true);
    maskFbo.end();

    // Apply alpha mask based on useMask flag
    if (useMask) {
      ofLog() << "Applying alpha mask for shard: " << name;
      warpFbo.getTexture().setAlphaMask(maskFbo.getTexture());
      customFbo_masked.getTexture().setAlphaMask(maskFbo.getTexture());
    } else {
      ofLog() << "Disabling alpha mask for shard: " << name;
      warpFbo.getTexture().disableAlphaMask();
      customFbo_masked.getTexture().disableAlphaMask();
    }
    
    maskChanged = false;
    maskReady = true;
    
    ofLog() << "Mask created successfully for shard: " << name;
  }

  // Extract 3D orientation from homography matrix
  void extractOrientation3D() {
    if (!homographyReady) {
      ofLog() << "Cannot extract orientation: homography not ready";
      return;
    }
    
    // Create a unit square in the display space
    vector<glm::vec3> squarePoints = {
      glm::vec3(0, 0, 0),
      glm::vec3(1, 0, 0),
      glm::vec3(1, 1, 0),
      glm::vec3(0, 1, 0)
    };
    
    // Transform the square using the homography
    vector<glm::vec3> transformedSquare;
    for (const auto& point : squarePoints) {
      glm::vec4 p(point, 1.0);
      glm::vec4 transformed = homography * p;
      // Perspective division
      if (abs(transformed.w) > 1e-6) {
        transformedSquare.push_back(glm::vec3(
          transformed.x / transformed.w,
          transformed.y / transformed.w,
          transformed.z / transformed.w
        ));
      } else {
        transformedSquare.push_back(point);
      }
    }
    
    // Calculate normal vector from the transformed square
    if (transformedSquare.size() >= 3) {
      glm::vec3 edge1 = transformedSquare[1] - transformedSquare[0];
      glm::vec3 edge2 = transformedSquare[2] - transformedSquare[0];
      glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
      
      // Make sure normal points toward the camera (positive z)
      if (normal.z < 0) {
        normal = -normal;
      }
      
      // Calculate rotation angles from the normal vector
      // X rotation (pitch) - rotation around X axis
      float pitch = glm::degrees(atan2(normal.y, normal.z));
      
      // Y rotation (yaw) - rotation around Y axis
      float yaw = glm::degrees(atan2(normal.x, normal.z));
      
      // Z rotation (roll) - we can estimate this from the transformed square
      glm::vec3 xAxis = glm::normalize(transformedSquare[1] - transformedSquare[0]);
      float roll = glm::degrees(atan2(xAxis.y, xAxis.x));
      
      // Store the orientation
      orientation3D = glm::vec3(pitch, yaw, roll);
      
      // Calculate the centroid of the transformed square for position
      glm::vec3 centroid(0, 0, 0);
      for (const auto& point : transformedSquare) {
        centroid += point;
      }
      centroid /= transformedSquare.size();
      
      // Store the position
      position3D = centroid;
      
      // ofLog() << "Extracted 3D orientation for shard " << name << ": "
      //         << "pitch=" << pitch << ", yaw=" << yaw << ", roll=" << roll;
    }
  }

  // Compute homography if enough points are available and points have changed
  bool computeHomography(bool debug = false) {
    try {
      if (livePoints.size() < 4 || displayPoints.size() < 4) {
        cout << "Not enough points to compute homography. Need at least 4 points." << endl;
        homographyReady = false;
        return false;
      }

      vector<glm::vec3> srcPoints, dstPoints;
      
      // Convert points to glm::vec3
      for(int i = 0; i < 4; i++) {
        srcPoints.push_back(glm::vec3(displayPoints[i].x, displayPoints[i].y, 0));
        dstPoints.push_back(glm::vec3(livePoints[i].x, livePoints[i].y, 0));
        
        if(debug) {
          ofLogNotice() << "Point " << i << " src: " << srcPoints[i].x << "," << srcPoints[i].y 
                       << " dst: " << dstPoints[i].x << "," << dstPoints[i].y;
        }
      }
      
      // Generate homography (4x4 matrix)
      homography = ofxHomography::findHomography(srcPoints,dstPoints);
  
      // Calculate inverse homography
      inverseHomography = glm::inverse(homography);
    
      
      homographyReady = true;
      maskChanged = true;  // Mask needs update when homography changes
      
      // Extract 3D orientation from homography
      extractOrientation3D();
      
      // Determine which quadrant this shard belongs to based on its centroid
      ofVec2f centroid(0, 0);
      for (const auto& point : displayPoints) {
          centroid += point;
      }
      if (!displayPoints.empty()) {
          centroid /= displayPoints.size();
      }
      
      // Determine quadrant based on position in 4K space (3840x2160)
      // A = top-left (red), B = top-right (blue), C = bottom-left (green), D = bottom-right (black)
      float displayWidth = 3840;
      float displayHeight = 2160;
      
      if (centroid.x < displayWidth/2) {
          // Left half
          if (centroid.y < displayHeight/2) {
              // Top-left quadrant - A
              quadrant = 'A';
              quadrantColor = ofColor(255, 0, 0); // Red
              quadrantOffset = ofVec2f(0, 0); // No offset for top-left
          } else {
              // Bottom-left quadrant - C
              quadrant = 'C';
              quadrantColor = ofColor(0, 255, 0); // Green
              quadrantOffset = ofVec2f(0, displayHeight/2); // Offset by half height
          }
      } else {
          // Right half
          if (centroid.y < displayHeight/2) {
              // Top-right quadrant - B
              quadrant = 'B';
              quadrantColor = ofColor(0, 0, 255); // Blue
              quadrantOffset = ofVec2f(displayWidth/2, 0); // Offset by half width
          } else {
              // Bottom-right quadrant - D
              quadrant = 'D';
              quadrantColor = ofColor(0, 0, 0); // Black
              quadrantOffset = ofVec2f(displayWidth/2, displayHeight/2); // Offset by half width and height
          }
      }
      
      if(debug) {
        ofLogNotice() << "Shard " << name << " assigned to quadrant " << quadrant 
                     << " with offset (" << quadrantOffset.x << "," << quadrantOffset.y << ")";
      }
      
      if(debug) {
        ofLogNotice() << "Homography Matrix:";
        for(int i = 0; i < 4; i++) {
          ofLogNotice() << homography[i][0] << " " << homography[i][1] << " "
                       << homography[i][2] << " " << homography[i][3];
        }
      }

    
      return true;

    } catch (const std::exception& e) {
      cout << "Exception during homography computation: " << e.what() << endl;
      homographyReady = false;
      return false;
    }
  }

void updateTransformedMaskPoints(){
      // Transform and cache all points using inverseHomography to get display space coordinates
    transformedMaskPoints.clear();

    for (const auto& point : maskPoints) {
      transformedMaskPoints.push_back(transformPoint(point, inverseHomography));
    }
}
  void update(ofFbo& _fbo) {
    // Update point cache and check for changes
    updatePointCache();
    
    // Check if mask needs to be updated
      maskChanged = true;
    if (maskChanged) {
      ofLog() << "Mask changed for shard: " << name << " - Creating new mask";
      createMask();
    }

    // Copy FBO content instead of reference
    customFbo.begin();
    ofClear(0, 0, 0, 0);
    ofSetColor(255);
    // if(bUseBoundingBox) {
    //   _fbo.getTexture().drawSubsection(0,0,boundingBox.width,boundingBox.height,boundingBox.x, boundingBox.y,boundingBox.width,boundingBox.height);
    // } else {
      _fbo.draw(0, 0);
    // }
    customFbo.end();
    
    customFbo_masked.begin();
    ofClear(0, 0, 0, 0);
    ofSetColor(255);
    // if(bUseBoundingBox) {
    //   _fbo.getTexture().drawSubsection(0,0,boundingBox.width,boundingBox.height,boundingBox.x, boundingBox.y,boundingBox.width,boundingBox.height);
    // } else {
      _fbo.draw(0, 0);
    // }
    customFbo_masked.end();

    ofLog() << "Warping applied successfully for shard: " << name;
  }

  void draw(int x, int y, bool _useInverse) {
    if (!areTransformsValid() || !customFbo.isAllocated()) return;
    
    // Draw warped content to warpFbo
    warpFbo.begin();
    ofClear(0, 0, 0, 0);
    ofPushMatrix();
    
          if(_useInverse) ofMultMatrix(inverseHomography);
          else ofMultMatrix(homography);
    
    
    
    ofSetColor(255);
    // Draw the content
    customFbo.draw(0, 0);
    ofPopMatrix();
    warpFbo.end();
    
    // Draw the result
    ofSetColor(255);
      warpFbo.draw(0, 0);
    
  }

  void draw(int x, int y, int opacity, bool _useInverse) {
    ofSetColor(255, opacity);
    draw(x, y, _useInverse);
  }

  // Save homography and points to files
  bool save(string basePath = "") {
    if (basePath.empty()) {
      basePath = name;
    }

    try {
      // Save calibration points to text file
      ofFile pointsFile(basePath + "_points.txt", ofFile::WriteOnly);
      pointsFile << livePoints.size() << endl;
      for (int i = 0; i < livePoints.size(); i++) {
        pointsFile << livePoints[i].x << " " << livePoints[i].y << endl;
        pointsFile << displayPoints[i].x << " " << displayPoints[i].y << endl;
      }
      pointsFile.close();

      // Save mask points to text file
      ofFile maskFile(basePath + "_mask.txt", ofFile::WriteOnly);
      maskFile << maskPoints.size() << endl;
      for (int i = 0; i < maskPoints.size(); i++) {
        maskFile << maskPoints[i].x << " " << maskPoints[i].y << endl;
      }
      maskFile.close();

      return true;
    } catch (const std::exception& e) {
      cout << "Exception during save for shard " << name << ": " << e.what()
           << endl;
      return false;
    }
  }

  // Load homography and points from files
  bool load(string basePath = "") {
    if (basePath.empty()) {
      basePath = name;
    }

    try {
      bool success = false;

      // Load calibration points from text file first
      string pointsPath = basePath + "_points.txt";
      if (ofFile::doesFileExist(pointsPath)) {
        livePoints.clear();
        displayPoints.clear();

        ofFile pointsFile(pointsPath, ofFile::ReadOnly);
        int numPoints;
        pointsFile >> numPoints;

        float x, y;
        for (int i = 0; i < numPoints; i++) {
          pointsFile >> x >> y;
          livePoints.push_back(ofVec2f(x, y));

          pointsFile >> x >> y;
          displayPoints.push_back(ofVec2f(x, y));
        }
        pointsFile.close();

        // Use the same computeHomography function as manual creation
        if (livePoints.size() >= 4 && displayPoints.size() >= 4) {
        computeHomography(true);  // Pass true for debug output
          success = true;
        }
      }

      // Try to load mask points if they exist
      string maskPath = basePath + "_mask.txt";
      loadMask(maskPath);  // Even if this fails, we still return success if
                           // points were loaded

      // Calculate the homography matrix
      if (computeHomography()) {
          // Calculate the inverse homography matrix
//          inverseHomography = homography.inv();
          homographyReady = true;
      } else {
          homographyReady = false;
      }

      return success;
    } catch (const std::exception& e) {
      cout << "Exception during load for shard " << name << ": " << e.what()
           << endl;
      return false;
    }
  }

  // Load mask points from file
  bool loadMask(string maskPath) {
    try {
      if (ofFile::doesFileExist(maskPath)) {
        maskPoints.clear();

        ofFile maskFile(maskPath, ofFile::ReadOnly);
        int numPoints;
        maskFile >> numPoints;

        float x, y;
        for (int i = 0; i < numPoints; i++) {
          maskFile >> x >> y;
          maskPoints.push_back(ofVec2f(x, y));
        }
        maskFile.close();

        // Use the same createMask function as manual creation
        if (maskPoints.size() >= 3) {
          // Note: You'll need to pass the correct width and height here
          // These should be the dimensions of your live image
        createMask();  // Assuming 4K resolution
        }
        return true;
      }
      return false;
    } catch (const std::exception& e) {
      cout << "Exception during mask load for shard " << name << ": "
           << e.what() << endl;
      return false;
    }
  }

    void drawDebug(int _x, int _y){
           ofPushMatrix();
           ofTranslate(_x,_y);
           ofScale(0.2);
           ofSetColor(255);
           customFbo.draw(0,0);
           warpFbo.draw(customFbo.getWidth(),0);
           customFbo_masked.draw(0,customFbo.getHeight());
           maskFbo.draw(customFbo_masked.getWidth(),customFbo.getHeight());
           
           ofNoFill();
           ofDrawRectangle(0,0,customFbo.getWidth(),customFbo.getHeight());
           ofDrawRectangle(customFbo.getWidth(),0,warpFbo.getWidth(),warpFbo.getHeight());
          ofDrawRectangle(0,customFbo.getHeight(),customFbo_masked.getWidth(),customFbo_masked.getHeight());
           ofDrawRectangle(customFbo_masked.getWidth(),customFbo.getHeight(),maskFbo.getWidth(),maskFbo.getHeight());
         
           ofPushMatrix();
           ofTranslate(0,customFbo.getHeight()*2);
           ofDrawRectangle(0,0,DISPLAY_WIDTH,DISPLAY_HEIGHT);
           ofDrawRectangle(boundingBox.x,boundingBox.y,boundingBox.width,boundingBox.height);
           ofPopMatrix();
           
           ofPopMatrix();
           
       }
    
  void drawTransformedText(const vector<vector<ofPolyline>>& textOutlines,
                           float lineWidth) {
    if (!areTransformsValid()) return;

    ofSetLineWidth(lineWidth);
    for (const auto& outlineGroup : textOutlines) {
      for (const auto& outline : outlineGroup) {
        ofPolyline transformedOutline;
        for (const auto& point : outline.getVertices()) {
          ofVec2f transformedPoint = transformPoint(point, inverseHomography);
          transformedOutline.addVertex(transformedPoint.x, transformedPoint.y);
        }
        transformedOutline.close();
        transformedOutline.draw();
      }
    }
  }

  // Helper function to transform a point using homography matrix
  ofVec2f transformPoint(ofVec2f point, const glm::mat4& matrix) {
    // Check if the matrix is valid before transforming
    float det = glm::determinant(matrix);
    if (abs(det) < 1e-6) {
      return point;  // Return original point if matrix is invalid
    }
    
    // Convert point to homogeneous coordinates
    glm::vec4 p(point.x, point.y, 0, 1);
    
    // Apply transformation
    glm::vec4 transformed = matrix * p;
    
    // Convert back to 2D with perspective division
    if(abs(transformed.w) > 1e-6) {
        return ofVec2f(transformed.x/transformed.w, transformed.y/transformed.w);
    }
    return point;
  }


  void drawMaskOutline(float lineWidth) {
    if (!areTransformsValid() || maskPoints.size() < 3) return;

    ofNoFill();
    ofSetColor(color);
    ofSetLineWidth(lineWidth);
    ofBeginShape();
    // Use transformed points to draw outline in warped space
    for (const auto& point : transformedMaskPoints) {
      ofVertex(point.x, point.y);
    }
    ofEndShape(true);
    ofSetLineWidth(1);
  }

  // Add these helper functions after the pointsChanged() function
  bool isHomographyValid() const {
    if (!homographyReady) return false;
    float det = glm::determinant(homography);
    return abs(det) > 1e-6;
  }

  bool isInverseHomographyValid() const {
    if (!homographyReady) return false;
    float det = glm::determinant(inverseHomography);
    return abs(det) > 1e-6;
  }

  bool areTransformsValid() const {
    return isHomographyValid() && isInverseHomographyValid();
  }

 private:
};


