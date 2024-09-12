#include "kafka_multiplayer_peer.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <functional>
#include <rdkafkacpp.h>

using namespace godot;


void KafkaMultiplayerPeer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("register_publisher", "metadata", "channel"), &KafkaMultiplayerPeer::register_publisher);
	ClassDB::bind_method(D_METHOD("register_subscriber", "metadata", "channel"), &KafkaMultiplayerPeer::register_subscriber);
	ClassDB::bind_method(D_METHOD("register_internal_publisher", "metadata"), &KafkaMultiplayerPeer::register_internal_publisher);
	ClassDB::bind_method(D_METHOD("register_internal_subscriber", "metadata"), &KafkaMultiplayerPeer::register_internal_subscriber);

	ADD_SIGNAL(MethodInfo("kafka_log",
		PropertyInfo(Variant::INT, "log_severity"),
		PropertyInfo(Variant::STRING, "message")
	));

	ADD_SIGNAL(MethodInfo("publisher_error",
		PropertyInfo(Variant::STRING, "message")
	));
	ADD_SIGNAL(MethodInfo("subscriber_error",
		PropertyInfo(Variant::STRING, "message")
	));

	BIND_ENUM_CONSTANT(EMERGENCY);
	BIND_ENUM_CONSTANT(ALERT);
	BIND_ENUM_CONSTANT(CRITICAL);
	BIND_ENUM_CONSTANT(ERROR);
	BIND_ENUM_CONSTANT(WARNING);
	BIND_ENUM_CONSTANT(NOTICE);
	BIND_ENUM_CONSTANT(INFO);
	BIND_ENUM_CONSTANT(DEBUG);
}

KafkaMultiplayerPeer::KafkaMultiplayerPeer()
{
	#if DEBUG_ENABLED || DEBUG
	kafka_controller.set_log_level(Kafka::LogLevel::DEBUG);
	#else
	kafka_controller.set_log_level(Kafka::LogLevel::INFO);
	#endif

	kafka_controller.set_log_callback([this](Kafka::LogLevel severity, const std::string &message)
	{
		emit_signal("kafka_log", (int)severity, message.c_str());
	});
}

KafkaMultiplayerPeer::~KafkaMultiplayerPeer()
{
	// Resource Acquisition Is Initialization (RAII) of all the information of the peer.
	_close();

	// Cleanup the packet queue.
	InternalPacket *current_packet = incoming_game_packet_head.load();
	while (current_packet != nullptr)
	{
		InternalPacket *next_packet = (InternalPacket *)current_packet->get_next();
		delete current_packet;
		current_packet = next_packet;
	}
	incoming_game_packet_head.store(nullptr);
	incoming_game_packet_tail.store(nullptr);

	// Cleanup the Kafka Controller.
	kafka_controller.clear();
}

godot::Error KafkaMultiplayerPeer::_get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size)
{
	// If there are no packets in the queue, return an empty packet.
	if (incoming_game_packet_head.load() == nullptr)
	{
		return godot::Error::ERR_UNAVAILABLE;
	}

	// Get the packet from the head of the queue.
	InternalPacket *message = incoming_game_packet_head.load();

	// Copy the message to the packet.
	*r_buffer		= (const uint8_t *)message->data();
	*r_buffer_size	= (int32_t)message->size();

	// Remove the packet from the queue.
	InternalPacket *next_packet = (InternalPacket *)incoming_game_packet_head.load()->get_next();
	delete incoming_game_packet_head.load();
	incoming_game_packet_head.store(next_packet);

	return godot::Error::OK;
}

godot::Error KafkaMultiplayerPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size)
{
	int32_t channel = _get_transfer_channel();

	InternalPacket packet(
		InternalPacket::PacketType::GAME_PACKET,
		target_peer,
		channel,
		(void *)p_buffer,
		p_buffer_size
	);

	// Get the producer for the channel.
	//	We send the raw packet, because we want all the data from it, including the internal bits.
	if (kafka_controller.send(channel, packet.raw_data(), packet.raw_size()))
	{
		return godot::Error::OK;
	}

	return godot::Error::FAILED;
}

