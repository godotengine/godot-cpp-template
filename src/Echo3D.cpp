#include "Echo3D.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Echo3D::_bind_methods()
{
	// No need to bind godot pre-defined methods

	// Bind Cpp method
	ClassDB::bind_method(D_METHOD("echo", "msg"), &Echo3D::echo);

	// Add a signal to invoke GDScript
	ADD_SIGNAL(MethodInfo("echo_signal", PropertyInfo(Variant::STRING, "msg")));
}

void Echo3D::echo(String msg)
{
	UtilityFunctions::print("Echo3D::echo: ", msg);
}

void Echo3D::_process(double delta)
{
	String msg = "EchoNode3D::_process: " + String::num(delta);
	echo(msg);
}

Echo3D::Echo3D()
{
}

Echo3D::~Echo3D()
{
}
