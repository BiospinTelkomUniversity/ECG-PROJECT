// EXPREES DAN SOCKET IO
const express = require('express'); // import package express
const app = express(); 
const server = require('http').createServer(app);
const io = require('socket.io').listen(server); // import package socket.io
const path = require('path'); // import package path (sudah default ada)
var mysql = require('mysql');

var con = mysql.createConnection({
  host: "localhost",
  user: "root",
  password: "",
  database: "dataekg"
});

app.use(express.static(path.join(__dirname,'www'))); // untuk nempation file web kita di folder www
const portListen = 1234;
server.listen(portListen);
// /*============================
// =            MQTT            =
// ============================*/
const mqtt = require('mqtt');
const topic = 'Test/datasensor'; //subscribe to all topics
const broker_server = 'mqtt://192.168.1.109';

const options = {
	clientId : 'MyMQTT',
	port : 1883,
	keepalive : 60
}

const clientMqtt = mqtt.connect(broker_server,options);
clientMqtt.on('connect', mqtt_connect);
clientMqtt.on('reconnect', mqtt_reconnect);
clientMqtt.on('error', mqtt_error);
clientMqtt.on('message', mqtt_messageReceived);

function mqtt_connect() {
	clientMqtt.subscribe(topic);
}

function mqtt_reconnect(err){
	//clientMqtt = mqtt.connect(broker_server, options); // reconnect
}

function mqtt_error(err){
	console.log(err);
}

function after_publish() {
	//call after publish
}

function mqtt_messageReceived(topic , message , packet){
	// let dataKirim = message;
	// console.log(dataKirim);
	let dataKirim = parsingRAWData(message,",");
	if (topic == 'Test/datasensor'){
		console.log(dataKirim);
		io.sockets.emit('kirim', {datasens : dataKirim});
	}

	var sql = "INSERT INTO `datastream`(`data`, `bpm`) VALUES (" + dataKirim[0] + "," + dataKirim[1] + ")";
	// con.query(sql, function (err, result) {
	// 	if (err) throw err;
	// 	//console.log("1 record inserted");
	// });
}

function parsingRAWData(data,delimiter){
	let result;
	result = data.toString().replace(/(\r\n|\n|\r)/gm,"").split(delimiter);
	return result;
}