#include "ofApp.h"

void ofApp::setup() {
	ofBackground(30);
	program = nullptr;

	ofFileDialogResult result = ofSystemLoadDialog("Select a G-code file");
	if (!result.bSuccess) return;

	string contents = ofBufferFromFile(result.getPath()).getText();
	if (contents.empty()) return;

	program = new gpr::gcode_program(gpr::parse_gcode(contents));

	ofPolyline currentLine;
	float cx = 0, cy = 0, cz = 0;

	for (auto& block : *program) {
		int gCode = -1;
		float x = cx, y = cy, z = cz;

		for (auto& chunk : block) {
			if (chunk.tp() == gpr::CHUNK_TYPE_WORD_ADDRESS) {
				char word = chunk.get_word();
				gpr::addr address = chunk.get_address();

				if (word == 'G') {
					gCode = address.int_value();
				} else if (word == 'X') {
					x = (float)address.double_value();
				} else if (word == 'Y') {
					y = (float)address.double_value();
				} else if (word == 'Z') {
					z = (float)address.double_value();
				}
			}
		}

		if (gCode == 0) {
			if (currentLine.size() > 1) {
				toolpaths.push_back(currentLine);
			}
			currentLine.clear();
			currentLine.addVertex(x, y, z);
		} else if (gCode == 1) {
			currentLine.addVertex(x, y, z);
		}

		cx = x;
		cy = y;
		cz = z;
	}

	if (currentLine.size() > 1) {
		toolpaths.push_back(currentLine);
	}
}

void ofApp::draw() {
	cam.begin();

	ofSetColor(0, 255, 100);
	for (auto& line : toolpaths) {
		line.draw();
	}

	ofSetColor(255, 0, 0);
	ofDrawSphere(0, 0, 0, 1);

	cam.end();

	ofSetColor(255);
	ofDrawBitmapString("ofxGpr - G-code Viewer", 20, 20);
	ofDrawBitmapString("Toolpaths: " + ofToString(toolpaths.size()), 20, 40);
	if (program) {
		ofDrawBitmapString("Blocks: " + ofToString(program->num_blocks()), 20, 60);
	}
}
