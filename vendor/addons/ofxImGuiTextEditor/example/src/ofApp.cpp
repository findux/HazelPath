#include "ofApp.h"

#include <iostream>

//--------------------------------------------------------------
void ofApp::setup(){
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	
//	task_manager_ = std::make_unique<TaskManager>(luaRunner.lua_);

	
	
	
//	static ImWchar glyph_ranges[] = {
//		0x0020, 0x00FF, // Latin-1
//		0x0100, 0x017F, // Latin Extended-A
//		0x2000, 0x206F, // General Punctuation (–, —, …)
//		0x20A0, 0x20CF, // Currency Symbols (€, ₣)
//		0
//	};	
//	ImFontConfig font_config;
//	font_config.OversampleH = 2;
//	font_config.OversampleV = 2;
//	font_config.PixelSnapH = true;

//	customFont = io.Fonts->AddFontFromFileTTF(&ofToDataPath("fonts/DejaVuSans.ttf").c_str()[0], 16.0f);
//	if (!customFont) {
//		ofLogError() << "Failed to load font!";
//	} else {
//		ofLogNotice() << "Font loaded successfully.";
//	}
//	io.Fonts->Build();
//	io.FontDefault = customFont;
	
	gui.setup();

	ofxImGuiTextEditorSettings settings = {
		.languageDefinition = TextEditor::LanguageDefinition::Lua()
	};

	/*
	editor_.setup(settings);
	editor_.set(R"(

function registerTask(f)
	if type(f) ~= "function" then
		error("registerTask: argument must be a function")
	end

	local task = {}
	task.__cpp_task = true
	task.__persistent_state = { counter = 0, lastResult = nil }
	task.__onStart = function() print("Task started") end
	task.__onEnd   = function() print("Task ended") end

	task.yield_or_return = function(value)
		if task.stop then
			task.stop = nil
			print("======= INTERRUPT ========")
			return value
		else
			return coroutine.yield(value)
		end
	end

	-- Wrap user function so it gets `task`
	task.func = function()
		return f(task)
	end

	return task
end

-- Example coroutine task
myRoutine = registerTask(function(data)
	local state = data or {}
	local counter = state.counter or 0
	while true do
		counter = counter + 1
		state.counter = counter
		state.lastResult = counter
		print("Lua: myRoutine")
--  coroutine.yield(counter)  -- yield back to C++
  yield_or_return(counter)  -- yield back to C++
	end
end)

anotherRoutine = registerTask(function(data)
	local sum = data.sum or 0
	for i = 1, 10 do
		sum = sum + i
		data.sum = sum
		print("Lua: anotherRoutine")
		yield_or_return(sum)
	end
end)


print("OK")

)");
	 
	 */
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	gui.begin();
	ImGui::Begin("Lua Script Editor");

	/*
	if (ImGui::Button("▶ Run")) {
		std::string code = editor_.get();
		std::scoped_lock lock(luaRunner.reload_lock_);
		try {
			luaRunner.lua_.safe_script(code);
			luaRunner.sol_lua_validity_ = true;

			// Clear error markers
			editor_.errorMarkers({});
			task_manager_->refresh();

		} catch (std::exception &e) {
			luaRunner.sol_lua_validity_ = false;
			luaRunner.sol_lua_error_ = e.what();

			// Try to extract a line number from the error string
			std::smatch match;
			std::regex line_error_pattern(":(\\d+):");

			std::map<int, std::string> markers;

			if (std::regex_search(luaRunner.sol_lua_error_, match, line_error_pattern)) {
				int line = std::stoi(match[1]);
				markers[line] = luaRunner.sol_lua_error_;
			}
			editor_.errorMarkers(markers);
		}
	}
	*/
	ImGui::PushFont(customFont);
	editor_.draw("LuaEditor");
	ImGui::PopFont();
	
	ImGui::End();
	gui.end();
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
//	task_manager_->updateTasks();
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
