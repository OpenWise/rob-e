const redis  = require('redis');
const events = require('events');

var console;

function RedisAdapter(redisChannel, port, host, options) {
    this.subscribe  = redis.createClient(port, host, options);
    this.publish    = redis.createClient(port, host, options);
    this.client     = redis.createClient(port, host, options);
    events.EventEmitter.call(this);
    
    // Clear REDIS keys
    this.client.flushdb ( function (err, didSucceed) {
        console.log("REDIS FLUSHDB - " + didSucceed);
    });

    this.subscribe.subscribe(redisChannel);
    var self = this;
    this.subscribe.on("message", function(channel, message) {
        console.log("[Channel] " + channel);
        switch (channel) {
            case redisChannel:
                var data = JSON.parse(message);
                self.emit ("DB", message);
                self.emit (data.id, message);
            break;
        }
    });
    console.info("REDIS ADAPTER: subscribed for channel " + redisChannel);
}

RedisAdapter.prototype.GetServoValue = function(sid, callback) {
    this.client.get("" + sid, callback);
}

RedisAdapter.prototype.Publish = function(channel, data, callback) {
    this.publish.publish(channel, data);
}

RedisAdapter.prototype.__proto__ = events.EventEmitter.prototype;

function RedisFactory(c) {
    console = c;
    return RedisAdapter;
}

module.exports = RedisFactory;
