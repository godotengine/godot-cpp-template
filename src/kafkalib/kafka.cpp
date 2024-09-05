#include "kafka.hpp"

#include <rdkafkacpp.h>

#include <inttypes.h>

using namespace Kafka;

const uint32_t MAX_TIMEOUT_MS = 500;

#pragma region Internal

class Kafka::KafkaController::InternalDeliveryReportCb : public RdKafka::DeliveryReportCb {
	std::function<void(RdKafka::Message &)> callback;

	void dr_cb(RdKafka::Message &message) override {
		if (callback) {
			callback(message);
		}
	}
public:
	~InternalDeliveryReportCb()
	{
		printf("DeliveryReportCb destroyed\n");
	}

	void set_callback(std::function<void(RdKafka::Message &)> cb) {
		callback = cb;
	}
};

class Kafka::KafkaController::InternalRebalanceCb : public RdKafka::RebalanceCb {
	std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> callback;

	void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
			std::vector<RdKafka::TopicPartition *> &partitions) override {

		if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
			consumer->assign(partitions);
		} else {
			consumer->unassign();
		}

		if (callback) {
			callback(consumer, err, partitions);
		}
	}
public:

	~InternalRebalanceCb()
	{
		printf("RebalanceCb destroyed\n");
	}

	void set_callback(std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> cb) {
		callback = cb;
	}
};

class Kafka::KafkaController::InternalLogger : public RdKafka::EventCb {
	std::function<void(const LogLevel logLevel, const std::string &message)> callback;

	void event_cb(RdKafka::Event &event) override {
		if (callback)
			callback(static_cast<LogLevel>(event.severity()), event.str());
	}
public:

	~InternalLogger()
	{
		printf("Logger destroyed\n");
	}

	void set_callback(std::function<void(const LogLevel logLevel, const std::string &message)> cb) {
		callback = cb;
	}
};


