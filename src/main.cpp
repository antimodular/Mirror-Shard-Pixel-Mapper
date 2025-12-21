// #include "ofMain.h"
// #include "ofApp.h"

// //========================================================================
// int main( ){

// #ifdef OF_TARGET_OPENGLES
// 	ofGLESWindowSettings settings;
// 	settings.glesVersion=2;
// #else
// 	ofGLWindowSettings settings;
// 	settings.setGLVersion(3,2);
// #endif

// 	auto window = ofCreateWindow(settings);

// 	ofRunApp(window, make_shared<ofApp>());
// 	ofRunMainLoop();

// }

#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGLFWWindow.h"

//set file type to Objective-C++ Source

//========================================================================
int main( ){

    ofGLFWWindowSettings settings;
    settings.setGLVersion(2, 1);    // Use OpenGL 2.1 for line width support
    settings.windowMode = OF_WINDOW;
    settings.decorated = true;
    //     settings.setGLVersion(4,0);
    //     settings.setGLVersion(3,2);
    //        settings.windowMode = OF_WINDOW; //OF_FULLSCREEN;
    settings.setSize(1730,900);
    //    settings.setPosition(ofVec2f(0,0));
    settings.resizable = false;

    settings.multiMonitorFullScreen = true;
    ofCreateWindow(settings);
    ofRunApp(new ofApp);

}

