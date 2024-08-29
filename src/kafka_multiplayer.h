#pragma once

#include <godot_cpp/classes/node.hpp>
#include <rdkafka.h>

class KafkaMultiplayer : godot::Node {
	GDCLASS(KafkaMultiplayer, godot::Node)

	private:

	protected:
		static void _bind_methods();

	public:
		KafkaMultiplayer();
		~KafkaMultiplayer();
};
