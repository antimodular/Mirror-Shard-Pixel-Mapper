#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

bool bDebug_timing = false;

void ofApp::setup() {
  SHARDS_PATH = "shards-4k";

  ofSetVerticalSync(true);
  ofSetBackgroundColor(0);
  ofDisableArbTex();  // Use normalized texture coordinates

  loadShards();

  // Setup FBOs with consistent settings
  fboSettings.width = DISPLAY_WIDTH;
  fboSettings.height = DISPLAY_HEIGHT;
  fboSettings.internalformat =
      GL_RGBA;                    // Use float format for better precision
  fboSettings.useDepth = true;    // Disable depth since we don't need it
  fboSettings.useStencil = true;  // Disable stencil since we don't need it
  fboSettings.numSamples =
      4;  // Disable multisampling for direct texture access
  fboSettings.textureTarget = GL_TEXTURE_2D;  // Use regular 2D textures

  if (customContentFbo.isAllocated()) {
    customContentFbo.clear();
  }
  customContentFbo.allocate(fboSettings);
  // Set high-quality texture filtering
  customContentFbo.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
  customContentFbo.getTexture().setTextureWrap(GL_CLAMP_TO_EDGE,
                                               GL_CLAMP_TO_EDGE);

  // Clear the FBO
  customContentFbo.begin();
  ofClear(0, 0, 0, 0);
  customContentFbo.end();

  ofDirectory dir("images");
  dir.allowExt("png");
  dir.allowExt("jpg");
  dir.allowExt("jpeg");
  dir.allowExt("gif");
  dir.allowExt("bmp");
  dir.allowExt("tiff");
  dir.listDir();
  dir.sort();
  for (int i = 0; i < dir.size(); i++) {
    images.emplace_back();
    images.back().load(dir.getPath(i));
    ofLog() << "Loaded image " << i << ", " << dir.getPath(i);
  }
  ofLog() << "Loaded " << images.size() << " images";

  // Setup GUI
  gui_main.setup("shader mapper", "GUIs/gui_main.json");
  gui_main.setHeaderBackgroundColor(ofColor(255, 0, 0));
  gui_main.add(bShowGui);
  gui_main.add(bDebug);
  gui_main.add(bShowDebug);
  gui_main.add(bShowBuffer);
  gui_main.add(bShowCamera);
  gui_main.add(bShowOtherViews);

  gui_main.add(showImage);
  gui_main.add(currentImageIndex);
  currentImageIndex.setMax(images.size() - 1);
  gui_main.add(cameraPosX);
  gui_main.add(cameraPosY);
  gui_main.add(cameraScaler);
  // Add external display parameters to GUI
  group_displays.setName("External Displays");
  group_displays.add(showExternal);
  group_displays.add(display1X);
  group_displays.add(display1Y);
  gui_main.add(group_displays);

  gui_main.loadFromFile("GUIs/gui_main.json");
  gui_main.setPosition(10, 10);

  //-----masking-----
  mask_object.setup(0, 3);

  mask_object.init();

  // Setup camera
  camera_object.setup("video camera");
  camera_object.gui_vidSource.setPosition(gui_main.getShape().getRight() + 5,
                                          gui_main.getShape().getTop());
  camera_object.gui_vidTransform.setPosition(
      camera_object.gui_vidSource.getShape().getLeft(),
      camera_object.gui_vidSource.getShape().getBottom() + 5);

  mask_object.gui_mask.setPosition(
      camera_object.gui_vidTransform.getShape().getLeft(),
      camera_object.gui_vidTransform.getShape().getBottom() + 5);

  //------faces or blob tracking--------
  tracker_object.setup(camera_object, mask_object);
  tracker_object.gui_tracking.setPosition(
      camera_object.gui_vidSource.getShape().getRight() + 5,
      camera_object.gui_vidSource.getShape().getTop());
  tracker_object.init();

#ifdef USE_UVC
  camera_object.usbCam_object.uvcController.gui_UVCcontroller.setPosition(
      tracker_object.gui_tracking.getShape().getRight() + 5,
      tracker_object.gui_tracking.getShape().getTop());

#endif

  gui_viewport.setup("viewport", "GUIs/gui_viewport.json");
  gui_viewport.setHeaderBackgroundColor(ofColor(255, 0, 0));

  // Add viewport parameters to GUI
  gui_viewport.add(liveViewportX);
  gui_viewport.add(liveViewportY);
  gui_viewport.add(liveViewportW);
  gui_viewport.add(liveViewportH);

  gui_viewport.add(contentViewportX);
  gui_viewport.add(contentViewportY);
  gui_viewport.add(contentViewportW);
  gui_viewport.add(contentViewportH);

  gui_viewport.add(displayViewportX);
  gui_viewport.add(displayViewportY);
  gui_viewport.add(displayViewportW);
  gui_viewport.add(displayViewportH);
  gui_viewport.loadFromFile("GUIs/gui_viewport.json");
  gui_viewport.setPosition(gui_main.getShape().getLeft(),
                           gui_main.getShape().getBottom() + 5);

  // Initialize orthographic cameras
  liveOrthoCam.enableOrtho();
  displayOrthoCam.enableOrtho();
  liveOrthoCam.enableMouse();
  displayOrthoCam.enableMouse();

  contentOrthoCam.enableOrtho();
  contentOrthoCam.enableMouse();
  // Set initial viewport positions
  setViewports();

  // Setup viewport listeners
  listeners.push(liveViewportX.newListener([&](float &) { setViewports(); }));
  listeners.push(liveViewportY.newListener([&](float &) { setViewports(); }));
  listeners.push(liveViewportW.newListener([&](float &) { setViewports(); }));
  listeners.push(liveViewportH.newListener([&](float &) { setViewports(); }));

  listeners.push(
      contentViewportX.newListener([&](float &) { setViewports(); }));
  listeners.push(
      contentViewportY.newListener([&](float &) { setViewports(); }));
  listeners.push(
      contentViewportW.newListener([&](float &) { setViewports(); }));
  listeners.push(
      displayViewportX.newListener([&](float &) { setViewports(); }));
  listeners.push(
      displayViewportY.newListener([&](float &) { setViewports(); }));
  listeners.push(
      displayViewportW.newListener([&](float &) { setViewports(); }));
  listeners.push(
      displayViewportH.newListener([&](float &) { setViewports(); }));

  gui_face.setup("face", "GUIs/gui_face.json");
  gui_face.setHeaderBackgroundColor(ofColor(255, 0, 0));
  gui_face.setPosition(gui_viewport.getShape().getLeft(),
                       gui_viewport.getShape().getBottom() + 5);
 
  gui_face.add(showFace);
  gui_face.add(applyTransformation);
  gui_face.add(bLimitScale);
  gui_face.add(eyeScaler);
  gui_face.add(eyeDistance);
  gui_face.add(faceX);
  gui_face.add(faceY);
  gui_face.add(bLimitOffset);
  gui_face.add(minRotation);
  gui_face.add(maxRotation);
  gui_face.add(frameDelay);
  gui_face.add(matrixLerp);
  gui_face.add(bUseOpacityChange);
   gui_face.add(shardOpacity);
  gui_face.loadFromFile("GUIs/gui_face.json");

  pixelMap_object.setup();

  pixelMap_object.gui_pixelMap.setPosition(gui_face.getShape().getLeft(),
                                           gui_face.getShape().getBottom() + 5);

  hasFace_fadeTimer.setLength(3, 3);
  hasFace_hysteresis.setDelay(0.5, 0.5);

  frameDelay = max(1, frameDelay.get());
  prev_frameDelay = frameDelay;
  ofLog() << "Initialize frame buffer frameDelay " << frameDelay;

  frameDelay.addListener(this, &ofApp::onFrameDelayChanged);
  int initialDelay = frameDelay.get();
  onFrameDelayChanged(initialDelay);
  liveFrameIndex = -1;
  current_frameDelay = frameDelay;
}

