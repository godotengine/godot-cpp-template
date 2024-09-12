#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <thread>

#include "kafka.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/multiplayer_peer_extension.hpp>

using ConsumerOffsetReset = Kafka::ConsumerOffsetReset;

class KafkaSubscriberMetadata : public godot::RefCounted {
	GDCLASS(KafkaSubscriberMetadata, godot::RefCounted)

public:
	enum OffsetReset {
		EARLIEST = Kafka::ConsumerOffsetReset::EARLIEST,
		LATEST = Kafka::ConsumerOffsetReset::LATEST
	};

	KafkaSubscriberMetadata() = default;
	~KafkaSubscriberMetadata() = default;

	void set_brokers(const godot::String &brokers) { metadata.brokers = brokers.utf8().get_data(); }
	std::string get_brokers() const { return metadata.brokers.c_str(); }

	void set_topics(const godot::PackedStringArray &topics) {
		metadata.topics.clear();
		for (int i = 0; i < topics.size(); i++) {
			metadata.topics.push_back(topics[i].utf8().get_data());
		}
	}
	std::vector<std::string> get_topics() const { return metadata.topics; }

	void set_group_id(const godot::String &group_id) { metadata.group_id = group_id.utf8().get_data(); }
	std::optional<std::string> get_group_id() const { return metadata.group_id; }

	void set_offset_reset(const OffsetReset offset_reset)
	{
		metadata.offset_reset = (Kafka::ConsumerOffsetReset)offset_reset;
	}
	std::string get_offset_reset() const { return Kafka::ConsumerOffsetResetToString(metadata.offset_reset); }
	const ConsumerOffsetReset get_offset_reset_enum() const { return metadata.offset_reset; }

protected:
	static void _bind_methods()
	{
		godot::ClassDB::bind_method(godot::D_METHOD("set_brokers", "brokers"), &KafkaSubscriberMetadata::set_brokers);
		//godot::ClassDB::bind_method(D_METHOD("get_brokers"), &KafkaSubscriberMetadata::get_brokers);

		godot::ClassDB::bind_method(godot::D_METHOD("set_topics", "topics"), &KafkaSubscriberMetadata::set_topics);
		//godot::ClassDB::bind_method(D_METHOD("get_topics"), &KafkaSubscriberMetadata::get_topics);

		godot::ClassDB::bind_method(godot::D_METHOD("set_group_id", "group_id"), &KafkaSubscriberMetadata::set_group_id);
		//godot::ClassDB::bind_method(D_METHOD("get_group_id"), &KafkaSubscriberMetadata::get_group_id);

		godot::ClassDB::bind_method(godot::D_METHOD("set_offset_reset", "offset_reset"), &KafkaSubscriberMetadata::set_offset_reset);
		//godot::ClassDB::bind_method(D_METHOD("get_offset_reset"), &KafkaSubscriberMetadata::get_offset_reset);

		BIND_ENUM_CONSTANT(EARLIEST);
		BIND_ENUM_CONSTANT(LATEST);
	}

private:
	Kafka::KafkaConsumerMetadata metadata;
};

class KafkaPublisherMetadata : public godot::RefCounted {
	GDCLASS(KafkaPublisherMetadata, godot::RefCounted)
public:
	KafkaPublisherMetadata() = default;
	~KafkaPublisherMetadata() = default;

	void set_brokers(const godot::String &brokers) { metadata.brokers = brokers.utf8().get_data(); }
	std::string get_brokers() const { return metadata.brokers.c_str(); }

	void set_topic(const godot::String &topic) { metadata.topic = topic.utf8().get_data(); }
	std::string get_topic() const { return metadata.topic.c_str(); }

protected:
	static void _bind_methods()
	{
		godot::ClassDB::bind_method(godot::D_METHOD("set_brokers", "brokers"), &KafkaPublisherMetadata::set_brokers);
		//godot::ClassDB::bind_method(D_METHOD("get_brokers"), &KafkaPublisherMetadata::get_brokers);

		godot::ClassDB::bind_method(godot::D_METHOD("set_topic", "topic"), &KafkaPublisherMetadata::set_topic);
		//godot::ClassDB::bind_method(D_METHOD("get_topic"), &KafkaPublisherMetadata::get_topic);
	}
private:
	Kafka::KafkaProducerMetadata metadata;
};

class KafkaMultiplayerPeer : public godot::MultiplayerPeerExtension {
		GDCLASS(KafkaMultiplayerPeer, godot::MultiplayerPeerExtension)
public:

	enum LogSeverity
	{
		EMERGENCY,
		ALERT,
		CRITICAL,
		ERROR,
		WARNING,
		NOTICE,
		INFO,
		DEBUG
	};

	virtual godot::Error _get_packet(const uint8_t **r_buffer, int32_t *r_buffer_size) override;
	virtual godot::Error _put_packet(const uint8_t *p_buffer, int32_t p_buffer_size) override;

	virtual int32_t _get_available_packet_count() const override;
	virtual int32_t _get_max_packet_size() const override { return MAX_PACKET_SIZE_MTU; }

	virtual godot::PackedByteArray _get_packet_script() override;
	virtual godot::Error _put_packet_script(const godot::PackedByteArray &p_buffer) override;

