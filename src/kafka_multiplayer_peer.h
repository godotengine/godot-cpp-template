#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <thread>

#include <godot_cpp/classes/multiplayer_peer_extension.hpp>
#include <rdkafkacpp.h>

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
	virtual godot::MultiplayerPeer::TransferMode _get_packet_mode() const override;

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

	class Packet {
		int8_t *data;
		uint32_t size;

	public:
		Packet(int8_t *data, uint32_t size) : data(data), size(size) {}

		~Packet()
		{
			delete[] data;
		}

		Packet(const Packet &other)
		{
			size = other.size;
			data = new int8_t[size];
			memcpy(data, other.data, size);
		}

		int8_t *get_data() const { return data; }
		uint32_t get_size() const { return size; }
	};

	class PacketNode
	{
		std::shared_ptr<Packet> packet;
		std::atomic<PacketNode *> next;

	public:
		PacketNode(std::shared_ptr<Packet> packet) : packet(packet), next(nullptr) {}

		~PacketNode() {}

		PacketNode(const PacketNode &other)
		{
			packet = other.packet;
			next = other.next.load();
		}

		std::shared_ptr<Packet> get_packet() const { return packet; }
		PacketNode *get_next() const { return next.load(); }
		void set_next(PacketNode *node) { next.store(node); }
	};

	class ProducerContext {
		std::shared_ptr<RdKafka::Producer> producer;
		std::shared_ptr<RdKafka::Topic> topic;

	public:
		ProducerContext(std::shared_ptr<RdKafka::Producer> producer, std::shared_ptr<RdKafka::Topic> topic)
			: producer(producer), topic(topic) {}

		~ProducerContext() {}

		ProducerContext(const ProducerContext &other)
		{
			producer = other.producer;
			topic = other.topic;
		}

		std::shared_ptr<RdKafka::Producer> get_producer() const { return producer; }
		std::shared_ptr<RdKafka::Topic> get_topic() const { return topic; }
	};

	class ConsumerContext {
		std::atomic<PacketNode *> next_packet_head;
		std::atomic<PacketNode *> next_packet_tail;
		std::atomic<uint32_t> packet_count;

		std::atomic<bool> running;
		std::shared_ptr<std::thread> thread;
		std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> consumers;

	public:
		ConsumerContext(std::shared_ptr<std::thread> thread, std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> consumers) :
				thread(thread), consumers(consumers), running(true) {}

		~ConsumerContext()
		{
			// Delete all the packets in the queue.
			while (PacketNode *node = next_packet_head.load()) {
				next_packet_head.store(node->get_next());
				delete node;
			}

			for (auto &consumer : consumers) {
				consumer->close();
			}

			// Update the running flag to false, so the thread can finish.
			running.store(false);

			// Wait for the thread to finish.
			thread->join();
		}

		ConsumerContext(const ConsumerContext &other)
		{
			running.store(other.running.load());
			thread = other.thread;
			consumers = other.consumers;
		}

		PacketNode *get_next_packet() const { return next_packet_head.load(); }
		void pop_next_packet()
		{
			PacketNode *node = next_packet_head.load();
			next_packet_head.store(node->get_next());
			delete node;
			packet_count.fetch_sub(1);
		}
		void push_next_packet(PacketNode *node)
		{
			PacketNode *tail = next_packet_tail.load();
			if (tail) {
				tail->set_next(node);
			} else {
				next_packet_head.store(node);
			}
			next_packet_tail.store(node);
			packet_count.fetch_add(1);
		}

		bool is_running() const { return running.load(); }
		void set_running(bool value) { running.store(value); }

		std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> get_consumers() const { return consumers; }
	};

	std::shared_ptr<ProducerContext> m_admin_producer;
	std::shared_ptr<ConsumerContext> m_admin_consumer;

	std::map<uint32_t, std::vector<std::shared_ptr<ProducerContext>>> m_producers;
	std::map<uint32_t, std::shared_ptr<ConsumerContext>> m_consumers;

	void _consumer_thread(std::shared_ptr<ConsumerContext> &consumer, const uint32_t channel);
};
