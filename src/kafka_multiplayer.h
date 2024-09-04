#pragma once
/*
#include <string>
#include <unordered_map>

#include "kafka_multiplayer_peer.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/multiplayer_api_extension.hpp>
#include <rdkafkacpp.h>

class KafkaMultiplayer : godot::MultiplayerAPIExtension {
	GDCLASS(KafkaMultiplayer, godot::MultiplayerAPIExtension)

public:
	KafkaMultiplayer();
	~KafkaMultiplayer();

	void add_producer(const std::string& name, const std::string& broker, const std::string& topic);
	void add_consumer(const std::string& name, const std::string& broker, const std::string& topic);

	void remove_producer(const std::string& name);
	void remove_consumer(const std::string& name);

	void produce(const std::string& name, const std::string& message);

	virtual godot::Error _poll() override;


private:
	std::unordered_map<std::string, RdKafka::Producer *> producers;
	std::unordered_map<std::string, RdKafka::Consumer *> consumers;

	void _on_producer_poll();
	void _on_consumer_poll();

protected:
	static void _bind_methods();
};*/