void ofApp::update() {
  float updateStartTime = ofGetElapsedTimef();

  ofSetWindowTitle("shader mapper::" + ofToString(ofGetFrameRate(), 2));
  checkGui();

  float cameraUpdateStartTime = ofGetElapsedTimef();
  camera_object.update();
  float cameraUpdateTime = ofGetElapsedTimef() - cameraUpdateStartTime;
  if (cameraUpdateTime > 0.016) {
    if (bDebug_timing)
      ofLog() << "Camera update took " << cameraUpdateTime * 1000 << "ms";
  }

  mask_object.maxRect =
      ofRectangle(0, 0, camera_object.camWidth, camera_object.camHeight);

  float maskUpdateStartTime = ofGetElapsedTimef();
  mask_object.update();
  float maskUpdateTime = ofGetElapsedTimef() - maskUpdateStartTime;
  if (maskUpdateTime > 0.016) {
    if (bDebug_timing)
      ofLog() << "Mask update took " << maskUpdateTime * 1000 << "ms";
  }

  float trackerUpdateStartTime = ofGetElapsedTimef();
  tracker_object.update();
  float trackerUpdateTime = ofGetElapsedTimef() - trackerUpdateStartTime;
  if (trackerUpdateTime > 0.016) {
    if (bDebug_timing)
      ofLog() << "Tracker update took " << trackerUpdateTime * 1000 << "ms";
  }

  if (camera_object.isFrameNew == true) {
    float frameProcessingStartTime = ofGetElapsedTimef();

    // Store current frame and matrix in buffer

    auto &followers = tracker_object.tracker.getFollowers();
    int index = tracker_object.oldest_followIndex;

    //        float camWidth = camera_object.vidSource_img.getWidth();
    //        float camHeight = camera_object.vidSource_img.getHeight();
    float camWidth = camera_object.vidSource_mat.cols;
    float camHeight = camera_object.vidSource_mat.rows;

    glm::mat4 newMat = glm::mat4(1.0f);  // Initialize as identity matrix
    // Draw the camera content

    // if showFace is true, then draw the face, we detect a face and it is being
    // tracked
    if (showFace && followers.size() > 0 && index != -1) {
      float faceProcessingStartTime = ofGetElapsedTimef();

      //        followers[index].draw();

      // Get normalized eye positions
      ofVec2f avg_leftEye =
          followers[index].getLeftEyeCentroid();  //.getCentroid2D();
      ofVec2f avg_rightEye =
          followers[index].getRightEyeCentroid();  //.getCentroid2D();

      //   ofLog() << "avg_leftEye " << avg_leftEye << " avg_rightEye "
      //           << avg_rightEye;
      // Get camera dimensions

      // Scale normalized positions to camera dimensions
      //        avg_leftEye.x *= camWidth;
      //        avg_leftEye.y *= camHeight;
      //        avg_rightEye.x *= camWidth;
      //        avg_rightEye.y *= camHeight;

      //        ofVec2f midEye = avg_leftEye.getMiddle(avg_rightEye);
      float temp_rotation =
          (avg_leftEye - avg_rightEye).angle(ofVec2f(1, 0));  // ofVec2f(_m,_f)
      temp_rotation = abs(temp_rotation);
      //   ofLog() << "temp_rotation " << temp_rotation;
      if (ofInRange(temp_rotation, minRotation, maxRotation) == true) {
        ofVec2f midEyes = avg_leftEye.getMiddle(avg_rightEye);

        // Calculate minimum scale to ensure video fills FBO
        float minScaleX = customContentFbo.getWidth() / float(camWidth);
        float minScaleY = customContentFbo.getHeight() / float(camHeight);
        float minScale = std::max(minScaleX, minScaleY);
        // if cam and fbo are the same size, then minScale is 1

        // Calculate eye-based scale
        float currentEyeDistance = avg_leftEye.distance(avg_rightEye);
        float targetScale = eyeDistance / currentEyeDistance;

        // Use the larger of minScale and targetScale to ensure FBO is filled
        float finalScale = std::max(targetScale, minScale);
        if (bLimitScale) {
          eyeScaler = finalScale;
        } else {
          eyeScaler = targetScale;
        }

        // Calculate scaled dimensions
        float scaledWidth = camWidth * eyeScaler;
        float scaledHeight = camHeight * eyeScaler;

        // First apply non-limiting transformations
        glm::mat4 stagePositionMat = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(customContentFbo.getWidth() / 2 + faceX,
                      customContentFbo.getHeight() / 2 + faceY, 0.0f));

        glm::mat4 scaleMat =
            glm::scale(glm::mat4(1.0f), glm::vec3(eyeScaler, eyeScaler, 1.0f));

        glm::mat4 eyeCenteringMat = glm::translate(
            glm::mat4(1.0f), glm::vec3(-midEyes.x, -midEyes.y, 0.0f));

        glm::mat4 nonLimitingMat =
            stagePositionMat * scaleMat * eyeCenteringMat;

        // Calculate the offset needed to ensure video extends beyond FBO edges
        if (bLimitOffset) {
          // Get the transformed corners of the video
          glm::vec4 topLeft = nonLimitingMat * glm::vec4(0, 0, 0, 1);
          glm::vec4 bottomRight =
              nonLimitingMat * glm::vec4(camWidth, camHeight, 0, 1);

          // Calculate how much we need to offset to ensure video extends beyond
          // FBO
          float minX = std::min(topLeft.x, bottomRight.x);
          float maxX = std::max(topLeft.x, bottomRight.x);
          float minY = std::min(topLeft.y, bottomRight.y);
          float maxY = std::max(topLeft.y, bottomRight.y);

          // Calculate offset to ensure video extends beyond FBO edges
          float offsetX = 0;
          float offsetY = 0;

          if (minX > 0) offsetX = -minX;
          if (maxX < customContentFbo.getWidth())
            offsetX = customContentFbo.getWidth() - maxX;
          if (minY > 0) offsetY = -minY;
          if (maxY < customContentFbo.getHeight())
            offsetY = customContentFbo.getHeight() - maxY;

          // Create offset matrix
          glm::mat4 offsetMat = glm::translate(
              glm::mat4(1.0f), glm::vec3(offsetX, offsetY, 0.0f));

          // Apply offset after non-limiting transformations
          newMat = nonLimitingMat * offsetMat;  // Fixed order

        } else {
          newMat = nonLimitingMat;
        }

      }  // end if(ofInRange(temp_rotation,minRotation, maxRotation) == true)

      float faceProcessingTime = ofGetElapsedTimef() - faceProcessingStartTime;
      if (faceProcessingTime > 0.016) {
        if (bDebug_timing)
          ofLog() << "Face processing took " << faceProcessingTime * 1000
                  << "ms";
      }
    }  // end  if (showFace && followers.size() > 0 && index != -1)
    else {
      float minScaleX = customContentFbo.getWidth() / float(camWidth);
      float minScaleY = customContentFbo.getHeight() / float(camHeight);
      float minScale = std::max(minScaleX, minScaleY);
      newMat = glm::scale(glm::mat4(1.0f), glm::vec3(minScale, minScale, 1.0f));
    }  // end else if (showFace && followers.size() > 0 && index != -1)

    if (frameDelay < prev_frameDelay) {
      if (liveFrameIndex > frameDelay) {
        current_frameDelay = prev_frameDelay;
      } else {
        prev_frameDelay = frameDelay;
        current_frameDelay = frameDelay;
      }
    } else {
      current_frameDelay = frameDelay;
    }
    liveFrameIndex = (liveFrameIndex + 1) % current_frameDelay;

    //    smoothedFinalMat = newMat;
    // Apply running average to the transformation matrix
    float matrixSmoothingStartTime = ofGetElapsedTimef();
    if (smoothedFinalMat == glm::mat4(1.0f)) {
      //          ofLog()<<"smoothedFinalMat.isIdentity()";
      smoothedFinalMat = newMat;  // Initialize on first frame
    } else {
      // Decompose matrices into translation, rotation, and scale
      glm::vec3 scale1, translation1, skew1;
      glm::vec4 perspective1;
      glm::quat rotation1;
      glm::decompose(smoothedFinalMat, scale1, rotation1, translation1, skew1,
                     perspective1);

      glm::vec3 scale2, translation2, skew2;
      glm::vec4 perspective2;
      glm::quat rotation2;
      glm::decompose(newMat, scale2, rotation2, translation2, skew2,
                     perspective2);

      // Interpolate each component
      glm::vec3 newTranslation =
          glm::mix(translation1, translation2, 1.0f - matrixLerp);
      glm::quat newRotation =
          glm::slerp(rotation1, rotation2, 1.0f - matrixLerp);
      glm::vec3 newScale = glm::mix(scale1, scale2, 1.0f - matrixLerp);

      // Reconstruct the matrix
      smoothedFinalMat = glm::mat4(1.0f);
      smoothedFinalMat = glm::translate(smoothedFinalMat, newTranslation);
      smoothedFinalMat = smoothedFinalMat * glm::mat4_cast(newRotation);
      smoothedFinalMat = glm::scale(smoothedFinalMat, newScale);
    }
    float matrixSmoothingTime = ofGetElapsedTimef() - matrixSmoothingStartTime;
    if (matrixSmoothingTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "Matrix smoothing took " << matrixSmoothingTime * 1000
                << "ms";
    }

    float frameBufferStartTime = ofGetElapsedTimef();
    frameBuffer[liveFrameIndex].begin();
    ofClear(0, 0, 0, 255);
    ofSetColor(255);
    //        camera_object.vidSource_img.draw(0, 0);
    drawMat(camera_object.vidSource_mat, 0, 0);
    frameBuffer[liveFrameIndex].end();

    // Store the current transformation matrix
    matrixBuffer[liveFrameIndex] = smoothedFinalMat;

    delayedFrameIndex = (liveFrameIndex + 1) % current_frameDelay;
    // Advance frame index

    float frameBufferTime = ofGetElapsedTimef() - frameBufferStartTime;
    if (frameBufferTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "Frame buffer operations took " << frameBufferTime * 1000
                << "ms";
    }

    float customContentStartTime = ofGetElapsedTimef();
    customContentFbo.begin();
    ofClear(0, 0, 0, 255);

    ofSetColor(255);
    if (showImage && images.size() > 0) {
      images[currentImageIndex].draw(0, 0);
    } else {
      ofPushMatrix();
      ofMultMatrix(matrixBuffer[int(delayedFrameIndex)]);
      frameBuffer[int(delayedFrameIndex)].draw(
          0,
          0);  //, customContentFbo.getWidth(), customContentFbo.getHeight());
      ofPopMatrix();
    }
    if (bShowDebug) {
      // Draw moving circles
      static vector<ofVec2f> positions(10);
      static vector<ofVec2f> velocities(10);

      // Initialize positions and velocities once
      static bool initialized = false;
      if (!initialized) {
        for (int i = 0; i < 10; i++) {
          positions[i].set(ofRandom(DISPLAY_WIDTH), ofRandom(DISPLAY_HEIGHT));
          velocities[i].set(ofRandom(-5, 5), ofRandom(-5, 5));
        }
        initialized = true;
      }

      // Update and draw circles
      ofSetColor(255);
      ofFill();
      for (int i = 0; i < 10; i++) {
        positions[i] += velocities[i];

        if (positions[i].x < 0 || positions[i].x > DISPLAY_WIDTH)
          velocities[i].x *= -1;
        if (positions[i].y < 0 || positions[i].y > DISPLAY_HEIGHT)
          velocities[i].y *= -1;

        ofDrawCircle(positions[i].x, positions[i].y, 100);
      }
      ofSetLineWidth(10);
      ofNoFill();
      ofSetColor(ofColor::yellow);
      ofDrawLine(DISPLAY_WIDTH / 2, 0, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT);
      ofDrawLine(0, DISPLAY_HEIGHT / 2, DISPLAY_WIDTH, DISPLAY_HEIGHT / 2);

      ofDrawLine(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
      ofDrawRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
      ofSetLineWidth(1);
    }  // end if debug

    customContentFbo.end();
    float customContentTime = ofGetElapsedTimef() - customContentStartTime;
    if (customContentTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "Custom content FBO operations took "
                << customContentTime * 1000 << "ms";
    }

    float totalFrameProcessingTime =
        ofGetElapsedTimef() - frameProcessingStartTime;
    if (totalFrameProcessingTime > 0.033) {
      ofLog() << "Total frame processing took "
              << totalFrameProcessingTime * 1000 << "ms";
    }
  }

  if (bUseOpacityChange) {
    float opacityStartTime = ofGetElapsedTimef();
    bool hasFace = tracker_object.allUsefullPeopleSize > 0;
    hasFace_hysteresis.update(hasFace);
    hasFace_fadeTimer.update(hasFace_hysteresis);
    static bool prevHasFace = false;
    //        ofLog()<<"hasFace "<<hasFace<<" prevHasFace "<<prevHasFace;

    pixelMap_object.shardOpacity = 1 - hasFace_fadeTimer.get();
    // Detect state change
    //        if(hasFace != prevHasFace) {
    //
    //        }

    prevHasFace = hasFace;
    float opacityTime = ofGetElapsedTimef() - opacityStartTime;
    if (opacityTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "Opacity calculations took " << opacityTime * 1000 << "ms";
    }
  } else {
    pixelMap_object.shardOpacity = shardOpacity;
  }

  float pixelMapStartTime = ofGetElapsedTimef();
  if (showImage && images.size() > 0) {
    pixelMap_object.updateAllShardsFbo(customContentFbo, customContentFbo,
                                       shards);
  } else {
    if (liveFrameIndex >= 0 && frameBuffer.size() > 0 && shards.size() > 0) {
      pixelMap_object.updateAllShardsFbo(customContentFbo,
                                         frameBuffer[liveFrameIndex], shards);
    }
  }
  float pixelMapTime = ofGetElapsedTimef() - pixelMapStartTime;
  if (pixelMapTime > 0.016) {
    if (bDebug_timing)
      ofLog() << "Pixel map shader update took " << pixelMapTime * 1000 << "ms";
  }

  float totalUpdateTime = ofGetElapsedTimef() - updateStartTime;
  if (totalUpdateTime > 0.033) {
    if (bDebug_timing)
      ofLog() << "Total update method took " << totalUpdateTime * 1000 << "ms";
  }
}

