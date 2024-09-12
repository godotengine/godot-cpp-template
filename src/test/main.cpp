#include <iostream>
#include "kafka.hpp"

static void log_callback(Kafka::LogLevel severity, const std::string &message)
{
	// Colorize Prefix.
	std::string color;
	switch (severity)
	{
		case Kafka::LogLevel::CRITICAL:
		case Kafka::LogLevel::ERROR:
			color = "\033[1;31m";
			break;
		case Kafka::LogLevel::WARNING:
			color = "\033[1;33m";
			break;
		case Kafka::LogLevel::INFO:
			color = "\033[1;34m";
			break;
		case Kafka::LogLevel::DEBUG:
			color = "\033[1;32m";
			break;
		default:
			color = "\033[0m";
			break;
	}

	printf("%s[%s] %s\n\033[0m", color.c_str(), Kafka::LogLevelToString(severity).c_str(), message.c_str());
}

static void on_error(const std::string &message)
{
	printf("\033[1;31mError: %s\n\033[0m", message.c_str());
}

static bool is_number(const std::string &s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

int main()
{
	// Create a Kafka Producer.
	Kafka::KafkaController controller;

	std::string brokers = "localhost:19092";

	// Settings.
	controller.set_log_level(Kafka::LogLevel::INFO);
	printf("Log level set.\n");

	controller.set_log_callback(log_callback);
	printf("Log callback set.\n");

	controller.set_error_callback(on_error);
	printf("Error callback set.\n");

	printf("Kafka Controller created.\n");

	// Add a Kafka Producer and Consumer.
	{
		Kafka::KafkaProducerMetadata producer_metadata;
		producer_metadata.brokers = brokers;
		producer_metadata.topic = "test";
		controller.add_producer(producer_metadata, 1);
		printf("Kafka Producer added.\n");
	}

	{
		Kafka::KafkaConsumerMetadata consumer_metadata;
		consumer_metadata.brokers = brokers;
		consumer_metadata.topics = { "test" };
		controller.add_consumer(consumer_metadata, 1);
		printf("Kafka Consumer added.\n");
	}

	// Logic.
	

	// Collect the delta time between each frame.
	int count = 0;
	double average = 0.0;

	while (true)
	{
		if (count > 100)
		{
			printf("--------------------\n");
			printf("Average: %fms\n", (average / count) * 1000);

			// Sleep for 1 second.
			std::this_thread::sleep_for(std::chrono::seconds(10));

			// Reset the count and average.
			count = 0;
			average = 0.0;
		}

		std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();
		std::string msg = std::to_string(start_time.time_since_epoch().count());

		// Send a message to the Kafka Producer.
		controller.send(
			1,
			msg.c_str(),
			msg.size()
		);
		// Receive a message from the Kafka Consumer.
		std::shared_ptr<Kafka::Packet> message = controller.receive();

		//printf("Consumed.\n");
		count++;

		if (message != nullptr)
		{
			// Convert the message to a string.
			std::string message_str = std::string((char *)(message->get_data()), message->get_size());

			// Check if message is an integer.
			if (!is_number(message_str))
			{
				printf("Message is not a number.\n");
				continue;
			}

			long long int time = std::stoll(message_str);

			std::chrono::time_point end_time = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> delta_time = end_time - start_time;

			double delta = delta_time.count();

			average += delta;

			printf("Delta Time: %fms\n", delta);
		}

	}

	return EXIT_SUCCESS;
}
