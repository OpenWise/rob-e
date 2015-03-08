var os = require('os');
const express       = require('express');
const config        = require('configure');
const SqliteAdapter = require('./sqlite_adapter.js')(console);
const RedisAdapter  = require('./redis_adapter.js')(console);

var db      = new SqliteAdapter(config.dbName);
var redis   = new RedisAdapter(config.redisChannel, config.redisPort, config.redisHost, config.redisOpt);
var app     = express();

redis.on ("DB", function (data) {
    console.log("Save to DB triggered.");
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
    var data = '{"handler":1,"x":' + req.param('x') + 
                           ',"y":' + req.param('y') + 
                           ',"z":' + req.param('z') +
                           ',"p":' + req.param('p') + '}';
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

apiRouter.route('/set_servo_angle/:id/:angle').get(function(req, res) {
    var data = '{"handler":2,"id":' + req.param('id') + ',"angle":' + req.param('angle') + '}';    
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
    var channel = config.sensorChannel;
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
    
    var onServoValueUpdate = function (data) {
        console.log("Update client triggered.");
        writeSSE(data);
    }

    console.log("Register for " + sid);
    redis.on(sid, onServoValueUpdate);
    req.on("close", function() {
        redis.removeListener(sid, onServoValueUpdate);
    });
    console.log("Done for " + sid);
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
