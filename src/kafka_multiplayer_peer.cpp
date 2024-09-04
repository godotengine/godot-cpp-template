#include "kafka_multiplayer_peer.h"

#include <rdkafkacpp.h>

using namespace godot;

class InternalDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
	void dr_cb(RdKafka::Message &message) override {
		std::cout << "Message delivered to partition " << message.partition()
				  << " at offset " << message.offset() << std::endl;
	}
};

class InternalRebalanceCb : public RdKafka::RebalanceCb {
public:
	void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
			std::vector<RdKafka::TopicPartition *> &partitions) override {
		std::cout << "Rebalance callback: " << RdKafka::err2str(err) << std::endl;

		if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
			consumer->assign(partitions);
		} else {
			consumer->unassign();
		}
	}
};

void KafkaMultiplayerPeer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_publisher", "topic", "brokers", "channel"), &KafkaMultiplayerPeer::register_publisher);
	//ClassDB::bind_method(D_METHOD("register_subscriber", "topic", "brokers", "channel"), &KafkaMultiplayerPeer::register_subscriber);
}

KafkaMultiplayerPeer::KafkaMultiplayerPeer()
{

}

KafkaMultiplayerPeer::~KafkaMultiplayerPeer()
{
	// Resource Acquisition Is Initialization (RAII) of all the information of the peer.
	// Cleanup Reading packets.
	while (PacketNode *node = m_read.load())
	{
		m_read.store(node->next);
		delete node;
	}

	// Cleanup Publish/Subscriber Brokers
	// TODO :: ...
}

void KafkaMultiplayerPeer::register_publisher(const godot::String &topic, const godot::String &brokers, const int32_t channel) {
	std::string topic_str = topic.utf8().get_data();
	std::string brokers_str = brokers.utf8().get_data();
	// Configure the producer.
	std::string errstr;
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

	if (conf->set("bootstrap.servers", brokers_str, errstr) != RdKafka::Conf::CONF_OK)
	{
		// TODO :: Handle Error
		return;
	}

    // Set the delivery report callback
	InternalDeliveryReportCb ex_dr_cb;
	conf->set("dr_cb", &ex_dr_cb, errstr);

	// Create a Rdkafka Producer
	RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
	if (!producer)
	{
		// TODO :: Handle Error
		return;
	}

	RdKafka::Topic *rd_topic = RdKafka::Topic::create(producer, topic_str, nullptr, errstr);
	if (!rd_topic)
	{
		// TODO :: Handle Error
		return;
	}

	std::shared_ptr<RdKafka::Producer> producer_shared(producer);
	std::shared_ptr<RdKafka::Topic> topic_shared(rd_topic);

	// Store the producer and topic in the context.
	ProducerContext context(producer_shared, topic_shared);

	// Create a list of producers of the channel key, if not exists.
	if (m_producers.find(channel) == m_producers.end())
	{
		m_producers[channel] = std::vector<std::shared_ptr<ProducerContext>>();
	}

	// Add the producer to the list of producers of the channel key.
	m_producers[channel].push_back(std::make_shared<ProducerContext>(context));
}

void KafkaMultiplayerPeer::register_subscriber(const std::vector<std::string> &topics, const std::string &brokers, const std::string &group_id, const int32_t channel)
{
	// Configure the consumer.
	std::string errstr;
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

	if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK)
	{
		// TODO :: Handle Error
		return;
	}

	if (conf->set("group.id", group_id, errstr) != RdKafka::Conf::CONF_OK)
	{
		// TODO :: Handle Error
		return;
	}

	InternalRebalanceCb rebalance_cb;
	if (conf->set("rebalance_cb", &rebalance_cb, errstr) != RdKafka::Conf::CONF_OK)
	{
		// TODO :: Handle Error
		return;
	}

	// Create a Rdkafka Consumer
	RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
	if (!consumer)
	{
		// TODO :: Handle Error
		return;
	}

	// Subscribe to the topic.
	RdKafka::ErrorCode err = consumer->subscribe(topics);
	if (err)
	{
		// TODO :: Handle Error
		return;
	}

	std::shared_ptr<RdKafka::KafkaConsumer> consumer_shared(consumer);

	// Create a list of consumers of the channel key, if not exists.
	if (m_consumers.find(channel) == m_consumers.end())
	{
		m_consumers[channel] = std::vector<std::shared_ptr<RdKafka::KafkaConsumer>>();
	}

	// Add the consumer to the list of consumers of the channel key.
	m_consumers[channel].push_back(consumer_shared);
}
