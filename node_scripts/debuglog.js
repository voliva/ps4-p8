const net = require('net');

const PORT = 9025;
const ADDR = process.argv[2];

console.log(`connecting to ${ADDR}:${PORT}`);

const client = new net.Socket();
client.connect(PORT, process.argv[2], function () {
  console.log('Connected');
});

client.on('data', function (data) {
  console.log(data);
});

client.on('close', function () {
  console.log('Connection closed');
});