	virtual int32_t _get_packet_channel() const override;
	virtual godot::MultiplayerPeer::TransferMode _get_packet_mode() const override { return godot::MultiplayerPeer::TRANSFER_MODE_RELIABLE; } // Always.

	virtual void _set_transfer_channel(int32_t p_channel) override;
	virtual int32_t _get_transfer_channel() const override;

	// We only support reliable mode.
	virtual void _set_transfer_mode(godot::MultiplayerPeer::TransferMode p_mode) override { /* no-op */ }
	virtual godot::MultiplayerPeer::TransferMode _get_transfer_mode() const override { return godot::MultiplayerPeer::TRANSFER_MODE_RELIABLE; }

	virtual void _set_target_peer(int32_t p_peer_id) override;
	virtual int32_t _get_packet_peer() const override;

	// This is always true, since we act as a server, always.
	virtual bool _is_server() const override { return true; }

	virtual void _poll() override;
	virtual void _close() override;

	virtual void _disconnect_peer(int32_t p_id, bool p_now) override;

	virtual int32_t _get_unique_id() const override { return 0; } // Always SERVER.

	virtual void _set_refuse_new_connections(bool p_refuse) override;

	virtual bool _is_refusing_new_connections() const override;
	virtual bool _is_server_relay_supported() const override { return false; } // This will never be a P2P Connection.

	virtual godot::MultiplayerPeer::ConnectionStatus _get_connection_status() const override;

	void register_internal_publisher(const godot::Ref<KafkaPublisherMetadata> &metadata);
	void register_internal_subscriber(const godot::Ref<KafkaSubscriberMetadata> &metadata);
	void register_publisher(const godot::Ref<KafkaPublisherMetadata> &metadata, const int32_t channel = 0);
	void register_subscriber(const godot::Ref<KafkaSubscriberMetadata> &metadata, const int32_t channel = 0);
	
	KafkaMultiplayerPeer();
	~KafkaMultiplayerPeer();

protected:
	static void _bind_methods();

private:

	// Internal Packet must follow the format:
	// [uint8_t type][uint32_t source][data...]
	class InternalPacket : public Kafka::PacketNode
	{
	public:
		enum class PacketType : uint8_t
		{
			UNKNOWN,
			GAME_PACKET,
			INTERNAL_PEER_DISCONNECT,
			INTERNAL_PEER_CONNECT,
		};

		InternalPacket(std::shared_ptr<Kafka::Packet> packet, uint32_t channel)
			: Kafka::PacketNode(packet), channel(channel), source(0), type(PacketType::UNKNOWN) { }

		InternalPacket(const PacketType type, const int64_t target_peer, const uint32_t channel, void* data = nullptr, int32_t data_length = 0)
		{
			// Assumble
			// HEADER: [uint8_t type][uint32_t source]
			int8_t *packet_data = new int8_t[sizeof(uint8_t) + sizeof(uint64_t) + data_length];
			*(packet_data) = (uint8_t)type;
			*(uint64_t *)(packet_data + sizeof(uint8_t)) = target_peer;
			if (data != nullptr)
			{
				memcpy(packet_data + sizeof(uint64_t) + sizeof(uint8_t), data, data_length);
			}

			// Store
			this->packet	= std::make_shared<Kafka::Packet>(packet_data, sizeof(uint64_t) + sizeof(uint8_t) + data_length);
			this->channel	= channel;
			this->source	= target_peer;
			this->type		= type;
		}

		const uint8_t *raw_data()
		{
			return (const uint8_t *)get_packet()->get_data();
		}

		const size_t raw_size()
		{
			return get_packet()->get_size();
		}


		const uint8_t *data()
		{
			// ERROR CASE
			if (get_packet()->get_size() < sizeof(uint64_t) + sizeof(uint8_t))
			{
				return nullptr;
			}

			// HEADER: [uint8_t type][uint32_t source]
			type = (PacketType)(*(get_packet()->get_data()));
			source = *(uint64_t *)(get_packet()->get_data() + sizeof(uint8_t));
			
			// Skip the source.
			return (const uint8_t *)(get_packet()->get_data() + sizeof(uint64_t) + sizeof(uint8_t));
		}

		const uint32_t size() const
		{
			return get_packet()->get_size() - sizeof(uint64_t) - sizeof(uint8_t);
		}

		PacketType get_type() const { return type; }
		uint64_t get_source() const { return source; }
		uint32_t get_channel() const { return channel; }
	private:
		PacketType type;
		uint64_t source;
		uint32_t channel;
	};

	void _internal_message_handler(const InternalPacket &packet);

	const uint32_t MAX_PACKET_SIZE_MTU = 1400;

	std::atomic<InternalPacket *> incoming_game_packet_head;
	std::atomic<InternalPacket *> incoming_game_packet_tail;

	int32_t transfer_channel	= 0;
	int32_t target_peer			= 0;

	Kafka::KafkaController kafka_controller;

};

VARIANT_ENUM_CAST(KafkaSubscriberMetadata::OffsetReset);
VARIANT_ENUM_CAST(KafkaMultiplayerPeer::LogSeverity);
