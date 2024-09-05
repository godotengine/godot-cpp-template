#include "kafka_multiplayer_peer.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <functional>
#include <rdkafkacpp.h>

using namespace godot;

class InternalDeliveryReportCb : public RdKafka::DeliveryReportCb {
	std::function<void(RdKafka::Message &)> callback;
public:
	void dr_cb(RdKafka::Message &message) override {
		/*std::cout << "Message delivered to partition " << message.partition()
				  << " at offset " << message.offset() << std::endl;*/

		if (callback)
		{
			callback(message);
		}
	}

	void set_callback(std::function<void(RdKafka::Message &)> cb) {
		callback = cb;
	}
};

class InternalRebalanceCb : public RdKafka::RebalanceCb {
	std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> callback;
public:
	void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
			std::vector<RdKafka::TopicPartition *> &partitions) override {
		/*std::cout << "Rebalance callback: " << RdKafka::err2str(err) << std::endl;*/

		if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
			consumer->assign(partitions);
		} else {
			consumer->unassign();
		}

		if (callback)
		{
			callback(consumer, err, partitions);
		}
	}

	void set_callback(std::function<void(RdKafka::KafkaConsumer *, RdKafka::ErrorCode, std::vector<RdKafka::TopicPartition *> &)> cb) {
		callback = cb;
	}
};

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
	return godot::Error();
}

godot::Error KafkaMultiplayerPeer::_put_packet(const uint8_t *p_buffer, int32_t p_buffer_size)
{
	return godot::Error();
}

int32_t KafkaMultiplayerPeer::_get_available_packet_count() const
{
	// Collect all the packet counts from the consumers.
	int32_t packet_count = 0;
	for (auto &channel_consumers : m_consumers)
	{
		packet_count += channel_consumers.second->get_packet_count();
	}

	return packet_count;
}

godot::PackedByteArray KafkaMultiplayerPeer::_get_packet_script()
{
	return godot::PackedByteArray();
}

godot::Error KafkaMultiplayerPeer::_put_packet_script(const godot::PackedByteArray &p_buffer)
{
	return godot::Error();
}

int32_t KafkaMultiplayerPeer::_get_packet_channel() const
{
	return 0;
}

godot::MultiplayerPeer::TransferMode KafkaMultiplayerPeer::_get_packet_mode() const
{
	return godot::MultiplayerPeer::TransferMode();
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
	m_producers.clear();
	m_consumers.clear();
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

static bool is_producer_connected(RdKafka::Producer *producer)
{
	RdKafka::Metadata *metadata;
	RdKafka::ErrorCode err = producer->metadata(false, nullptr, &metadata, 5000);
	if (err != RdKafka::ERR_NO_ERROR)
	{
		return false;
	}

	return true;
}

static bool is_consumer_connected(RdKafka::KafkaConsumer *consumer)
{
	RdKafka::Metadata *metadata;
	RdKafka::ErrorCode err = consumer->metadata(false, nullptr, &metadata, 5000);
	if (err != RdKafka::ERR_NO_ERROR)
	{
		return false;
	}

	return true;
}

godot::MultiplayerPeer::ConnectionStatus KafkaMultiplayerPeer::_get_connection_status() const
{
	// Check if consumers and producers are empty.
	if (m_producers.empty() && m_consumers.empty())
	{
		return godot::MultiplayerPeer::CONNECTION_DISCONNECTED;
	}

	// Check all the producers and consumers.
	for (auto &channel_producers : m_producers)
	{
		for (auto &producer : channel_producers.second)
		{
			if (!is_producer_connected(producer->get_producer().get()))
			{
				return godot::MultiplayerPeer::CONNECTION_DISCONNECTED;
			}
		}
	}

	for (auto &channel_consumers : m_consumers)
	{
		for (auto &consumer : channel_consumers.second->get_consumers())
		{
			if (!is_consumer_connected(consumer.get()))
			{
				return godot::MultiplayerPeer::CONNECTION_DISCONNECTED;
			}
		}
	}

	return MultiplayerPeer::CONNECTION_CONNECTED;
}

static RdKafka::Producer *create_producer(const std::string &brokers, std::string &errstr, InternalDeliveryReportCb &deliveryReportCb)
{
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

	if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK)
	{
		return nullptr;
	}

	// Set the delivery report callback
	if (conf->set("dr_cb", &deliveryReportCb, errstr) != RdKafka::Conf::CONF_OK)
	{
		return nullptr;
	}

	// Create a Rdkafka Producer
	RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
	if (!producer)
	{
		return nullptr;
	}

	return producer;
}