void ofApp::draw() {
  ofSetBackgroundColor(0);
  ofClear(0);

  float drawStartTime = ofGetElapsedTimef();

  //------------- MARK: draw live view
  //  liveOrthoCam.setDrawableSize(camera_object.camWidth,
  //  camera_object.camHeight); liveOrthoCam.begin(liveViewport);
  //  ofSetColor(255);
  //  liveOrthoCam.end();

  //  ofNoFill();
  //  ofSetColor(ofColor::yellow);
  //  ofDrawRectangle(liveViewport);
  //  ofDrawBitmapStringHighlight("Live Camera (Mirror View)",
  //                              liveViewport.getBottomLeft(), ofColor::black,
  //                              ofColor::white);

  if (bShowCamera) {
    float cameraViewsStartTime = ofGetElapsedTimef();
    ofPushMatrix();
    ofTranslate(cameraPosX, cameraPosY);
    ofScale(cameraScaler);
    ofSetColor(255);
    camera_object.draw();
    tracker_object.draw();
    mask_object.draw(0, 0);
    ofPopMatrix();

    if (tracker_object.bShowAllTracking == true) {
      ofSetColor(255);
      tracker_object.drawInfo(ofGetWidth() - 250, 20);

      ofPushMatrix();
      ofTranslate(cameraPosX, cameraPosY);
      ofScale(cameraScaler);
      ofScale(tracker_object.vision_object.detectionScale);

      tracker_object.drawFollowers();
      ofPopMatrix();

      tracker_object.drawStaticStuff();
    }
    float cameraViewsTime = ofGetElapsedTimef() - cameraViewsStartTime;
    if (cameraViewsTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "camera views drawing took " << cameraViewsTime * 1000
                << "ms";
    }
  }  // end if(bShowCamera)
  //------------- MARK: draw frameBuffer view
  if (bShowBuffer) {
    float bufferViewStartTime = ofGetElapsedTimef();
    ofPushMatrix();
    ofTranslate(0, 500);
    if (camera_object.camWidth > 1920)
      ofScale(0.01);
    else
      ofScale(0.02);

    int temp_col = 0;
    int temp_row = 0;
    int cnt = 0;
    for (auto &fbo : frameBuffer) {
      ofSetColor(255);
      fbo.draw(temp_col * fbo.getWidth(), temp_row * fbo.getHeight());

      if (cnt == int(delayedFrameIndex)) {
        ofSetColor(ofColor::red);
        ofNoFill();
        ofSetLineWidth(3);
        ofDrawRectangle(temp_col * fbo.getWidth(), temp_row * fbo.getHeight(),
                        fbo.getWidth(), fbo.getHeight());
        ofSetLineWidth(1);
      } else if (cnt == liveFrameIndex) {
        ofSetColor(ofColor::blue);
        ofNoFill();
        ofSetLineWidth(3);
        ofDrawRectangle(temp_col * fbo.getWidth(), temp_row * fbo.getHeight(),
                        fbo.getWidth(), fbo.getHeight());
        ofSetLineWidth(1);
      }

      temp_col++;
      if (temp_col > 15) {
        temp_col = 0;
        temp_row++;
      }
      cnt++;
    }
    ofPopMatrix();
    float bufferViewTime = ofGetElapsedTimef() - bufferViewStartTime;
    if (bufferViewTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "Buffer view drawing took " << bufferViewTime * 1000 << "ms";
    }
  }  // end if(bShowBuffer)

  if (bShowOtherViews) {
    float contentViewStartTime = ofGetElapsedTimef();
    //------------- MARK: draw display view
    contentOrthoCam.setDrawableSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    contentOrthoCam.begin(contentViewport);
    ofSetColor(255);
    customContentFbo.draw(0, 0);
    contentOrthoCam.end();

    ofNoFill();
    ofSetColor(ofColor::yellow);
    ofDrawRectangle(contentViewport);
    ofDrawBitmapStringHighlight("customContentFbo",
                                contentViewport.getBottomLeft(), ofColor::black,
                                ofColor::white);

    //------------- MARK: draw display view
    displayOrthoCam.setDrawableSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    displayOrthoCam.begin(displayViewport);
    ofSetColor(255);

    pixelMap_object.allShardsFbo.draw(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    displayOrthoCam.end();

    ofNoFill();
    ofSetColor(ofColor::yellow);
    ofDrawRectangle(displayViewport);
    ofDrawBitmapStringHighlight("pixelMap result",
                                displayViewport.getBottomLeft(), ofColor::black,
                                ofColor::white);
    float contentViewTime = ofGetElapsedTimef() - contentViewStartTime;
    if (contentViewTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "Content view drawing took " << contentViewTime * 1000
                << "ms";
    }
  }  // end if(bShowOtherViews)
  //------------- MARK: Draw to external display if enabled
  if (showExternal) {
    float externalDisplayStartTime = ofGetElapsedTimef();
    ofPushMatrix();
    ofTranslate(display1X, display1Y);
    ofSetColor(255);
    pixelMap_object.allShardsFbo.draw(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    ofPopMatrix();
    float externalDisplayTime = ofGetElapsedTimef() - externalDisplayStartTime;
    if (externalDisplayTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "External display drawing took "
                << externalDisplayTime * 1000 << "ms";
    }
  }
  ofSetColor(255);
  ofDrawBitmapStringHighlight("fps: " + ofToString(ofGetFrameRate(), 2), mouseX,
                              mouseY);
  // Draw GUI if enabled
  if (bShowGui) {
    float guiStartTime = ofGetElapsedTimef();
    gui_main.draw();
    gui_viewport.draw();
    camera_object.gui_vidSource.draw();
    camera_object.gui_vidTransform.draw();
    tracker_object.gui_tracking.draw();
    mask_object.gui_mask.draw();

#ifdef USE_UVC
    if (camera_object.usbCam_object.uvcController.gui_UVCcontroller.getShape()
            .getLeft() <= 10) {
      camera_object.usbCam_object.uvcController.gui_UVCcontroller.setPosition(
          tracker_object.gui_tracking.getShape().getRight() + 5,
          tracker_object.gui_tracking.getShape().getTop());
    }
    camera_object.usbCam_object.uvcController.gui_UVCcontroller.draw();
#endif
    gui_face.draw();
    pixelMap_object.gui_pixelMap.draw();
    float guiTime = ofGetElapsedTimef() - guiStartTime;
    if (guiTime > 0.016) {
      if (bDebug_timing)
        ofLog() << "GUI drawing took " << guiTime * 1000 << "ms";
    }
  }

  if (bShowDebug) {
    ofSetColor(255);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 20, 20);
  }

  float totalDrawTime = ofGetElapsedTimef() - drawStartTime;
  if (totalDrawTime > 0.033) {
    ofLog() << "Total draw method took " << totalDrawTime * 1000 << "ms";
  }
}

void ofApp::setViewports() {
  liveViewport.set(liveViewportX, liveViewportY, liveViewportW, liveViewportH);
  contentViewport.set(contentViewportX, contentViewportY, contentViewportW,
                      contentViewportH);
  displayViewport.set(displayViewportX, displayViewportY, displayViewportW,
                      displayViewportH);
}

void ofApp::mousePressed(int x, int y, int button) {}

void ofApp::mouseDragged(int x, int y, int button) {}

void ofApp::mouseReleased(int x, int y, int button) {}

void ofApp::mouseMoved(int x, int y) {}

void ofApp::keyReleased(int key) { tracker_object.keyReleased(key); }

void ofApp::keyPressed(int key) {
  if (key == 'g') {
    bShowGui = !bShowGui;
    if (bShowGui == false) {
      gui_main.saveToFile("GUIs/gui_main.json");
      gui_viewport.saveToFile("GUIs/gui_viewport.json");
      //          gui_text.saveToFile("GUIs/gui_text.json");
      camera_object.saveGui();
      tracker_object.gui_tracking.saveToFile("GUIs/gui_tracking.json");
      mask_object.gui_mask.saveToFile("GUIs/gui_mask.json");
      gui_face.saveToFile("GUIs/gui_face.json");
      pixelMap_object.gui_pixelMap.saveToFile("GUIs/gui_pixelMap.json");
    }
  }
  if (key == 'd') bDebug = !bDebug;
  if (key == 'f') ofToggleFullscreen();
}

void ofApp::checkGui() {
  tracker_object.bShowGUI = bShowGui;
  mask_object.bShowGUI = bShowGui;

  tracker_object.mainPosX = cameraPosX;
  tracker_object.mainPosY = cameraPosY;
  tracker_object.mainScaler = cameraScaler;

  mask_object.mainPosX = cameraPosX;
  mask_object.mainPosY = cameraPosY;
  mask_object.mainScaler = cameraScaler;
}

void ofApp::onFrameDelayChanged(int &value) {
  ofLog() << "onFrameDelayChanged value " << value << " frameBuffer.size() "
          << frameBuffer.size();

  if ((value + 2) > frameBuffer.size()) {
    frameBuffer.resize(value + 3);
    matrixBuffer.resize(value + 3);
  }
  for (auto &fbo : frameBuffer) {
    if (!fbo.isAllocated()) {
      ofFboSettings frame_fboSettings;
      frame_fboSettings.width = camera_object.camWidth;
      frame_fboSettings.height = camera_object.camHeight;
      frame_fboSettings.internalformat =
          GL_RGBA;  // Use float format for better precision
      frame_fboSettings.useDepth =
          true;  // Disable depth since we don't need it
      frame_fboSettings.useStencil =
          true;  // Disable stencil since we don't need it
      frame_fboSettings.numSamples =
          4;  // Disable multisampling for direct texture access
      frame_fboSettings.textureTarget =
          GL_TEXTURE_2D;  // Use regular 2D textures

      fbo.allocate(frame_fboSettings);
      //            fbo.allocate(camera_object.camWidth,
      //                         camera_object.camHeight,
      //                         GL_RGBA);
    }
  }
  ofLog() << "onFrameDelayChanged  frameBuffer.size() " << frameBuffer.size();
  //    liveFrameIndex = -1;
  // framesStored = 0;  // Reset counter when delay changes
}

void ofApp::exit() {
  // Save GUI settings
  if (bShowGui) {
    gui_main.saveToFile("GUIs/gui_main.json");
    gui_viewport.saveToFile("GUIs/gui_viewport.json");
    camera_object.saveGui();
    tracker_object.gui_tracking.saveToFile("GUIs/gui_tracking.json");
    mask_object.gui_mask.saveToFile("GUIs/gui_mask.json");
    gui_face.saveToFile("GUIs/gui_face.json");
  }
}

void ofApp::windowResized(int w, int h) {}

void ofApp::mouseEntered(int x, int y) {}

void ofApp::mouseExited(int x, int y) {}

void ofApp::dragEvent(ofDragInfo dragInfo) {}

void ofApp::gotMessage(ofMessage msg) {}

void ofApp::loadShards() {
  // Create shards directory if it doesn't exist
  ofDirectory shardsDir(ofToDataPath(SHARDS_PATH));
  if (!shardsDir.exists()) {
    shardsDir.create(true);
    cout << "Created shards directory" << endl;
    return;
  }

  // Clear existing shards and cache
  shards.clear();
  // Scan the shards directory for all files
  shardsDir.listDir();
  shardsDir.sort();
    ofLog()<<"shardsDir.size() "<<shardsDir.size();
  if (shardsDir.size() == 0) {
    cout << "No files found in shards directory. Creating default shard."
         << endl;
    // createDefaultShard();
    return;
  }

  // Map to store shard names and their files
  map<string, vector<string>> shardFiles;

  // First pass: find all files and organize them by shard name
  for (int i = 0; i < shardsDir.size(); i++) {
    string path = shardsDir.getPath(i);
    string filename = ofFilePath::getFileName(path);
    cout << "Found file: " << filename << endl;

    string shardName = "";
    if (filename.find("_points.txt") != string::npos) {
      shardName = filename.substr(0, filename.find("_points"));
    } else if (filename.find("_mask.txt") != string::npos) {
      shardName = filename.substr(0, filename.find("_mask"));
    }

    if (!shardName.empty()) {
      cout << "File belongs to shard: " << shardName << endl;
      shardFiles[shardName].push_back(path);
    }
  }

  // Now create and load shards based on found files
  for (auto const &[shardName, files] : shardFiles) {
    bool hasPoints = false;
    bool hasMaskPoints = false;
    string pointsPath, maskPath;

    // Find the points and mask files
    for (const auto &file : files) {
      string filename = ofFilePath::getFileName(file);
      if (filename.find("_points.txt") != string::npos) {
        hasPoints = true;
        pointsPath = file;
      }
      if (filename.find("_mask.txt") != string::npos) {
        hasMaskPoints = true;
        maskPath = file;
      }
    }

    // Log if points or mask points are missing
    if (!hasPoints) {
      ofLog() << "No homography points found for shard: " << shardName;
    }
    if (!hasMaskPoints) {
      ofLog() << "No mask points found for shard: " << shardName;
    }

    // Only proceed if we have points file
    if (!hasPoints) {
      cout << "Skipping shard " << shardName << " - missing points file"
           << endl;
      continue;
    }

    try {
      singleShard newShard;
      newShard.name = shardName;
      newShard.index = shards.size();
      newShard.color = ofColor((shards.size() * 23 + 311) % 200,
                               (shards.size() * 41 + 431) % 200,
                               (shards.size() * 33 + 197) % 200);

      // Initialize image buffers
      newShard.warpedColor.allocate(DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                    OF_IMAGE_COLOR);

      // Clear the image buffers
      ofPixels &warpedPixels = newShard.warpedColor.getPixels();
      //          ofPixels &inversePixels =
      //          newShard.inverseWarpedColor.getPixels();
      warpedPixels.setColor(ofColor(0, 0, 0));
      //          inversePixels.setColor(ofColor(0, 0, 0));
      newShard.warpedColor.update();
      //          newShard.inverseWarpedColor.update();

      // Load the shard data (this will compute homography from points)
      if (!newShard.load(ofToDataPath(SHARDS_PATH + "/" + shardName))) {
        cout << "Failed to load shard: " << shardName << endl;
        continue;
      }

      // Only apply warping if homography is valid
      if (newShard.homographyReady) {
        try {
          // Transform mask points using the homography
          newShard.transformedMaskPoints.clear();
          for (const auto &point : newShard.maskPoints) {
            ofPoint transformedPoint =
                newShard.transformPoint(point, newShard.inverseHomography);
            newShard.transformedMaskPoints.push_back(transformedPoint);
          }

          // Ensure quadrant information is set based on the shard's
          // position Calculate centroid to determine which quadrant this
          // shard belongs to
          ofVec2f centroid(0, 0);
          for (const auto &point : newShard.displayPoints) {
            centroid += point;
          }
          if (!newShard.displayPoints.empty()) {
            centroid /= newShard.displayPoints.size();
          }

          // Determine quadrant based on position in 4K space (3840x2160)
          if (centroid.x < DISPLAY_WIDTH / 2) {
            // Left half
            if (centroid.y < DISPLAY_HEIGHT / 2) {
              // Top-left quadrant - A
              newShard.quadrant = 'A';
              newShard.quadrantColor = ofColor(255, 0, 0);  // Red
              newShard.quadrantOffset =
                  ofVec2f(0, 0);  // No offset for top-left
            } else {
              // Bottom-left quadrant - C
              newShard.quadrant = 'C';
              newShard.quadrantColor = ofColor(0, 255, 0);  // Green
              newShard.quadrantOffset =
                  ofVec2f(0, DISPLAY_HEIGHT / 2);  // Offset by half height
            }
          } else {
            // Right half
            if (centroid.y < DISPLAY_HEIGHT / 2) {
              // Top-right quadrant - B
              newShard.quadrant = 'B';
              newShard.quadrantColor = ofColor(0, 0, 255);  // Blue
              newShard.quadrantOffset =
                  ofVec2f(DISPLAY_WIDTH / 2, 0);  // Offset by half width
            } else {
              // Bottom-right quadrant - D
              newShard.quadrant = 'D';
              newShard.quadrantColor = ofColor(0, 0, 0);  // Black
              newShard.quadrantOffset = ofVec2f(
                  DISPLAY_WIDTH / 2,
                  DISPLAY_HEIGHT / 2);  // Offset by half width and height
            }
          }

          ofLog() << "Shard " << newShard.name << " assigned to quadrant "
                  << newShard.quadrant;

          // Extract 3D orientation from homography
          newShard.extractOrientation3D();
          // ofLog() << "Extracted 3D orientation for shard " <<
          // newShard.name
          //         << ": "
          //         << "pitch=" << newShard.orientation3D.x
          //         << ", yaw=" << newShard.orientation3D.y
          //         << ", roll=" << newShard.orientation3D.z;

          //              newShard.update(customFbo);
          cout << "Applied warping with custom image for loaded shard: "
               << shardName << endl;
          cout << "Transformed " << newShard.transformedMaskPoints.size()
               << " mask points for shard: " << shardName << endl;
        } catch (const std::exception &e) {
          cout << "Warning: Failed to apply warping for shard " << shardName
               << ": " << e.what() << endl;
        }
      }

      shards.push_back(newShard);

    } catch (const std::exception &e) {
      cout << "Exception while processing shard " << shardName << ": "
           << e.what() << endl;
    }
  }

  // If no shards were loaded, create a default one
  if (shards.empty()) {
    cout << "No valid shards found. Creating default shard." << endl;
    // createDefaultShard();
  }
}
