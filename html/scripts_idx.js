var interval = 250; // ms
var expected = Date.now() + interval;
var ws;
var wsUri = "ws://"+window.location.host+"/ws/upd";
var wsReconnect = 10000; // on error reconnect every 10 s
var fxSym = [ 'B', '1', '2', '3', '4', '5', 'S', 'N', 'FB', 'F1', 'F2', 'F3', 'F4', 'F5' , 'FS', 'FN' ];
var fxBlank = {"0":{"type":-1,"tsync":0},"1":{"type":-1,"tsync":0},"2":{"type":-1,"tsync":0},"3":{"type":-1,"tsync":0},"4":{"type":-1,"tsync":0},"5":{"type":-1,"tsync":0},"6":{"type":-1,"tsync":0},"7":{"type":-1,"tsync":0},"8":{"type":-1,"tsync":0},"9":{"type":-1,"tsync":0},"10":{"type":-1,"tsync":0},"11":{"type":-1,"tsync":0},"12":{"type":-1,"tsync":0},"13":{"type":-1,"tsync":0},"14":{"type":-1,"tsync":0}};

function updateTime() {
   var dt = Date.now() - expected;
   var currentTime = new Date();
   var hours = currentTime.getHours();
   var minutes = currentTime.getMinutes();
   var seconds = currentTime.getSeconds();

   if (minutes < 10) {
      minutes = "0" + minutes;
   }
   if (seconds < 10){
      seconds = "0" + seconds;
   }
   var v = hours + ":" + minutes + ":" + seconds + " ";
   expected += interval;
   setTimeout(updateTime, Math.max(0, interval - dt)); // timeout in ms
   if (document.getElementById('time_span'))
      document.getElementById('time_span').innerHTML=v;
}


function populateForm() {
   var rTm = new Date();
   var tmp;

   tmp = Math.floor(rTm.getTime()/300000);
   tmp *= 300000;
   rTm.setTime(tmp + 3600000);
   document.getElementById('start_dd').value = rTm.getDate();
   document.getElementById('start_hh').value = rTm.getHours();
   document.getElementById('start_mm').value = rTm.getMinutes();
}

function populateFxDiv(data) {
    var accucap, dodidx, cl, ch, x;
    var numfoxes;

    obj = data;
    for (var key in obj) {
        if (obj.hasOwnProperty(key)) {
            obj2 = obj[key];
            accucap = obj2['accucap'];
            dodidx = obj2['dodidx'];
            numfoxes = obj2['numfoxes'];

            if ( numfoxes == 0 ) numfoxes = 1;

            if ( accucap <= 0 || dodidx > accucap ) {
                ch = 0; cl = 0;
                x = 'batXXX.svg';
            } else {
                ch = (accucap - dodidx) / 60.0 * numfoxes; // capacity hours
                /*
                    STM8L ARDF FC (fox controller) v5.3 board revision
                    contains a sad bug --- SI5351 does not turn off
                    and draws about 13.5 mA in pauses.
                    So we must lower ch (the number of capacity hours, estimated value).
                    Correction does not apply to beacon mode (numfoxes == 1) because
                    the capacity is being estimated in becaon mode.
                */
                if ( numfoxes > 1 ) {
                    ch *= 0.7; 
                }
                cl = (accucap - dodidx) * 100 / accucap; // capacity level in percent
                    x= 'bat000.svg';
                    if ( cl >= 5 )  { x= 'bat005.svg'; }
                    if ( cl >= 10 ) { x= 'bat010.svg'; }
                    if ( cl >= 20 ) { x= 'bat020.svg'; }
                    if ( cl >= 40 ) { x= 'bat040.svg'; }
                    if ( cl >= 50 ) { x= 'bat050.svg'; }
                    if ( cl >= 60 ) { x= 'bat060.svg'; }
                    if ( cl >= 80 ) { x= 'bat080.svg'; }
                    if ( cl > 90 )  { x= 'bat100.svg'; }
            }

            switch ( obj2['type'] ) {
                case 1: // master
                    v='<a target="_blank" href="config.html" class="lnk">';
                    v+='<div class="sgcontent" style="background-color:green">';
                    v+='<table class="sqtab"><tr><td class="cell1">';
                    v+=fxSym[key];
                    v+='</td><td class="cell3" rowspan="2"><img class="bat" src="img/';
                    v+=x;
                    v+='"></td></tr><tr><td class="cell2"><span style="">';
                    v+=ch.toFixed(1);
                    v+=' h</span></td></tr></table></div></a>\n';
                    break;
                case 0: // slave
                    v='<a target="_blank" href="http://'+obj2['addr'];
                    v+='/config.html" class="lnk"><div class="sgcontent" style="background-color:';
                    ( obj2['tsync'] == 1 ) ? v+='#8888FF">' : v+='#FF8833">';
                    v+='<table class="sqtab"><tr><td class="cell1">';
                    v+=fxSym[key];
                    v+='</td><td class="cell3" rowspan="2"><img class="bat" src="img/';
                    v+=x;
                    v+='"></td></tr><tr><td class="cell2"><span style="">';
                    v+=ch.toFixed(1);
                    v+=' h</span></td></tr></table></div></a>\n';
                    break;
                default: // inactive
                    v='<div class="sgcontent"><table class="sqtab"><tr><td class="cell0">';
                    v+=fxSym[key]+'</td></tr></table></div>\n';
            }
            if (document.getElementById('FOX_'+key))
                document.getElementById('FOX_'+key).innerHTML=v;
        }
    }
}

