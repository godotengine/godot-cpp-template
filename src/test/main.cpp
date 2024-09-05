#include <iostream>
#include "kafka.hpp"

static void log_callback(Kafka::LogLevel severity, const std::string &message)
{
	// Colorize Prefix.
	std::string color;
	switch (severity)
	{
		case Kafka::LogLevel::CRIT:
		case Kafka::LogLevel::ERR:
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

void on_error(const std::string &message)
{
	printf("\033[1;31mError: %s\n\033[0m", message.c_str());
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
	controller.add_producer(brokers, "test", 1);
	printf("Kafka Producer added.\n");

	controller.add_consumer(brokers, { "test" }, "test_group", 1);
	printf("Kafka Consumer added.\n");

	// Logic.
	std::string msg = "Hello, World!";

	// Send a message to the Kafka Producer.
	if (controller.send(
		1,
		msg.c_str(),
		msg.size()))
	{
		printf("Message sent.\n");
	} else {
		printf("Failed to send message.\n");
		return EXIT_FAILURE;
	}


	//while (true)
	{
		// Sleep.
		std::this_thread::sleep_for(std::chrono::seconds(1));

		// Receive a message from the Kafka Consumer.
		std::shared_ptr<Kafka::Packet> message = controller.receive();

		printf("Consumed.\n");

		if (message != nullptr)
		{
			printf("Message: %d bytes\n", message->get_size());
		}
		else
		{
			printf("No message.\n");
		}
	}

	return EXIT_SUCCESS;
}
