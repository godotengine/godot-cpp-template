#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "godot_object_compiler/macros.h"

using namespace godot;
#include "example_class.generated.h"

GODOT_CLASS();
class ExampleClass : public RefCounted {
	GODOT_GENERATED_BODY();

public:
	ExampleClass() = default;
	~ExampleClass() override = default;

	GODOT_FUNCTION();
	void print_type(const Variant &p_variant) const;
};

GODOT_GENERATED_GLOBAL();
