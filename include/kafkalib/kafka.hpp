#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <optional>

// Standin for rdkafka...
namespace RdKafka
{
class KafkaConsumer;
class Producer;
class Message;
class Topic;
}

namespace Kafka
{
class Packet {
public:
	Packet(int8_t *data, uint32_t size) :
			data(data), size(size) {}

	~Packet();

	Packet(const Packet &other);

	int8_t *get_data() const;
	uint32_t get_size() const;
private:
	int8_t *data;
	uint32_t size;
};

class PacketNode {
public:
	PacketNode(std::shared_ptr<Packet> packet) :
			packet(packet), next(nullptr) {}

	~PacketNode() {}

	PacketNode(const PacketNode &other);

	std::shared_ptr<Packet> get_packet() const;
	PacketNode *get_next() const;
	void set_next(PacketNode *node);
private:
	std::shared_ptr<Packet> packet;
	std::atomic<PacketNode *> next;
};

enum class LogLevel {
	EMERG = 0,
	ALERT = 1,
	CRIT = 2,
	ERR = 3,
	WARNING = 4,
	NOTICE = 5,
	INFO = 6,
	DEBUG = 7
};

static const std::string LogLevelToString(const LogLevel level) {
	switch (level) {
		case LogLevel::EMERG:
			return "EMERGENCY";
		case LogLevel::ALERT:
			return "ALERT";
		case LogLevel::CRIT:
			return "CRITICAL";
		case LogLevel::ERR:
			return "ERROR";
		case LogLevel::WARNING:
			return "WARNING";
		case LogLevel::NOTICE:
			return "NOTICE";
		case LogLevel::INFO:
			return "INFO";
		case LogLevel::DEBUG:
			return "DEBUG";
		default:
			return "UNKNOWN";
	}
}

class KafkaController {
public:
	KafkaController();
	~KafkaController();

	/// <summary>
	/// Creates a producer and tracks it internally.
	/// </summary>
	/// <param name="brokers"></param>
	/// <param name="topic"></param>
	/// <param name="channel"></param>
	void add_producer(const std::string &brokers, const std::string &topic, const uint32_t channel);
	/// <summary>
	/// Creates a consumer and tracks it internally.
	/// </summary>
	/// <param name="brokers"></param>
	/// <param name="topic"></param>
	/// <param name="group_id"></param>
	/// <param name="channel"></param>
	void add_consumer(const std::string &brokers, const std::string &topic, const std::string &group_id, const uint32_t channel);

	/// <summary>
	/// Clears all producers of a channel.
	/// </summary>
	/// <param name="channel"></param>
	void clear_producers(const uint32_t channel);
	/// <summary>
	/// Clears all consumers of a channel.
	/// </summary>
	/// <param name="channel"></param>
	void clear_consumers(const uint32_t channel);

	/// <summary>
	/// Clears all producers and consumers.
	/// </summary>
	void clear();

	/// <summary>
	/// Sends a packet to a channel.
	/// </summary>
	/// <param name="channel"></param>
	/// <param name="data"></param>
	/// <param name="size"></param>
	bool send(const uint32_t channel, const void *data, const uint32_t size, const std::optional<std::string> &key = {});
	/// <summary>
	/// Listens for a packet on a channel. This is non-blocking, and will return nullptr if no packet is available.
	/// </summary>
	/// <param name="channel"></param>
	/// <returns></returns>
	std::shared_ptr<Packet> receive(const uint32_t channel);
	/// <summary>
	/// Listens for a packet on any channel. This is non-blocking, and will return nullptr if no packet is available.
	/// </summary>
	/// <returns></returns>
	std::shared_ptr<Packet> receive();

	/// <summary>
	/// Gets a count of packets on a channel.
	/// </summary>
	/// <param name="channel"></param>
	/// <returns></returns>
	const uint32_t get_packet_count(const uint32_t channel) const;

	/// <summary>
	/// Gets a count of packets for any channel.
	/// </summary>
	/// <returns></returns>
	const uint32_t get_packet_count() const;

	/// <summary>
	/// Sets the log level for all producers and consumers.
	/// </summary>
	/// <param name="level"></param>
	void set_log_level(const LogLevel level);

	/// <summary>
	/// Sets the log callback for all producers and consumers.
	/// </summary>
	/// <param name="callback"></param>
	void set_log_callback(const std::function<void(const LogLevel logLevel, const std::string &message)> callback);

	/// <summary>
	/// Set Error Callback.
	/// </summary>
	/// <param name="callback"></param>
	void set_error_callback(const std::function<void(const std::string &message)> callback);

	class InternalLogger;
	class InternalRebalanceCb;
	class InternalDeliveryReportCb;
private:

	class ProducerContext
	{
	public:
		ProducerContext(
			std::shared_ptr<RdKafka::Producer> producer,
			std::shared_ptr<RdKafka::Topic> topic,
			std::shared_ptr<InternalLogger> logger,
			std::shared_ptr<InternalDeliveryReportCb> delivery_report_cb
		) : producer(producer), topic(topic), logger(logger), delivery_report_cb(delivery_report_cb) {}

		~ProducerContext();

		ProducerContext(const ProducerContext &other);

		bool is_healthy() const;

		std::shared_ptr<RdKafka::Producer> get_producer() const;
		std::shared_ptr<RdKafka::Topic> get_topic() const;

		std::shared_ptr<InternalLogger> get_logger_cb() const;
		std::shared_ptr<InternalDeliveryReportCb> get_delivery_report_cb() const;
	private:
		std::shared_ptr<RdKafka::Producer> producer;
		std::shared_ptr<RdKafka::Topic> topic;
		std::shared_ptr<InternalLogger> logger;
		std::shared_ptr<InternalDeliveryReportCb> delivery_report_cb;	
	};

	class ConsumerContext
	{
	public:
		ConsumerContext(
			std::shared_ptr<std::thread> thread,
			std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> consumers,
			std::shared_ptr<InternalLogger> logger,
			std::shared_ptr<InternalRebalanceCb> rebalance_cb
		) : thread(thread), consumers(consumers), logger(logger), rebalance_cb(rebalance_cb) {}

		~ConsumerContext();

		ConsumerContext(const ConsumerContext &other);

		PacketNode *get_next_packet() const;
		void pop_next_packet();
		void push_next_packet(PacketNode *node);
		const uint32_t get_packet_count() const;

		bool is_healthy() const;

		bool is_running() const;
		void set_running(bool value);


		std::shared_ptr<InternalLogger> get_logger_cb() const;
		std::shared_ptr<InternalRebalanceCb> get_rebalance_cb() const;

		std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> get_consumers() const;
	private:
		std::atomic<PacketNode *> next_packet_head;
		std::atomic<PacketNode *> next_packet_tail;
		std::atomic<uint32_t> packet_count;

		std::atomic<bool> running;
		std::shared_ptr<std::thread> thread;
		std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> consumers;

		std::shared_ptr<InternalLogger> logger;
		std::shared_ptr<InternalRebalanceCb> rebalance_cb;
	};

	std::function<void(const std::string &message)> m_error_callback;
	std::function<void(const LogLevel logLevel, const std::string &message)> m_log_callback;
	LogLevel m_log_level = LogLevel::WARNING;

	std::shared_ptr<ProducerContext> m_admin_producer;
	std::shared_ptr<ConsumerContext> m_admin_consumer;

	std::map<uint32_t, std::vector<std::shared_ptr<ProducerContext>>> m_producers;
	std::map<uint32_t, std::shared_ptr<ConsumerContext>> m_consumers;

	void _consumer_thread(std::shared_ptr<ConsumerContext> &consumer, const uint32_t channel);
};
}