int32_t KafkaMultiplayerPeer::_get_available_packet_count() const
{
	return kafka_controller.get_packet_count();
}

godot::PackedByteArray KafkaMultiplayerPeer::_get_packet_script()
{
	godot::PackedByteArray packet;

	const uint8_t *buffer;
	int32_t buffer_size;
	if (_get_packet(&buffer, &buffer_size) == godot::Error::OK)
	{
		packet.resize(buffer_size);
		memcpy(packet.ptrw(), buffer, buffer_size);
	}

	return packet;
}

godot::Error KafkaMultiplayerPeer::_put_packet_script(const godot::PackedByteArray &p_buffer)
{
	// Convert godot::PackedByteArray to a uint8_t array.
	const uint8_t *buffer = p_buffer.ptr();
	int32_t buffer_size = p_buffer.size();

	return _put_packet(buffer, buffer_size);
}

int32_t KafkaMultiplayerPeer::_get_packet_channel() const
{
	if (incoming_game_packet_head.load() != nullptr)
	{
		return incoming_game_packet_head.load()->get_channel();
	}

	return -1;
}

void KafkaMultiplayerPeer::_set_transfer_channel(int32_t p_channel)
{
	transfer_channel = p_channel;
}

int32_t KafkaMultiplayerPeer::_get_transfer_channel() const
{
	return transfer_channel;
}

void KafkaMultiplayerPeer::_set_target_peer(int32_t p_peer_id)
{
	target_peer = p_peer_id;
}

int32_t KafkaMultiplayerPeer::_get_packet_peer() const
{
	if (incoming_game_packet_head.load() != nullptr)
	{
		return incoming_game_packet_head.load()->get_source();
	}

	return -1;
}

void KafkaMultiplayerPeer::_poll()
{
	// Poll the Kafka Controller for any new messages
	for (auto channel : kafka_controller.get_registered_consumer_channels())
	{
		std::shared_ptr<Kafka::Packet> message = kafka_controller.receive(channel);

		if (message == nullptr)
		{
			continue;
		}

		// Moves the packet into the internal queue.
		if (channel == INT32_MAX)
		{
			InternalPacket internal_packet = InternalPacket(message, channel);
			_internal_message_handler(internal_packet);
			continue;
		}

		// Append the message to the packet queue.
		InternalPacket *next_packet = new InternalPacket(message, channel);

		// Start the queue if it is empty.
		if (incoming_game_packet_tail.load() == nullptr)
		{
			incoming_game_packet_head.store(next_packet);
			incoming_game_packet_tail.store(next_packet);
		}
		else // Append the message to the tail of the queue.
		{
			// Loads the current tail of the queue, and points the next one to the new packet.
			incoming_game_packet_tail.load()->set_next(next_packet);

			// Updates the tail of the queue to the new packet.
			incoming_game_packet_tail.store(next_packet);
		}
	}
}

