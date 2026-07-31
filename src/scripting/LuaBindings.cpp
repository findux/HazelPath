#include "LuaBindings.h"

#include "ofxGpr.h"
#include "toolpath/Toolpath.h"

namespace {

double addressNumber(const gpr::addr& address) {
	return address.tp() == gpr::ADDRESS_TYPE_INTEGER
		? static_cast<double>(address.int_value())
		: address.double_value();
}

std::string oneCharacter(char value) {
	return std::string(1, value);
}

} // namespace

namespace nc::lua {

void registerToolpathBindings(sol::state& state) {
	state.set_function("of_log", [](const std::string & message) {
		ofLogNotice("Lua") << message;
	});

	state.new_enum<Units>("ToolpathUnits", {
		{"Millimeters", Units::Millimeters},
		{"Inches", Units::Inches}
	});
	state.new_enum<MotionType>("MotionType", {
		{"Rapid", MotionType::Rapid},
		{"Linear", MotionType::Linear},
		{"ArcClockwise", MotionType::ArcClockwise},
		{"ArcCounterClockwise", MotionType::ArcCounterClockwise},
		{"Dwell", MotionType::Dwell},
		{"ToolChange", MotionType::ToolChange},
		{"Spindle", MotionType::Spindle},
		{"Custom", MotionType::Custom}
	});
	state.new_enum<ArcPlane>("ArcPlane", {
		{"XY", ArcPlane::XY},
		{"XZ", ArcPlane::XZ},
		{"YZ", ArcPlane::YZ}
	});

	state.new_usertype<AxisPosition>("AxisPosition",
		sol::constructors<AxisPosition(), AxisPosition(double, double, double)>(),
		"x", &AxisPosition::x,
		"y", &AxisPosition::y,
		"z", &AxisPosition::z,
		"a", &AxisPosition::a,
		"b", &AxisPosition::b,
		"c", &AxisPosition::c);

	state.new_usertype<ToolpathCommand>("ToolpathCommand",
		sol::constructors<ToolpathCommand()>(),
		"type", &ToolpathCommand::type,
		"start", &ToolpathCommand::start,
		"finish", &ToolpathCommand::end,
		"center", &ToolpathCommand::center,
		"plane", &ToolpathCommand::plane,
		"feed_rate", &ToolpathCommand::feedRate,
		"spindle_speed", &ToolpathCommand::spindleSpeed,
		"dwell_seconds", &ToolpathCommand::dwellSeconds,
		"tool_number", &ToolpathCommand::toolNumber,
		"source_line", &ToolpathCommand::sourceLine,
		"source_text", &ToolpathCommand::sourceText,
		"custom_type", &ToolpathCommand::customType,
		"set_metadata", &ToolpathCommand::setMetadata,
		"get_metadata", &ToolpathCommand::getMetadata);

	state.new_usertype<Toolpath>("Toolpath",
		sol::constructors<Toolpath()>(),
		"units", &Toolpath::units,
		"machine_name", &Toolpath::machineName,
		"source_name", &Toolpath::sourceName,
		"clear", &Toolpath::clear,
		"size", &Toolpath::size,
		"empty", &Toolpath::empty,
		"set_current_position", &Toolpath::setCurrentPosition,
		"get_current_position", [](const Toolpath& path) { return path.getCurrentPosition(); },
		"add_command", &Toolpath::addCommand,
		"add_rapid", [](Toolpath& path, const AxisPosition& end, int line, const std::string& text) {
			path.addRapid(end, line, text);
		},
		"add_linear", [](Toolpath& path, const AxisPosition& end, double feed, int line, const std::string& text) {
			path.addLinear(end, feed, line, text);
		},
		"add_arc", [](Toolpath& path, const AxisPosition& end, const AxisPosition& center,
			bool clockwise, ArcPlane plane, double feed, int line, const std::string& text) {
			path.addArc(end, center, clockwise, plane, feed, line, text);
		},
		"add_dwell", &Toolpath::addDwell,
		"add_tool_change", &Toolpath::addToolChange,
		"add_spindle", &Toolpath::addSpindle,
		"get_command", &Toolpath::getCommand,
		"set_metadata", &Toolpath::setMetadata,
		"get_metadata", &Toolpath::getMetadata);
}

void registerGprBindings(sol::state& state) {
	state.new_enum<gpr::address_type>("GprAddressType", {
		{"Integer", gpr::ADDRESS_TYPE_INTEGER},
		{"Double", gpr::ADDRESS_TYPE_DOUBLE}
	});
	state.new_enum<gpr::chunk_type>("GprChunkType", {
		{"Comment", gpr::CHUNK_TYPE_COMMENT},
		{"WordAddress", gpr::CHUNK_TYPE_WORD_ADDRESS},
		{"Percent", gpr::CHUNK_TYPE_PERCENT},
		{"Word", gpr::CHUNK_TYPE_WORD}
	});

	state.new_usertype<gpr::addr>("GprAddress", sol::no_constructor,
		"type", &gpr::addr::tp,
		"number", &addressNumber,
		"integer_value", [](const gpr::addr& address) {
			return address.tp() == gpr::ADDRESS_TYPE_INTEGER ? address.int_value() : 0;
		},
		"double_value", [](const gpr::addr& address) {
			return address.tp() == gpr::ADDRESS_TYPE_DOUBLE ? address.double_value() : 0.0;
		});

	state.new_usertype<gpr::chunk>("GprChunk", sol::no_constructor,
		"type", &gpr::chunk::tp,
		"word", [](const gpr::chunk& chunk) {
			return chunk.tp() == gpr::CHUNK_TYPE_WORD_ADDRESS ? oneCharacter(chunk.get_word()) : std::string{};
		},
		"address", [](const gpr::chunk& chunk) {
			return chunk.tp() == gpr::CHUNK_TYPE_WORD_ADDRESS
				? chunk.get_address()
				: gpr::make_int_address(0);
		},
		"comment", [](const gpr::chunk& chunk) {
			return chunk.tp() == gpr::CHUNK_TYPE_COMMENT ? chunk.get_comment_text() : std::string{};
		},
		"isolated_word", [](const gpr::chunk& chunk) {
			return chunk.tp() == gpr::CHUNK_TYPE_WORD ? oneCharacter(chunk.get_single_word()) : std::string{};
		});

	state.new_usertype<gpr::block>("GprBlock", sol::no_constructor,
		"size", &gpr::block::size,
		"is_deleted", &gpr::block::is_deleted,
		"has_line_number", &gpr::block::has_line_number,
		"line_number", [](const gpr::block& block) {
			return block.has_line_number() ? block.line_number() : -1;
		},
		"source", &gpr::block::to_string,
		"get_chunk", [](gpr::block& block, int index) {
			return block.get_chunk(index);
		});

	state.new_usertype<gpr::gcode_program>("GprProgram", sol::no_constructor,
		"num_blocks", &gpr::gcode_program::num_blocks,
		"get_block", [](gpr::gcode_program& program, int index) {
			return program.get_block(static_cast<std::size_t>(index));
		});

	sol::table gprModule = state["gpr"].get_or_create<sol::table>();
	gprModule.set_function("parse", &gpr::parse_gcode);
	gprModule.set_function("parse_with_source", &gpr::parse_gcode_saving_block_text);
	gprModule.set_function("lex_block", &gpr::lex_block);
}

void registerAllBindings(sol::state& state) {
	registerToolpathBindings(state);
	registerGprBindings(state);
	registerMachineBindings(state);
}

} // namespace nc::lua
