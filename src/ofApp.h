#pragma once

// Define resolution mode (uncomment one)
#define USE_4K_MODE

#define DISPLAY_WIDTH 3840
#define DISPLAY_HEIGHT 2160

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

#include "OrthoEasyCam.h"

#include "ofxDropdown.h"
#include "cameraHandler.h"

#include "ofxTiming.h"
#include "ofxHomography.h"

#include "maskHandler.h"
#include "mainTracker.h"
#include "pixelMapShader.h"

#include "singleShard.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/gtx/matrix_decompose.hpp>

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void updateAllShardsFbo();
	void forceUpdateAllShards();
	void draw();
	void drawHelp();
	void drawCrossHair(const ofVec2f& point, int size, const ofColor& color, float lineWidth = 1, float rotation = 0, bool showCircle = false);
	
	void mousePressed(int x, int y, int button);
	void mouseDragged(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseMoved(int x, int y);
	void keyPressed(int key);
	void keyReleased(int key);
	void setViewports();

	void checkGui();
	
	void exit();
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	void onFrameDelayChanged(int &value);

	ofEventListeners listeners;

	ofFboSettings fboSettings;
	OrthoEasyCam liveOrthoCam;
	OrthoEasyCam contentOrthoCam;
	OrthoEasyCam displayOrthoCam;
	
	// Add viewport rectangles
	ofRectangle liveViewport;
	ofRectangle contentViewport;
	ofRectangle displayViewport;

	// GUI parameters
	
	ofxPanel gui_main;
		ofParameter<bool> bShowGui{"Show GUI", true};
	ofParameter<bool> bDebug{"Debug", false};
	ofParameter<bool> bShowDebug{"Show Debug", false};
	ofParameter<bool> bShowBuffer{"Show Buffer", false};
    ofParameter<bool> bShowCamera{"Show camera", true};
	ofParameter<bool> bShowOtherViews{"Show Views", true};
	ofParameter<bool> bEditMode{"Edit Mode", false};
	ofParameter<bool> showHelp{"Show Help", true};
	ofParameter<bool> useMasks{"use Masks", true};
	ofParameter<bool> showImage{"Show Image", true};
    vector<ofImage> images;
    ofParameter<int> currentImageIndex{"Current Image Index", 0, 0, 2};
	ofParameter<bool> showAnimation{"Show Animation", true};
	ofParameter<float> shardOpacity{"Shard Opacity", 1, 0, 1};
	ofParameter<float> cameraPosX{"camera X", 0, 0, 1920};
	ofParameter<float> cameraPosY{"camera Y", 0, 0, 1080};
	ofParameter<float> cameraScaler{"camera Scaler", 1, 0.1, 10};
	
	ofxPanel gui_viewport;
	ofParameter<float> liveViewportX{"live X", 0, 0, 1920};
	ofParameter<float> liveViewportY{"live Y", 0, 0, 1080};
	ofParameter<float> liveViewportW{"live Width", 640, 100, 3840};
	ofParameter<float> liveViewportH{"live Height", 480, 100, 2160};
	
	ofParameter<float> contentViewportX{"content X", 0, 0, 1920};
	ofParameter<float> contentViewportY{"content Y", 0, 0, 1080};
	ofParameter<float> contentViewportW{"content Width", 640, 100, 3840};
	ofParameter<float> contentViewportH{"content Height", 480, 100, 2160};

	ofParameter<float> displayViewportX{"display X", 640, 0, 5760};
	ofParameter<float> displayViewportY{"display Y", 0, 0, 5760};
	ofParameter<float> displayViewportW{"display Width", 640, 100, 3840};
	ofParameter<float> displayViewportH{"display Height", 480, 100, 2160};

	// Text parameters
		ofxPanel gui_text;
	ofParameter<bool> showText{"Show Text", true};
	ofParameter<bool> showDebugText{"Show Debug Text", true};
	ofParameter<string> textToDisplay{"Text", "Hello World"};
	ofParameter<float> textSize{"Text Size", 50, 10, 500};
	ofParameter<int> textLineWidth{"Text lineWidth", 2, 1, 20};
	
	ofParameter<float> textX{"Text X", 100, 0, 5000};
	ofParameter<float> textY{"Text Y", 100, 0, 3000};
	ofParameter<int> textAngle{"Text Angle", 0, 0, 360};
	ofParameter<int> textR{"Text R", 255, 0, 255};
	ofParameter<int> textG{"Text G", 255, 0, 255};
	ofParameter<int> textB{"Text B", 255, 0, 255};
	ofParameter<int> textA{"Text A", 255, 0, 255};

	//----------camera-------------
	cameraHandler camera_object;
	mainTracker tracker_object;
	maskHandler mask_object;

	// External display parameters
	ofParameterGroup group_displays;
	ofParameter<bool> showExternal{"Show External", false};
	ofParameter<int> display1X{"Display 1 X", 1920, 0, 5760};
	ofParameter<int> display1Y{"Display 1 Y", 0, 0, 5760};

	ofxPanel gui_face;
	ofParameter<bool> showFace{"Show Face", true};
	ofParameter<float> eyeDistance{"Eye Distance", 640, 100, 3840};
	ofParameter<float> minRotation{"Min Rotation", 0, 0, 360};
	ofParameter<float> maxRotation{"Max Rotation", 90, 0, 360};
	ofParameter<float> eyeScaler{"Eye Scaler", 1, 0.1, 10};
	ofParameter<bool> bLimitScale{"Limit Scale", true};
	ofParameter<bool> applyTransformation{"Apply Transformation", true};
	ofParameter<int> faceX{"Face X", 0, -2000, 2000};
	ofParameter<int> faceY{"Face Y", 0, -2000, 2000};
	ofParameter<bool> bLimitOffset{"Limit Offset", true};
    ofParameter<float> matrixLerp{"matrixLerp", 0.5, 0, 1};
	ofParameter<bool> bUseOpacityChange{"Opacity Change", true};
	ofFbo customContentFbo;
	// FBOs for external display output
	ofMatrix4x4 calcFullAdjusted(ofVec2f avg_leftEye , ofVec2f avg_RightEye ,int _m, bool _useHeadRotation);

    FadeTimer hasFace_fadeTimer;
    Hysteresis hasFace_hysteresis;
    
	pixelMapShader pixelMap_object;
	ofParameter<int> frameDelay{"Frame Delay", 1, 1, 300};
    int prev_frameDelay;
    int current_frameDelay;
	std::vector<ofFbo> frameBuffer;
	std::vector<glm::mat4> matrixBuffer;
	glm::mat4 smoothedFinalMat;
	float matrixBlendAlpha = 0.8;
	int liveFrameIndex = 0;
	float delayedFrameIndex = 0;
	int framesStored = 0;  // Track how many frames have been stored

  string SHARDS_PATH;
	vector<singleShard> shards;
	void loadShards();
private:
};