static RdKafka::Producer *create_producer(const std::string &brokers, std::string &errstr, Kafka::KafkaController::InternalDeliveryReportCb &deliveryReportCb, Kafka::KafkaController::InternalLogger &loggerCb, const LogLevel logLevel) {
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

	// Setup Logging.
	if (conf->set("log_level", std::to_string(static_cast<int>(logLevel)), errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	if (conf->set("event_cb", &loggerCb, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	// Setup the brokers.
	if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	// Set the delivery report callback
	if (conf->set("dr_cb", &deliveryReportCb, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	// Create a Rdkafka Producer
	RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
	if (!producer) {
		return nullptr;
	}

	return producer;
}

static RdKafka::KafkaConsumer *create_consumer(const std::string &brokers, const std::string &group_id, std::string &errstr, Kafka::KafkaController::InternalRebalanceCb &rebalance_cb, Kafka::KafkaController::InternalLogger &loggerCb, const LogLevel logLevel) {
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

	// Setup Logging.
	if (conf->set("log_level", std::to_string(static_cast<int>(logLevel)), errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	if (conf->set("event_cb", &loggerCb, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	// Setup the brokers and group id.
	if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	if (conf->set("group.id", group_id, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	// Set the rebalance callback
	if (conf->set("rebalance_cb", &rebalance_cb, errstr) != RdKafka::Conf::CONF_OK) {
		return nullptr;
	}

	// Create a Rdkafka Consumer
	RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
	if (!consumer) {
		return nullptr;
	}

	return consumer;
}

static bool is_producer_connected(RdKafka::Producer *producer) {
	RdKafka::Metadata *metadata;
	RdKafka::ErrorCode err = producer->metadata(false, nullptr, &metadata, MAX_TIMEOUT_MS);
	if (err != RdKafka::ERR_NO_ERROR) {
		return false;
	}

	return true;
}

static bool is_consumer_connected(RdKafka::KafkaConsumer *consumer) {
	RdKafka::Metadata *metadata;
	RdKafka::ErrorCode err = consumer->metadata(false, nullptr, &metadata, MAX_TIMEOUT_MS);
	if (err != RdKafka::ERR_NO_ERROR) {
		return false;
	}

	return true;
}

#pragma endregion

/// ------------------------- KafkaController ------------------------- ///

Kafka::KafkaController::KafkaController() {}

Kafka::KafkaController::~KafkaController()
{
	clear();
}

void Kafka::KafkaController::add_producer(const std::string &brokers, const std::string &topic, const uint32_t channel)
{
	std::string errstr;

	// Reuse existing callbacks, if the producer already exists; otherwise create new objects.
	std::shared_ptr<InternalDeliveryReportCb> deliveryReportCb;
	std::shared_ptr<InternalLogger> loggerCb;

	if (m_producers.find(channel) != m_producers.end())
	{
		deliveryReportCb	= m_producers[channel].front()->get_delivery_report_cb();
		loggerCb			= m_producers[channel].front()->get_logger_cb();
	}
	else
	{
		deliveryReportCb	= std::make_shared<InternalDeliveryReportCb>();
		loggerCb			= std::make_shared<InternalLogger>();
	}

	RdKafka::Producer *producer = create_producer(brokers, errstr, *deliveryReportCb, *loggerCb, m_log_level);

	if (!producer) {
		//std::cerr << "Failed to create producer: " << errstr << std::endl;
		if (m_error_callback)
			m_error_callback("Failed to create producer: " + errstr);
		return;
	}

	// Set the delivery report callback
	deliveryReportCb->set_callback([this](RdKafka::Message &message) {
		printf("Message delivered to partition %d at offset %" PRId64 "\n", message.partition(), message.offset());

		if (message.err())
		{
			if (m_error_callback)
				m_error_callback("Message delivery failed: " + RdKafka::err2str(message.err()));
		}
	});
	loggerCb->set_callback([this](const LogLevel logLevel, const std::string &message) {
		if (m_log_callback)
			m_log_callback(logLevel, message);
	});

	// Create topic handle
	RdKafka::Topic *rd_topic = RdKafka::Topic::create(producer, topic, nullptr, errstr);

	// Create shared objects.
	std::shared_ptr<RdKafka::Producer> producer_shared(producer);
	std::shared_ptr<RdKafka::Topic> topic_shared(rd_topic);

	// Create Context
	std::shared_ptr<ProducerContext> producerContext = std::make_shared<ProducerContext>(
		producer_shared,
		topic_shared,
		loggerCb,
		deliveryReportCb
	);

	// Create a vector, if it doesn't exist.
	if (m_producers.find(channel) == m_producers.end()) {
		m_producers[channel] = std::vector<std::shared_ptr<ProducerContext>>();
	}

	// Add the producer to the vector.
	m_producers[channel].push_back(producerContext);
}

void Kafka::KafkaController::add_consumer(const std::string &brokers, const std::string &topic, const std::string &group_id, const uint32_t channel)
{
	std::string errstr;
	std::shared_ptr<InternalRebalanceCb> rebalance_cb;
	std::shared_ptr<InternalLogger> loggerCb;

	// Reuse existing callbacks, if the consumer already exists; otherwise create new objects.
	if (m_consumers.find(channel) != m_consumers.end())
	{
		rebalance_cb	= m_consumers[channel]->get_rebalance_cb();
		loggerCb		= m_consumers[channel]->get_logger_cb();
	}
	else
	{
		rebalance_cb = std::make_shared<InternalRebalanceCb>();
		loggerCb = std::make_shared<InternalLogger>();
	}

	RdKafka::KafkaConsumer *consumer = create_consumer(brokers, group_id, errstr, *rebalance_cb, *loggerCb, m_log_level);

	if (!consumer) {
		if (m_error_callback)
			m_error_callback("Failed to create consumer: " + errstr);
		return;
	}

	// Set the rebalance callback
	rebalance_cb->set_callback([this](RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err, std::vector<RdKafka::TopicPartition *> &partitions) {
		if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
			consumer->assign(partitions);
		} else {
			consumer->unassign();
		}

		if (err != RdKafka::ERR_NO_ERROR)
		{
			if (m_error_callback)
				m_error_callback("Rebalance callback: " + RdKafka::err2str(err));
		}
	});
	loggerCb->set_callback([this](const LogLevel logLevel, const std::string &message) {
		if (m_log_callback)
			m_log_callback(logLevel, message);
	});

	// Create topic handle
	RdKafka::Topic *rd_topic = RdKafka::Topic::create(consumer, topic, nullptr, errstr);

	// Create shared objects.
	std::shared_ptr<RdKafka::KafkaConsumer> consumer_shared(consumer);
	std::shared_ptr<RdKafka::Topic> topic_shared(rd_topic);
	std::shared_ptr<InternalLogger> loggerCb_shared(loggerCb);
	std::shared_ptr<InternalRebalanceCb> rebalance_cb_shared(rebalance_cb);

	// Create Context, if it doesn't exist.
	if (m_consumers.find(channel) == m_consumers.end())
	{
		// Create the thread.
		std::thread *consumer_thread = new std::thread(&KafkaController::_consumer_thread, this, m_consumers[channel], channel);
		std::shared_ptr<std::thread> thread_shared(consumer_thread);

		std::shared_ptr<ConsumerContext> consumerContext = std::make_shared<ConsumerContext>(
			thread_shared,
			std::vector<std::shared_ptr<RdKafka::KafkaConsumer>>(),
			loggerCb_shared,
			rebalance_cb_shared
		);

		m_consumers[channel] = consumerContext;
	}

	// Add the consumer to the vector.
	m_consumers[channel]->get_consumers().push_back(consumer_shared);

	// Wait until the consumer is connected.
	//  Timeout after the MAX_TIMEOUT_MS.
	int32_t timeout = 0;
	while (!is_consumer_connected(consumer) && timeout < MAX_TIMEOUT_MS)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		timeout += 100;
	}
}

void Kafka::KafkaController::clear_producers(const uint32_t channel)
{
	m_producers[channel].clear();
}

void Kafka::KafkaController::clear_consumers(const uint32_t channel)
{
	m_consumers[channel]->get_consumers().clear();
}

void Kafka::KafkaController::clear()
{
	m_producers.clear();
	m_consumers.clear();
}

bool Kafka::KafkaController::send(const uint32_t channel, const void *data, const uint32_t size, const std::optional<std::string> &key) {
	// Cannot find any producers for the channel.
	if (m_producers.find(channel) == m_producers.end())
	{
		return false;
	}

	// Iterate through each producer send the message.
	for (auto &producer : m_producers[channel]) {
		RdKafka::ErrorCode err;
		if (key.has_value()) {
			std::string key_str = key.value();

			err = producer->get_producer()->produce(
					producer->get_topic().get(),
					RdKafka::Topic::PARTITION_UA,
					RdKafka::Producer::RK_MSG_COPY,
					const_cast<void *>(data),
					size,
					key_str.c_str(),
					key_str.size(),
					nullptr);
		} else {
			err = producer->get_producer()->produce(
					producer->get_topic().get(),
					RdKafka::Topic::PARTITION_UA,
					RdKafka::Producer::RK_MSG_COPY,
					const_cast<void *>(data),
					size,
					nullptr,
					0,
					nullptr);
		}

		if (err) {
			if (m_error_callback)
				m_error_callback("Failed to produce message: " + RdKafka::err2str(err));

			return false;
		}
	}

	return true;
}

std::shared_ptr<Packet> Kafka::KafkaController::receive(const uint32_t channel) {
	if (m_consumers.find(channel) != m_consumers.end())
	{
		PacketNode *node = m_consumers[channel]->get_next_packet();
		if (node)
		{
			std::shared_ptr<Packet> packet = node->get_packet();
			m_consumers[channel]->pop_next_packet();
			return packet;
		}
	}
	return nullptr;
}

std::shared_ptr<Packet> Kafka::KafkaController::receive() {
	std::shared_ptr<Packet> packet = nullptr;
	for (auto &consumer : m_consumers)
	{
		packet = receive(consumer.first);
		if (packet)
		{
			return packet;
		}
	}

	return packet;
}

const uint32_t Kafka::KafkaController::get_packet_count(const uint32_t channel) const {

	// Check if the channel exists.
	uint32_t count = 0;
	if (m_consumers.find(channel) != m_consumers.end())
	{
		count = m_consumers.at(channel)->get_packet_count();
	}

	return count;
}

const uint32_t Kafka::KafkaController::get_packet_count() const
{
	uint32_t count = 0;
	for (auto &consumer : m_consumers)
	{
		count += consumer.second->get_packet_count();
	}
	return count;
}

void Kafka::KafkaController::set_log_level(const LogLevel level)
{
	m_log_level = level;
}

void Kafka::KafkaController::set_log_callback(const std::function<void(const LogLevel logLevel, const std::string &message)> callback)
{
	m_log_callback = callback;
}

void Kafka::KafkaController::set_error_callback(const std::function<void(const std::string &message)> callback)
{
	m_error_callback = callback;
}

void Kafka::KafkaController::_consumer_thread(std::shared_ptr<ConsumerContext> &consumer, const uint32_t channel)
{
	std::printf("Consumer thread started : %s\n", std::to_string(channel).c_str());
}

/// ------------------------- Packet ------------------------- ///

Kafka::Packet::~Packet() {
	delete[] data;
}

Kafka::Packet::Packet(const Packet &other) {
	size = other.size;
	data = new int8_t[size];
	memcpy(data, other.data, size);
}

int8_t *Kafka::Packet::get_data() const { return data; }

uint32_t Kafka::Packet::get_size() const { return size; }

/// ------------------------- PacketNode ------------------------- ///

Kafka::PacketNode::PacketNode(const PacketNode &other) {
	packet = other.packet;
	next = other.next.load();
}

std::shared_ptr<Packet> Kafka::PacketNode::get_packet() const { return packet; }

PacketNode *Kafka::PacketNode::get_next() const { return next.load(); }

void Kafka::PacketNode::set_next(PacketNode *node) { next.store(node); }

/// ------------------------- ProducerContext ------------------------- ///

Kafka::KafkaController::ProducerContext::~ProducerContext()
{
	topic.reset();

	producer->flush(MAX_TIMEOUT_MS);
	producer.reset();
}

Kafka::KafkaController::ProducerContext::ProducerContext(const ProducerContext &other)
{
	producer = other.producer;
	topic = other.topic;
}

bool Kafka::KafkaController::ProducerContext::is_healthy() const
{
	return is_producer_connected(producer.get());
}

std::shared_ptr<RdKafka::Producer> Kafka::KafkaController::ProducerContext::get_producer() const
{
	return producer;
}

std::shared_ptr<RdKafka::Topic> Kafka::KafkaController::ProducerContext::get_topic() const
{
	return topic;
}

std::shared_ptr<Kafka::KafkaController::InternalLogger> Kafka::KafkaController::ProducerContext::get_logger_cb() const {
	return logger;
}

std::shared_ptr<Kafka::KafkaController::InternalDeliveryReportCb> Kafka::KafkaController::ProducerContext::get_delivery_report_cb() const {
	return delivery_report_cb;
}


/// ------------------------- ConsumerContext ------------------------- ///

Kafka::KafkaController::ConsumerContext::~ConsumerContext() {
	// Delete all the packets in the queue.
	while (PacketNode *node = next_packet_head.load()) {
		next_packet_head.store(node->get_next());
		delete node;
	}

	// Clear consumers.
	for (auto &consumer : consumers) {
		consumer->close();
	}
	consumers.clear();

	// Update the running flag to false, so the thread can finish.
	running.store(false);
	// Wait for the thread to finish.
	thread->join();
}

Kafka::KafkaController::ConsumerContext::ConsumerContext(const ConsumerContext &other) {
	running.store(other.running.load());
	thread = other.thread;
	consumers = other.consumers;
}

PacketNode *Kafka::KafkaController::ConsumerContext::get_next_packet() const { return next_packet_head.load(); }

void Kafka::KafkaController::ConsumerContext::pop_next_packet() {
	PacketNode *node = next_packet_head.load();
	next_packet_head.store(node->get_next());
	delete node;
	packet_count.fetch_sub(1);
}

void Kafka::KafkaController::ConsumerContext::push_next_packet(PacketNode *node) {
	PacketNode *tail = next_packet_tail.load();
	if (tail)
	{
		tail->set_next(node);
	}
	else
	{
		next_packet_head.store(node);
	}
	next_packet_tail.store(node);
	packet_count.fetch_add(1);
}

const uint32_t Kafka::KafkaController::ConsumerContext::get_packet_count() const { return packet_count.load(); }

bool Kafka::KafkaController::ConsumerContext::is_healthy() const
{
	for (auto &consumer : consumers)
	{
		bool is_connected = is_consumer_connected(consumer.get());
		if (!is_connected)
		{
			return false;
		}
	}

	return true;
}

bool Kafka::KafkaController::ConsumerContext::is_running() const { return running.load(); }

void Kafka::KafkaController::ConsumerContext::set_running(bool value) { running.store(value); }

std::shared_ptr<Kafka::KafkaController::InternalLogger> Kafka::KafkaController::ConsumerContext::get_logger_cb() const {
	return logger;
}

std::shared_ptr<Kafka::KafkaController::InternalRebalanceCb> Kafka::KafkaController::ConsumerContext::get_rebalance_cb() const {
	return rebalance_cb;
}

std::vector<std::shared_ptr<RdKafka::KafkaConsumer>> Kafka::KafkaController::ConsumerContext::get_consumers() const {
	return std::vector<std::shared_ptr<RdKafka::KafkaConsumer>>();
}
