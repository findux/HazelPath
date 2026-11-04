#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
//#include "ofxSol.hpp"

#include <unordered_set>

// --------- LuaCoroutineTask wrapper ---------


//struct LuaCoroutineTask {
//	sol::thread thread;         // separate Lua thread for this task
//	sol::coroutine coro;        // coroutine running in that thread
//	std::unordered_map<std::string, sol::object> persistentData;
//	sol::function onStart, onEnd;
//
//	LuaCoroutineTask(sol::state& lua, sol::table taskTable)
//		: thread(sol::thread::create(lua))  // create a new Lua thread
//	{
//		if (sol::table pdata = taskTable["__persistent_state"]; pdata.valid()) {
//			for (auto&& [k, v] : pdata) persistentData[k.as<std::string>()] = v;
//		}
//		recreateFromTable(lua, taskTable);
//	}
//
//	void recreateFromTable(sol::state& lua, sol::table taskTable) {
//		
//		if (coro.valid() && coro.status() == sol::call_status::yielded) {
//			ofLogNotice("LuaCoroutineTask::recreateFromTable() was yielded");
//			taskTable["stop"] = true;
//		} else {
//			ofLogNotice("LuaCoroutineTask::recreateFromTable() was not yielded");
//			taskTable["stop"] = sol::lua_nil;
//		}
//
//		thread.abandon();
//		thread = sol::thread::create(lua);
//		sol::state_view thread_view = thread.state();
//		sol::function f = taskTable["func"];
//
//		ofLogNotice("LuaCoroutineTask::recreateFromTable() new coroutine");
//		coro.abandon();
//		coro = sol::coroutine(thread_view, f);  // recreate coroutine in the same thread
//
//
//		onStart = taskTable["__onStart"];
//		onEnd   = taskTable["__onEnd"];
//		if (onStart.valid()) onStart();
//	}
//
//	sol::object resume(sol::state_view lua) {
//		if (!coro.valid()) return sol::lua_nil;
//
//		sol::state_view thread_view = thread.state();
////		sol::table dataTable = thread_view.create_table();
////		for (auto&& [k, v] : persistentData) dataTable[k] = v;
//
//		ofLogNotice("LuaCoroutineTask::resume()");
//		sol::object result = coro();
//
////		for (auto&& [k, v] : dataTable) persistentData[k.as<std::string>()] = v;
//
//		return result;
//	}
//
//	bool isValid() const { return coro.valid(); }
//	~LuaCoroutineTask() { if (onEnd.valid()) onEnd(); }
//};
//
//struct TaskManager {
//	sol::state& lua;
//	std::unordered_map<std::string, std::unique_ptr<LuaCoroutineTask>> activeTasks;
//	
//	TaskManager(sol::state& L) : lua(L) {}
//	
//	void refresh() {
//		try {
//			
//			sol::table globals = lua.globals();
//			std::unordered_set<std::string> luaTaskNames;
//			
//			for (auto&& [nameObj, valueObj] : globals) {
//				if (valueObj.get_type() != sol::type::table) continue;
//				sol::table taskTable = valueObj.as<sol::table>();
//				
//				sol::object cppTaskFlag = taskTable["__cpp_task"];
//				if (!cppTaskFlag.valid() || !cppTaskFlag.as<bool>()) continue;
//				
//				std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(taskTable.pointer());
//				std::cout << "Address: 0x" << std::hex << addr << "\n";
//				
//
//				std::string name = nameObj.as<std::string>();
//				luaTaskNames.insert(name);
//				
//				auto it = activeTasks.find(name);
//				if (it != activeTasks.end()) {
//					ofLogNotice("recreating task") << name;
//					it->second->recreateFromTable(lua, taskTable);
//				} else {
//					ofLogNotice("creating task") << name;
//					activeTasks[name] = std::make_unique<LuaCoroutineTask>(lua, taskTable);
//				}
//			}
//			
//			
//			// Remove tasks no longer present in Lua using modern erase_if
//			std::erase_if(activeTasks, [&](auto&& pair) {
//				return !luaTaskNames.contains(pair.first);
//			});
//			
//			
//			
//		} catch (const sol::error& e) {
//			std::cerr << "Lua script error: " << e.what() << std::endl;
//		}
//	}
//	
//	void updateTasks() {
//		for (auto& [name, task] : activeTasks) {
//			if (task->isValid()) {
//				sol::object result = task->resume(lua);
//				if (result != sol::lua_nil) {
//					task->persistentData["lastYield"] = result;
//					std::cout << "Coroutine [" << name << "] yielded: " << result.as<int>() << std::endl;
//					std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(task->coro.pointer());
//					std::cout << "Coro Address: 0x" << std::hex << addr << "\n";
//
//				}
//			}
//		}
//	}
//};
//
//
//
//#include "TextEditor.h"
//struct ofxImGuiTextEditorSettings {
//	TextEditor::LanguageDefinition languageDefinition = TextEditor::LanguageDefinition::Lua();
//};
//
//class ofxImGuiTextEditor {
//	TextEditor editor_;
//	
//public:
//	auto setup(ofxImGuiTextEditorSettings settings = ofxImGuiTextEditorSettings()) {
//		editor_.SetLanguageDefinition(settings.languageDefinition);
//	}
//	auto set(const std::string & text) { editor_.SetText(text); }
//	std::string get() { return editor_.GetText(); }
//	
//	auto errorMarkers(const TextEditor::ErrorMarkers& markers) { editor_.SetErrorMarkers(markers) ;}
//	auto draw(std::string name="Editor") { return editor_.Render(name.c_str()); }
//	
//};

#include "ofxImGuiTextEditor.h"

class ofApp : public ofBaseApp{
	ofxImGui::Gui gui;
//	ofxSol luaRunner;
	
	ofxImGuiTextEditor editor_;
//	std::unique_ptr<TaskManager> task_manager_;
	
	
	ImFont* customFont;
	
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	
	void keyPressed(int key) override;
	void keyReleased(int key) override;
	void mouseMoved(int x, int y ) override;
	void mouseDragged(int x, int y, int button) override;
	void mousePressed(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;
	void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
	void mouseEntered(int x, int y) override;
	void mouseExited(int x, int y) override;
	void windowResized(int w, int h) override;
	void dragEvent(ofDragInfo dragInfo) override;
	void gotMessage(ofMessage msg) override;
	
};
