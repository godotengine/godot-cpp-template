#pragma once
#ifndef ECHONODE3D_H
#define ECHONODE3D_H

#include <godot_cpp/classes/node3d.hpp>

namespace godot
{
	class Echo3D : public Node3D
	{
		GDCLASS(Echo3D, Node3D)

	public:
		void _process(double delta);
		void echo(String msg);
		
		Echo3D();
		~Echo3D();

	protected:
		static void _bind_methods();
	};
}

#endif
