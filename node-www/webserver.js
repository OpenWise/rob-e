const express = require('express');

var app = express();
app.use(express.static(__dirname + '/www'));

var server = app.listen(6789, function() {
    console.log('Listening on port %d', server.address().port);
});

process.on('uncaughtException', function(err) {
    console.log(err);
});
