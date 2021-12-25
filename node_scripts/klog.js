// node klog.js 192.168.1.38 | tee -a klog.log

const Client = require('ftp');

const c = new Client();

c.on('ready', function () {
  c.get('/dev/klog', function (err, stream) {
    if (err) throw err;
    stream.once('close', function () { c.end(); });
    stream.pipe(process.stdout);
  });
});

c.on('error', error => console.error(error));

c.connect({
  host: process.argv[2],
  port: 2121
});
