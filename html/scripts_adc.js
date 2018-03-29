var ws;
var wsReconnect = 10000; // on error reconnect every 10 s
var wsUri = "ws://"+window.location.host+"/ws/iant";
var chart;

function runOnLoad() {
chart = new Highcharts.Chart('iant_adc', {
				tooltip: {
				    style: {
					display: "none",
				    }
				},
				chart: {
				    animation: false
				},
				title: {
					text: 'i_ANT ADC voltage'
				},
				xAxis: {
					labels: {
						style: {
							fontSize: '25px'
						}
					}
				},
				yAxis: {
					title: {
						text: 'Value'
					},
					labels: {
						style: {
							fontSize: '25px'
						}
					}
				},
				plotOptions: {
				        series: {
				            animation: false
				        }
				},
				series: [{
					name: 'Step #',
					data: []
				}]
			});
}

function wsOnOpen(evt) {
    console.log('WS CONNECTED');
}

function wsOnClose(evt) {
    console.log('WS DISCONNECTED');
    if (ws.readyState === ws.CLOSED) {
        setTimeout(initWs, wsReconnect); // reconnect if closed after wsReconnect timeout
    }
}

function wsOnError(evt) {
    console.log('ERROR: ' + evt.data);
}

function wsOnMsg(evt) {
    console.log(evt.data);
    obj = JSON.parse(evt.data);
    switch (obj['msg_type']) {
        case 'i':
		//chart.series[0].addPoint(obj['payload']);
		chart.series[0].setData(obj['payload']);
            break;
	case 'j':
		document.getElementById('smax').value = obj['payload'][0];
		document.getElementById('slast').value = obj['payload'][1];
		document.getElementById('vmax').value = obj['payload'][2];
		document.getElementById('vlast').value = obj['payload'][2] - obj['payload'][3];
	    break;
	case 'b':
		document.getElementById('corr').value = obj['payload'];
	    break;
    }
}

function initWs() {
    ws = new WebSocket(wsUri);
    ws.onopen = function(evt) { wsOnOpen(evt) };
    ws.onclose = function(evt) { wsOnClose(evt) };
    ws.onmessage = function(evt) { wsOnMsg(evt) };
    ws.onerror = function(evt) { wsOnError(evt) };
}

function wsSend(message) {
    ws.send(message);
    console.log("SENT: " + message);
}

function requestIant() {
    var obj = {};

    obj['msg_type'] = 'I';
    wsSend(JSON.stringify(obj));
}

function incCor() {
    var obj = {};

    obj['msg_type'] = 'B';
    obj['payload'] = Number(1);
    wsSend(JSON.stringify(obj));
}

function decCor() {
    var obj = {};

    obj['msg_type'] = 'B';
    obj['payload'] = Number(-1);
    wsSend(JSON.stringify(obj));
}

window.onload = function() {
	runOnLoad();
	initWs();
};

