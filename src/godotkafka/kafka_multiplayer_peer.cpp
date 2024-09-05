#include "kafka_multiplayer_peer.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <functional>
#include <rdkafkacpp.h>

using namespace godot;


void KafkaMultiplayerPeer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_publisher", "topic", "brokers", "channel"), &KafkaMultiplayerPeer::register_publisher);
	ClassDB::bind_method(D_METHOD("register_subscriber", "topics", "brokers", "group_id", "channel"), &KafkaMultiplayerPeer::register_subscriber);
	ClassDB::bind_method(D_METHOD("register_admin_channel", "brokers", "topics_prefix"), &KafkaMultiplayerPeer::register_admin_channel);

	ADD_SIGNAL(MethodInfo("publisher_error",
		PropertyInfo(Variant::STRING, "message")
	));
	ADD_SIGNAL(MethodInfo("subscriber_error",
		PropertyInfo(Variant::STRING, "message")
	));
}

KafkaMultiplayerPeer::KafkaMultiplayerPeer()
{

}

KafkaMultiplayerPeer::~KafkaMultiplayerPeer()
{
	// Resource Acquisition Is Initialization (RAII) of all the information of the peer.
	_close();
}

godot::Error KafkaMultiplayerPeer::_get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size)
{
	return godot::Error::OK;
}

godot::Error KafkaMultiplayerPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size)
{
	return godot::Error::OK;
}

int32_t KafkaMultiplayerPeer::_get_available_packet_count() const
{
	return kafka_controller.get_packet_count();
}

godot::PackedByteArray KafkaMultiplayerPeer::_get_packet_script()
{
	return godot::PackedByteArray();
}

godot::Error KafkaMultiplayerPeer::_put_packet_script(const godot::PackedByteArray &p_buffer)
{
	return godot::Error::OK;
}

int32_t KafkaMultiplayerPeer::_get_packet_channel() const
{
	return 0;
}

void KafkaMultiplayerPeer::_set_transfer_channel(int32_t p_channel)
{
}

int32_t KafkaMultiplayerPeer::_get_transfer_channel() const
{
	return 0;
}

void KafkaMultiplayerPeer::_set_target_peer(int32_t p_peer_id)
{
}

int32_t KafkaMultiplayerPeer::_get_packet_peer() const
{
	return 0;
}

void KafkaMultiplayerPeer::_poll()
{
}

void KafkaMultiplayerPeer::_close()
{
	// Cleanup Publish/Subscriber Brokers
	//	The memory is managed by the shared_ptr, so we don't need to delete the pointers.
	kafka_controller.clear();
}

void KafkaMultiplayerPeer::_disconnect_peer(int32_t p_id, bool p_now)
{
	// Todo :: Send a packet to the edge service to disconnect the peer.
	// Todo :: Create a Admin Publisher/Subscriber connection to the kafka topics.
}

void KafkaMultiplayerPeer::_set_refuse_new_connections(bool p_refuse)
{
	// Todo :: We can refuse new connections, as long as the edge service knows to stop sending new connection messages to this server.

	UtilityFunctions::printerr("KafkaMultiplayerPeer::_set_refuse_new_connections() cannot be set.");
}

bool KafkaMultiplayerPeer::_is_refusing_new_connections() const
{
	return false;
}

godot::MultiplayerPeer::ConnectionStatus KafkaMultiplayerPeer::_get_connection_status() const
{
	return MultiplayerPeer::CONNECTION_CONNECTED;
}

void KafkaMultiplayerPeer::register_admin_channel(const godot::String &brokers, const godot::String &topics_prefix)
{
	std::string brokers_str = brokers.utf8().get_data();
	std::string topics_prefix_str = topics_prefix.utf8().get_data();

	std::string admin_out_topic = topics_prefix_str + "_admin_out";
	std::string admin_in_topic = topics_prefix_str + "_admin_in";

	// Create a Producer Context for the Admin Channel.
	
	// Create a Consumer Context for the Admin Channel.

}

void KafkaMultiplayerPeer::register_publisher(const godot::String &topic, const godot::String &brokers, const int32_t channel) {
	std::string topic_str = topic.utf8().get_data();
	std::string brokers_str = brokers.utf8().get_data();

	// Create a Producer Context for the Channel.
	std::string errstr;
}

void KafkaMultiplayerPeer::register_subscriber(const godot::PackedStringArray &topics, const godot::String &brokers, const godot::String &group_id, const int32_t channel) {
	std::string brokers_str = brokers.utf8().get_data();
	std::vector<std::string> topics_str;
	for (int i = 0; i < topics.size(); i++)
	{
		topics_str.push_back(topics[i].utf8().get_data());
	}
	std::string group_id_str = group_id.utf8().get_data();

	// Create a Consumer Context for the Channel.
	std::string errstr;
}
