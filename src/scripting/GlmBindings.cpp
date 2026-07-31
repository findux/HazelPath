#include "LuaBindings.h"

#include <glm/glm.hpp>

namespace nc::lua {

void registerGlmBindings(sol::state& state) {
	using Vec3 = glm::dvec3;

	state.new_usertype<Vec3>("GlmVec3",
		sol::constructors<Vec3(), Vec3(double), Vec3(double, double, double)>(),
		"x", &Vec3::x,
		"y", &Vec3::y,
		"z", &Vec3::z,
		sol::meta_function::addition,
		[](const Vec3& left, const Vec3& right) { return left + right; },
		sol::meta_function::subtraction,
		[](const Vec3& left, const Vec3& right) { return left - right; },
		sol::meta_function::unary_minus,
		[](const Vec3& value) { return -value; },
		sol::meta_function::multiplication,
		sol::overload(
			[](const Vec3& value, double scalar) { return value * scalar; },
			[](double scalar, const Vec3& value) { return scalar * value; }),
		sol::meta_function::division,
		[](const Vec3& value, double scalar) { return value / scalar; });

	sol::table glmModule = state["glm"].get_or_create<sol::table>();
	glmModule.set_function("vec3", [](double x, double y, double z) {
		return Vec3(x, y, z);
	});
	glmModule.set_function("length", [](const Vec3& value) {
		return glm::length(value);
	});
	glmModule.set_function("normalize", [](const Vec3& value) {
		const double length = glm::length(value);
		return length > 0.0 ? value / length : Vec3(0.0);
	});
	glmModule.set_function("dot", [](const Vec3& left, const Vec3& right) {
		return glm::dot(left, right);
	});
	glmModule.set_function("cross", [](const Vec3& left, const Vec3& right) {
		return glm::cross(left, right);
	});
}

} // namespace nc::lua