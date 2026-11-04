#include "TextEditor.h"
#include "ofxImgui.h"

struct ofxImGuiTextEditorSettings {
	TextEditor::LanguageDefinition languageDefinition = TextEditor::LanguageDefinition::Lua();
};

class ofxImGuiTextEditor {
	TextEditor editor_;
	
public:
	auto setup(ofxImGuiTextEditorSettings settings = ofxImGuiTextEditorSettings()) {
		editor_.SetLanguageDefinition(settings.languageDefinition);
	}
	auto set(const std::string & text) { editor_.SetText(text); }
	std::string get() { return editor_.GetText(); }
	
	auto errorMarkers(const TextEditor::ErrorMarkers& markers) { editor_.SetErrorMarkers(markers) ;}
	auto draw(std::string name="Editor") { return editor_.Render(name.c_str()); }
	
	auto save(std::string filename) {
		std::ofstream out_file(ofToDataPath(filename));
		if (out_file.is_open()) {
			out_file << editor_.GetText();
		}
	}
	auto load(std::string filename) {
		std::ifstream in_file(ofToDataPath(filename));
		if (in_file) {
			std::stringstream buffer;
			buffer << in_file.rdbuf();
			editor_.SetText(buffer.str());
		}
	}
};
