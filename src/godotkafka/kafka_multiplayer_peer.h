#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <thread>

#include "kafka.hpp"

#include <godot_cpp/classes/multiplayer_peer_extension.hpp>

class KafkaMultiplayerPeer : public godot::MultiplayerPeerExtension {
		GDCLASS(KafkaMultiplayerPeer, godot::MultiplayerPeerExtension)
public:

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

	void register_admin_channel(const godot::String &brokers, const godot::String &topics_prefix);
	void register_publisher(const godot::String &topic, const godot::String &brokers, const int32_t channel = 1);
	void register_subscriber(const godot::PackedStringArray &topics, const godot::String &brokers, const godot::String &group_id, const int32_t channel = 1);
	
	KafkaMultiplayerPeer();
	~KafkaMultiplayerPeer();

protected:
	static void _bind_methods();


private:

	const uint32_t MAX_PACKET_SIZE_MTU = 1400;

	Kafka::KafkaController kafka_controller;

};
