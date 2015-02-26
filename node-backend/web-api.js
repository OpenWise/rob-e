var os = require('os');
const express       = require('express');
const config        = require('configure');
const SqliteAdapter = require('./sqlite_adapter.js')(console);
const RedisAdapter  = require('./redis_adapter.js')(console);

var db      = new SqliteAdapter(config.dbName);
var redis   = new RedisAdapter(config.redisChannel, config.redisPort, config.redisHost, config.redisOpt);
var app     = express();

redis.on(config.sensorChannel, function(data, type) {
});

//enable CORS
app.use(function(req, res, next) {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    res.header('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE');
    next();
});

var apiRouter = express.Router();
var sseRouter = express.Router();

app.use('/api', apiRouter);
app.use('/sse', sseRouter);

apiRouter.route('/').get(function(req, res) {
    res.end('API');
});

apiRouter.route('/debug').get(function(req, res) {
    res.end('DEBUG');
});

apiRouter.route('/set_position/:x/:y/:z/:p').get(function(req, res) {
    res.end('OK');
});

apiRouter.route('/set_servo_angle/:id/:angle').get(function(req, res) {
    var data = '{"handler":2,"id":' + req.param('id') + ',"angle":' + req.param('angle') + '}';
    console.log(data);
    
    redis.Publish("ROBE-IN", data, function(err, info) {
        if (err) {
            console.error("ERROR");
            res.status(500).send(err.message);
        } else {
            console.error("GOOD");
        }
    });
    res.end('OK');
});


var msgID = 0;

sseRouter.route('/servo/:id').get(function(req, res) {
    var sid = req.params.id;
    console.log("Received new listener for sensor id " + sid);
    req.socket.setTimeout(Infinity);
    
    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive'
    });
    res.write('\n');

    var writeSSE = function(data) {
        res.write("id: " + msgID++ +"\n");
        res.write("data: " + data);
        res.write("\n\n");
    }

    redis.GetServoValue(sid, function(err, data) {
        if (err) {
            console.error(err.message);
        } else {
            if (data) {
                console.log("Initial SSE value from REDIS: " + sid + " " + data);
                var sensor = JSON.parse(data);
                writeSSE(sensor.value);
            } else {
                console.log("Servo value not found in REDIS: " + sid);
            }
        }
    });
    
    var onServoValueUpdate = function(data) {
        console.log("Received servo value event for id " + sid + ": " + data);
        writeSSE(data);
    }
    
    var subid = redis.on(sid, onServoValueUpdate);
    req.on("close", function() {
        redis.removeListener(sid, onServoValueUpdate);
    });
});

var server = app.listen(config.serverPort, function() {
    console.info('Listening on port ' + server.address().port);
    printIPAdress ();
});

function printIPAdress () {
    var ifaces = os.networkInterfaces();
    Object.keys(ifaces).forEach(function (ifname) {
        var alias = 0;
        ifaces[ifname].forEach(function (iface) {
            if ('IPv4' !== iface.family || iface.internal !== false) {
                // skip over internal (i.e. 127.0.0.1) and non-ipv4 addresses
                return;
            }

            if (alias >= 1) {
                // this single interface has multiple ipv4 addresses
                console.log(ifname + ':' + alias, iface.address);
            } else {
                // this interface has only one ipv4 adress
                console.log(ifname, iface.address);
            }
        });
    });
}