static RdKafka::KafkaConsumer *create_consumer(const std::string &brokers, const std::string &group_id, std::string &errstr, InternalRebalanceCb &rebalance_cb)
{
	RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

	if (conf->set("bootstrap.servers", brokers, errstr) != RdKafka::Conf::CONF_OK)
	{
		return nullptr;
	}

	if (conf->set("group.id", group_id, errstr) != RdKafka::Conf::CONF_OK)
	{
		return nullptr;
	}

	// Set the rebalance callback
	if (conf->set("rebalance_cb", &rebalance_cb, errstr) != RdKafka::Conf::CONF_OK)
	{
		return nullptr;
	}

	// Create a Rdkafka Consumer
	RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
	if (!consumer)
	{
		return nullptr;
	}

	return consumer;
}

void KafkaMultiplayerPeer::register_admin_channel(const godot::String &brokers, const godot::String &topics_prefix)
{
	std::string brokers_str = brokers.utf8().get_data();
	std::string topics_prefix_str = topics_prefix.utf8().get_data();

	std::string admin_out_topic = topics_prefix_str + "_admin_out";
	std::string admin_in_topic = topics_prefix_str + "_admin_in";

	// Create a Producer Context for the Admin Channel.
	{
		std::string errstr;
		InternalDeliveryReportCb deliveryReportCb;
		RdKafka::Producer *producer = create_producer(brokers_str, errstr, deliveryReportCb);
		if (!producer) {
			emit_signal("publisher_error", String(("[Channel: ADMIN] Failed to create producer: " + errstr).c_str()));
			return;
		}

		deliveryReportCb.set_callback([this](RdKafka::Message &message) {
			std::cout << "[ADMIN] Message delivered to partition " << message.partition()
					  << " at offset " << message.offset() << std::endl;

			// Emit the error signal.
			if (message.err() != RdKafka::ERR_NO_ERROR) {
				emit_signal("publisher_error", String(("[Channel: ADMIN] Message delivery error: " + RdKafka::err2str(message.err())).c_str()));
			}
		});

		RdKafka::Topic *rd_topic = RdKafka::Topic::create(producer, admin_out_topic, nullptr, errstr);
		if (!rd_topic) {
			emit_signal("publisher_error", String(("[Channel: ADMIN] Failed to create topic: " + errstr).c_str()));
			return;
		}

		std::shared_ptr<RdKafka::Producer> producer_shared(producer);
		std::shared_ptr<RdKafka::Topic> topic_shared(rd_topic);

		// Store the producer and topic in the context.
		ProducerContext producer_context(producer_shared, topic_shared);

		m_admin_producer = std::make_shared<ProducerContext>(producer_context);
	}

	// Create a Consumer Context for the Admin Channel.
	{
		std::string errstr;
		InternalRebalanceCb rebalance_cb;
		RdKafka::KafkaConsumer *consumer = create_consumer(brokers_str, "admin_group", errstr, rebalance_cb);
		if (!consumer) {
			emit_signal("subscriber_error", String(("[Channel: ADMIN] Failed to create consumer: " + errstr).c_str()));
			return;
		}

		rebalance_cb.set_callback([this](RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err, std::vector<RdKafka::TopicPartition *> &partitions) {
			//std::cout << "[ADMIN] Rebalance callback: " << RdKafka::err2str(err) << std::endl;

			// Emit the error signal.
			if (err != RdKafka::ERR_NO_ERROR) {
				emit_signal("subscriber_error", String(("[Channel: ADMIN] Rebalance error: " + RdKafka::err2str(err)).c_str()));
			}
		});

		// Subscribe to the topic.
		RdKafka::ErrorCode err = consumer->subscribe({admin_in_topic});
		if (err) {
			emit_signal("subscriber_error", String(("[Channel: ADMIN] Failed to subscribe to topics: " + RdKafka::err2str(err)).c_str()));
			return;
		}

		std::shared_ptr<RdKafka::KafkaConsumer> consumer_shared(consumer);

		// Store the consumer in the context.

		std::thread *consumer_thread = new std::thread(&KafkaMultiplayerPeer::_consumer_thread, this, m_admin_consumer, 0);

		std::shared_ptr<std::thread> consumer_thread_shared(consumer_thread);

		ConsumerContext consumer_context(consumer_thread_shared, std::vector<std::shared_ptr<RdKafka::KafkaConsumer>>());
		consumer_context.get_consumers().push_back(consumer_shared);

		m_admin_consumer = std::make_shared<ConsumerContext>(consumer_context);
	}

}

