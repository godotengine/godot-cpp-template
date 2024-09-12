extends Node

var controlled_peers: Dictionary = {}

var local_ping: int = 0
var ping_delay_secs: int = 5
var ping_timer: Timer
var timeout_peer_secs: int = 5

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var host: String
	# if OS.has_feature("dedicated_server"):
	host = "localhost:19092" # Kafka Server
	# else:
	# 	host = "ws://localhost:3000/ws" # Websocket Client
	init_network(host)

func init_network(host: String) -> MultiplayerPeer:
	var socket: MultiplayerPeer
	# if OS.has_feature("dedicated_server"):
	print("Connecting to Kafka Broker: ", host)
	var kafka: KafkaMultiplayerPeer = KafkaMultiplayerPeer.new()
	kafka.kafka_log.connect(_on_kafka_logs)
	# Setup Internal Channels...
	#	These channels do not communicate from client to server, instead this communicates between server to only edge internally.
	#	This allows the edge server to tell the game servers that a client has connected or disconnected.
	var internal_producer_metadata: KafkaPublisherMetadata = KafkaPublisherMetadata.new()
	internal_producer_metadata.set_brokers(host)
	internal_producer_metadata.set_topic("internal_to_edge")
	kafka.register_internal_publisher(internal_producer_metadata)
	
	var internal_consumer_metadata: KafkaSubscriberMetadata = KafkaSubscriberMetadata.new()
	internal_consumer_metadata.set_brokers(host)
	internal_consumer_metadata.set_topics(["internal_to_game"])
	internal_consumer_metadata.set_group_id("group-name")
	internal_consumer_metadata.set_offset_reset(KafkaSubscriberMetadata.OffsetReset.EARLIEST)
	kafka.register_internal_subscriber(internal_consumer_metadata)
	
	# Setup Producer
	# 	This communicates directly to the clients via kafka topics.
	var producer_metadata: KafkaPublisherMetadata = KafkaPublisherMetadata.new()
	producer_metadata.set_brokers(host)
	producer_metadata.set_topic("server-to-client")
	kafka.register_publisher(producer_metadata, 1)
	
	# Setup Consumer
	#	This listens from the clients via the kafka topics.
	var consumer_metadata: KafkaSubscriberMetadata = KafkaSubscriberMetadata.new()
	consumer_metadata.set_brokers(host)
	consumer_metadata.set_topics(["client-to-server"])
	consumer_metadata.set_group_id("group-name")
	consumer_metadata.set_offset_reset(KafkaSubscriberMetadata.OffsetReset.EARLIEST)
	kafka.register_subscriber(consumer_metadata, 1)
	
	kafka.peer_connected.connect(_server_on_connection)
	kafka.peer_disconnected.connect(_server_on_disconnection)
	
	socket = kafka
	
	# Initialize the ping timer for the server.
	ping_timer = Timer.new()
	ping_timer.timeout.connect(_on_ping_caller)
	ping_timer.wait_time = ping_delay_secs
	add_child(ping_timer)
	ping_timer.start()
	# else:
	# 	print("Connecting to a Websocket URL: ", host)
	# 	var websocket: WebSocketMultiplayerPeer = WebSocketMultiplayerPeer.new()
	# 	websocket.create_client(host)
	# 	websocket.peer_connected.connect(_on_connected)
	# 	socket = websocket

	# Set the Multiplayer API to be pointed on the peer above.
	# At this point, the peer should be polling data.
	multiplayer.multiplayer_peer = socket
	
	return socket

func _on_kafka_logs(log_severity: int, message: String) -> void:
	print("[KAFKA::%s] %s" % [str(log_severity), message])

func _on_ping_caller() -> void:
	print("[SERVER] Pinging all clients...")
	
	# Check timeouts for peers.
	for controlled_peer in controlled_peers.keys():
		var last_ping_timestamp = controlled_peers[controlled_peer]
		var now: float = Time.get_unix_time_from_system()
		if now > last_ping_timestamp + ping_delay_secs + timeout_peer_secs:
			multiplayer.multiplayer_peer.disconnect_peer(controlled_peer, true)
	
	ping_all()

func _server_on_connection(source: int) -> void:
	print("[SERVER] Client connected: ", source);

func _server_on_disconnection(source: int) -> void:
	print("[SERVER] Client disconnected: ", source)

func _on_connected():
	print("connected...")
	login.rpc("Hello", "World")

@rpc("authority")
func login_callback(success: bool) -> void:
	if multiplayer.is_server():
		printerr("Server should not be processing this message...")
		return
	print("Login Successfully: ", success)

@rpc("any_peer")
func login(username: String, password: String) -> void:
	if !multiplayer.is_server():
		printerr("Client should not be processing this message...")
		return
	
	print("Sending Login...")
	login_callback.rpc_id(multiplayer.get_remote_sender_id(), true)

func ping_all() -> void:
	if !multiplayer.is_server():
		return
		
	for controlled_peer in controlled_peers.keys():
		var last_timestamp = controlled_peers[controlled_peer]
		ping.rpc_id(controlled_peer, last_timestamp)

@rpc("any_peer")
func ping(last_timestamp: int) -> void:
	if multiplayer.is_server():
		return
	pong.rpc()

@rpc("any_peer")
func pong() -> void:
	if !multiplayer.is_server():
		return
	
	var peer: int = multiplayer.get_remote_sender_id()
	controlled_peers[peer] = Time.get_ticks_msec()
