#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main() {
    // Set up the OpenGL window.
    // This function takes the width, height, and display mode (windowed, fullscreen, etc.)
    // as arguments.
	ofSetupOpenGL(1280, 720, OF_WINDOW);

    // Run the application.
    // This function creates an instance of the ofApp class and starts the openFrameworks
    // main loop, which handles calling setup(), update(), and draw().
	return ofRunApp(new ofApp());
}