void KafkaMultiplayerPeer::_internal_message_handler(const InternalPacket &packet)
{
	switch (packet.get_type())
	{
		case InternalPacket::PacketType::INTERNAL_PEER_CONNECT:
		{
			emit_signal("peer_connected", packet.get_source());
			break;
		}
		case InternalPacket::PacketType::INTERNAL_PEER_DISCONNECT:
		{
			emit_signal("peer_disconnected", packet.get_source());
			break;
		}
		default:
		{
			UtilityFunctions::printerr("KafkaMultiplayerPeer::_internal_message_handler() Unknown Packet Type.");
			break;
		}
	}
	
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

	UtilityFunctions::printerr("KafkaMultiplayerPeer::_disconnect_peer() has not been implemented.");
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

void KafkaMultiplayerPeer::register_internal_publisher(const godot::Ref<KafkaPublisherMetadata> &metadata)
{
	register_publisher(metadata, INT32_MAX);

	// Test message
	InternalPacket packet(
		InternalPacket::PacketType::INTERNAL_PEER_CONNECT,
		0,
		INT32_MAX,
		nullptr,
		0
	);

	// Get the producer for the channel.
	//	We send the raw packet, because we want all the data from it, including the internal bits.
	if (!kafka_controller.send(INT32_MAX, packet.raw_data(), packet.raw_size()))
	{
		UtilityFunctions::printerr("KafkaMultiplayerPeer::register_internal_publisher() Failed to send the internal message.");
		return;
	}

	UtilityFunctions::print("KafkaMultiplayerPeer::register_internal_publisher() Sent the internal message.");
}

void KafkaMultiplayerPeer::register_internal_subscriber(const godot::Ref<KafkaSubscriberMetadata> &metadata)
{
	register_subscriber(metadata, INT32_MAX);
}

//void KafkaMultiplayerPeer::register_internal_publisher(const godot::String &brokers, const godot::String &topics_prefix)
//{
//	// Create the Admin Channel Topics.
//	std::string topics_prefix_str = topics_prefix.utf8().get_data();
//	std::string admin_out_topic;
//	if (topics_prefix_str.empty())
//	{
//		admin_out_topic = "admin_out";
//	}
//	else
//	{
//		admin_out_topic = topics_prefix_str + "_admin_out";
//	}
//	std::string admin_in_topic;
//	if (topics_prefix_str.empty())
//	{
//		admin_in_topic = "admin_in";
//	}
//	else
//	{
//		admin_in_topic = topics_prefix_str + "_admin_in";
//	}
//
//	// Create a Producer Context for the Admin Channel.
//	{
//		KafkaPublisherMetadata *metadata = memnew(KafkaPublisherMetadata);
//		metadata->set_brokers(brokers);
//		metadata->set_topic(godot::String(admin_out_topic.c_str()));
//
//		register_publisher(Ref<KafkaPublisherMetadata>(metadata), INT32_MAX);
//	}
//	
//	// Create a Consumer Context for the Admin Channel.
//	{
//		KafkaSubscriberMetadata *metadata = memnew(KafkaSubscriberMetadata);
//		metadata->set_brokers(brokers);
//		metadata->set_group_id("admin_group");
//		godot::PackedStringArray topics;
//		topics.push_back(godot::String(admin_in_topic.c_str()));
//		metadata->set_topics(topics);
//		metadata->set_offset_reset(KafkaSubscriberMetadata::OffsetReset::LATEST);
//
//		register_subscriber(Ref<KafkaSubscriberMetadata>(metadata), INT32_MAX);
//	}
//}

void KafkaMultiplayerPeer::register_publisher(const godot::Ref<KafkaPublisherMetadata> &metadata_ref, const int32_t channel)
{
	KafkaPublisherMetadata metadata = **metadata_ref;

	// Create a Producer Context for the Channel.
	Kafka::KafkaProducerMetadata kafka_metadata;
	kafka_metadata.brokers = metadata.get_brokers();
	kafka_metadata.topic = metadata.get_topic();

	kafka_controller.add_producer(kafka_metadata, channel);

	UtilityFunctions::print("KafkaMultiplayerPeer::register_publisher() Added Producer to Channel: " + itos(channel));
}

void KafkaMultiplayerPeer::register_subscriber(const godot::Ref<KafkaSubscriberMetadata> &metadata_ref, const int32_t channel)
{
	KafkaSubscriberMetadata metadata = **metadata_ref;

	// Create a Consumer Context for the Channel.
	Kafka::KafkaConsumerMetadata kafka_metadata;
	kafka_metadata.brokers = metadata.get_brokers();
	kafka_metadata.group_id = metadata.get_group_id();
	kafka_metadata.topics = metadata.get_topics();
	kafka_metadata.offset_reset = metadata.get_offset_reset_enum();

	kafka_controller.add_consumer(kafka_metadata, channel);

	UtilityFunctions::print("KafkaMultiplayerPeer::register_subscriber() Added Consumer to Channel: " + itos(channel));
}