function submitForm() {
   console.log("Form submission:");
   var obj = {};
   var obj2 = {};
   var cur_ss = new Date().getSeconds();

   // Delay before beginning of a new second.
   if (cur_ss == 59) {
      while(new Date().getSeconds() != 0) {}
   }
   else {
      while(new Date().getSeconds() < cur_ss + 1) {}
   }

   cd = new Date();
   obj['cur_yy'] = cd.getYear() - 100;
   obj['cur_mn'] = cd.getMonth() + 1;
   obj['cur_dd'] = cd.getDate();
   obj['cur_hh'] = cd.getHours();
   obj['cur_mm'] = cd.getMinutes();
   obj['cur_ss'] = cd.getSeconds();
   obj['start_dd'] = Number(document.getElementById('start_dd').value);
   obj['start_hh'] = Number(document.getElementById('start_hh').value);
   obj['start_mm'] = Number(document.getElementById('start_mm').value);
   obj['seancelen'] = Number(document.getElementById('seancelen').value);
   obj2['msg_type'] = 'T';
   obj2['payload'] = obj;
   wsSend(JSON.stringify(obj2));
   console.log(obj2);
}

function wsOnOpen(evt) {
    console.log('WS CONNECTED');
}

function wsOnClose(evt) {
    console.log('WS DISCONNECTED');
    populateFxDiv(fxBlank);
    if (ws.readyState === ws.CLOSED) {
        setTimeout(initWs, wsReconnect); // reconnect if closed after wsReconnect timeout
    }
}

function wsOnMsg(evt) {
    console.log(evt.data);
    obj = JSON.parse(evt.data);
    switch (obj['msg_type']) {
        case 'U':
            populateFxDiv(obj['payload']);
            break;
        case 't':
            if (obj['payload'] == "1") {
                alert("Час старту відправлено.");
            } else {
                alert("Невірна комбінація параметрів.");
            }
            break;
    }
}

function wsOnError(evt) {
    console.log('ERROR: ' + evt.data);
    populateFxDiv(fxBlank);
}

function wsSend(message) {
    ws.send(message);
    console.log("SENT: " + message);
}

function initWs() {
    ws = new WebSocket(wsUri);
    ws.onopen = function(evt) { wsOnOpen(evt) };
    ws.onclose = function(evt) { wsOnClose(evt) };
    ws.onmessage = function(evt) { wsOnMsg(evt) };
    ws.onerror = function(evt) { wsOnError(evt) };
}

window.onload = function() {
    populateForm();
    updateTime();
    initWs();
}
