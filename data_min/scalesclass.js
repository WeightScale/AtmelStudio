function ScalesSocket(h, p, fm, fe)  {
    var host = h;
    var protocol = p;
    var timerWeight;
    var ws;
    var startWeightTimeout = function()  {
        clearTimeout(timerWeight);
        timerWeight = setTimeout(function () {
            fe();
        },5000);
    };
    this.getWeight = function () {
        ws.send("/wt");
        startWeightTimeout();
    };

    this.openSocket = function () {
        ws = new WebSocket(host, protocol);
        ws.onopen = this.getWeight;
        ws.onclose = function(e){
            clearTimeout(timerWeight);
            starSocketTimeout();
        };
        ws.onerror = function(e){
            fe();
        };
        ws.onmessage = function (e) {
            //var data = e.data;
            var json = JSON.parse(e.data);
            data.w = json.w;
            //data.c = json.c+'%';
            fm(json);
        }
    };

    var starSocketTimeout = function () {
        clearTimeout(timerSocket);
        timerSocket = setTimeout(function () {
            this.prototype.openSocket();
        },5000);
    }
}