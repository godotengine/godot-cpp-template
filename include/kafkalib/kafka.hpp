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
	Packet(int8_t *data, size_t size) :
			data(data), size(size) {}

	~Packet();

	Packet(const Packet &other);

	int8_t *get_data() const;
	uint32_t get_size() const;
private:
	int8_t *data;
	size_t size;
};

class PacketNode {
public:
	PacketNode() :
			packet(nullptr), next(nullptr) {}
	PacketNode(std::shared_ptr<Packet> packet) :
			packet(packet), next(nullptr) {}

	virtual ~PacketNode();

	PacketNode(const PacketNode &other);

	std::shared_ptr<Packet> get_packet() const;
	PacketNode *get_next() const;
	void set_next(PacketNode *node);

protected:
	std::shared_ptr<Packet> packet;
private:
	std::atomic<PacketNode *> next;
};

enum class LogLevel {
	EMERGENCY = 0,
	ALERT = 1,
	CRITICAL = 2,
	ERROR = 3,
	WARNING = 4,
	NOTICE = 5,
	INFO = 6,
	DEBUG = 7
};

static const std::string LogLevelToString(const LogLevel level) {
	switch (level) {
		case LogLevel::EMERGENCY:
			return "EMERGENCY";
		case LogLevel::ALERT:
			return "ALERT";
		case LogLevel::CRITICAL:
			return "CRITICAL";
		case LogLevel::ERROR:
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

enum ConsumerOffsetReset {
	EARLIEST,
	LATEST
};

static const std::string ConsumerOffsetResetToString(const ConsumerOffsetReset reset) {
	switch (reset) {
		case ConsumerOffsetReset::EARLIEST:
			return "earliest";
		case ConsumerOffsetReset::LATEST:
			return "latest";
		default:
			return "latest";
	}
}
static const ConsumerOffsetReset ConsumerOffsetResetFromString(const std::string &reset) {
	if (reset == "earliest") {
		return ConsumerOffsetReset::EARLIEST;
	} else if (reset == "latest") {
		return ConsumerOffsetReset::LATEST;
	} else {
		return ConsumerOffsetReset::LATEST;
	}
}
struct KafkaConsumerMetadata {
public:
	std::string brokers;
	std::vector<std::string> topics;

	std::optional<std::string> group_id;
	ConsumerOffsetReset offset_reset = ConsumerOffsetReset::EARLIEST;
};

struct KafkaProducerMetadata {
	public:
	std::string brokers;
	std::string topic;
};

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
	void add_producer(const KafkaProducerMetadata &metadata, const uint32_t channel = 0);
	/// <summary>
	/// Creates a consumer and tracks it internally.
	/// </summary>
	/// <param name="brokers"></param>
	/// <param name="topic"></param>
	/// <param name="group_id"></param>
	/// <param name="channel"></param>
	void add_consumer(const KafkaConsumerMetadata &metadata, const uint32_t channel = 0);

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
	/// Gets all the channel ids of registered consumers.
	/// </summary>
	/// <returns></returns>
	const std::vector<uint32_t> get_registered_consumer_channels() const;

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
			const uint32_t channel,
			std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> consumers,
			std::shared_ptr<InternalLogger> logger,
			std::shared_ptr<InternalRebalanceCb> rebalance_cb
		) : consumers(consumers), logger(logger), rebalance_cb(rebalance_cb), running(true)
		{
			thread = std::make_shared<std::thread>(&ConsumerContext::_consumer_thread, this, channel);
		}

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
		void add_consumer(std::shared_ptr<RdKafka::KafkaConsumer> consumer);
	private:
		void _consumer_thread(const uint32_t channel);

		std::atomic<PacketNode *> next_packet_head;
		std::atomic<PacketNode *> next_packet_tail;
		std::atomic<uint32_t> packet_count;

		std::atomic<bool> running;
		std::shared_ptr<std::thread> thread;
		std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> consumers;

		std::shared_ptr<InternalLogger> logger;
		std::shared_ptr<InternalRebalanceCb> rebalance_cb;
	};

	std::function<void(const LogLevel logLevel, const std::string &message)> m_log_callback;
	LogLevel m_log_level = LogLevel::WARNING;

	std::shared_ptr<ProducerContext> m_admin_producer;
	std::shared_ptr<ConsumerContext> m_admin_consumer;

	std::map<uint32_t, std::vector<std::shared_ptr<ProducerContext>>> m_producers;
	std::map<uint32_t, std::shared_ptr<ConsumerContext>> m_consumers;
};
}
