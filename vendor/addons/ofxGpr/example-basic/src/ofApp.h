#pragma once

#include "ofMain.h"
#include "ofxGpr.h"

class ofApp : public ofBaseApp {
public:
	void setup();
	void draw();

	gpr::gcode_program* program;
	vector<ofPolyline> toolpaths;
	ofEasyCam cam;
};
