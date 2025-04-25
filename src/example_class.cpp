#include "example_class.h"

void ExampleClass::_bind_methods() {
	godot::ClassDB::bind_method(D_METHOD("print_type", "variant"), &ExampleClass::print_type);
}

void ExampleClass::print_type(const Variant &variant) const {
	print_line(vformat("Type: %d", variant.get_type()));
}
