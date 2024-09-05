extends Node


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.

func init_network(host: String) -> MultiplayerPeer:
	var socket: MultiplayerPeer;
	#if OS.has_feature("dedicated_server"):
	print("Connecting to Kafka Broker: ", host)
	var kafka: KafkaMultiplayerPeer = KafkaMultiplayerPeer.new();
	kafka.register_publisher("server-to-client", host, 1);
	kafka.register_subscriber(["client-to-server"], host, "group-name", 1);
	socket = kafka;
	#else:
		#print("Connecting to a Websocket URL: ", host)
		#var websocket: WebSocketMultiplayerPeer = WebSocketMultiplayerPeer.new();
		#websocket.create_client(host)
		#socket = websocket;
	#
	# Set the Multiplayer API to be pointed on the peer above.
	# At this point, the peer should be polling data.
	multiplayer.multiplayer_peer = socket;
	return socket;
