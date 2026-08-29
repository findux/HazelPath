#include "ofMain.h"
#include "ofApp.h"

#ifdef _WIN32
#include <Windows.h>
#include <cstdio>
#include <cstring>

namespace {
void openConsoleWhenRequested(int argc, char* argv[]) {
	bool requested = false;
	for (int i = 1; i < argc; ++i) {
		if (std::strcmp(argv[i], "--console") == 0) {
			requested = true;
			break;
		}
	}

	if (!requested || !AllocConsole()) {
		return;
	}

	FILE* stream = nullptr;
	freopen_s(&stream, "CONIN$", "r", stdin);
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
}
}
#endif

//========================================================================
int main(int argc, char* argv[]){
#ifdef _WIN32
	openConsoleWhenRequested(argc, argv);
#endif

	//Use ofGLFWWindowSettings for more options like multi-monitor fullscreen
	ofGLWindowSettings settings;
	settings.setSize(1024, 768);
	settings.title = "HazelPath";
	settings.windowMode = OF_WINDOW; //can also be OF_FULLSCREEN

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();

}