void KafkaMultiplayerPeer::register_publisher(const godot::String &topic, const godot::String &brokers, const int32_t channel) {
	std::string topic_str = topic.utf8().get_data();
	std::string brokers_str = brokers.utf8().get_data();

	// Create a Producer Context for the Channel.
	std::string errstr;

	InternalDeliveryReportCb deliveryReportCb;
	RdKafka::Producer *producer = create_producer(brokers_str, errstr, deliveryReportCb);
	if (!producer)
	{
		emit_signal("publisher_error", String(("[Channel: " + std::to_string(channel) + "] Failed to create producer: " + errstr).c_str()));
		return;
	}

	deliveryReportCb.set_callback([this, channel](RdKafka::Message &message) {
		std::cout << "Message delivered to partition " << message.partition()
				  << " at offset " << message.offset() << std::endl;

		// Emit the error signal.
		if (message.err() != RdKafka::ERR_NO_ERROR) {
			emit_signal("publisher_error", String(("[Channel: " + std::to_string(channel) + "] Message delivery error: " + RdKafka::err2str(message.err())).c_str()));
		}
	});

	RdKafka::Topic *rd_topic = RdKafka::Topic::create(producer, topic_str, nullptr, errstr);
	if (!rd_topic)
	{
		emit_signal("publisher_error", String(("[Channel: " + std::to_string(channel) + "] Failed to create topic: " + errstr).c_str()));
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

	InternalRebalanceCb rebalance_cb;
	RdKafka::KafkaConsumer *consumer = create_consumer(brokers_str, group_id_str, errstr, rebalance_cb);
	if (!consumer)
	{
		emit_signal("subscriber_error", String(("[Channel: " + std::to_string(channel) + "] Failed to create consumer: " + errstr).c_str()));
		return;
	}

	rebalance_cb.set_callback([this, channel](RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err, std::vector<RdKafka::TopicPartition *> &partitions) {
		//std::cout << "Rebalance callback: " << RdKafka::err2str(err) << std::endl;

		// Emit the error signal.
		if (err != RdKafka::ERR_NO_ERROR) {
			emit_signal("subscriber_error", String(("[Channel: " + std::to_string(channel) + "] Rebalance error: " + RdKafka::err2str(err)).c_str()));
		}
	});

	// Subscribe to the topic.
	RdKafka::ErrorCode err = consumer->subscribe(topics_str);
	if (err)
	{
		emit_signal("subscriber_error", String(("[Channel: " + std::to_string(channel) + "] Failed to subscribe to topics: " + RdKafka::err2str(err)).c_str()));
		return;
	}

	std::shared_ptr<RdKafka::KafkaConsumer> consumer_shared(consumer);

	// Create a list of consumers of the channel key, if not exists.
	if (m_consumers.find(channel) == m_consumers.end())
	{
		ConsumerContext *consumer_context;

		// Create separate thread for each new channel.
		std::thread *consumer_thread = new std::thread(&KafkaMultiplayerPeer::_consumer_thread, this, m_consumers[channel], channel);
		std::shared_ptr<std::thread> consumer_thread_shared = std::shared_ptr<std::thread>(consumer_thread);

		consumer_context = new ConsumerContext(consumer_thread_shared, std::vector<std::shared_ptr<RdKafka::KafkaConsumer>>());

		m_consumers[channel] = std::shared_ptr<ConsumerContext>(consumer_context);
	}

	// Add the consumer to the list of consumers of the channel key.
	m_consumers[channel]->get_consumers().push_back(consumer_shared);
}

void KafkaMultiplayerPeer::_consumer_thread(std::shared_ptr<ConsumerContext> &consumer, const uint32_t channel) {
	// Write a test message to the console.
	std::cout << "Consumer thread started for channel " << channel << std::endl;

	// TODO :: Implement the consumer thread.
}
