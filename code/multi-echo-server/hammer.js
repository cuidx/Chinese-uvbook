var net = require('net');

var PHRASE = "hello world";
var write = function(socket) {
    socket.write(PHRASE, 'utf8');
}

for (var i = 0; i < 10000; i++) {
(function() {
    var socket = net.connect(7000, 'localhost', function() {
		console.error("onconnect " + i)
        socket.on('data', function(reply) {
			console.error(reply.toString())
            if (reply.toString().indexOf(PHRASE) != 0)
                console.error("Problem! '" + reply + "'" + "  '" + PHRASE + "'");
            else
                write(socket);
        });
        write(socket);
    });
})();
}
