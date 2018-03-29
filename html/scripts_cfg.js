var HttpClient = function() {
    this.get = function(aUrl, aCallback) {
        var anHttpRequest = new XMLHttpRequest();
        anHttpRequest.onreadystatechange = function() { 
            if (anHttpRequest.readyState == 4 && anHttpRequest.status == 200)
                aCallback(anHttpRequest.responseText);
        }

        anHttpRequest.open( "GET", aUrl, true );            
        anHttpRequest.send( null );
    }
    this.post = function(aUrl, postdata, aCallback) {
        var anHttpRequest = new XMLHttpRequest();
        anHttpRequest.onreadystatechange = function() { 
            if (anHttpRequest.readyState == 4 && anHttpRequest.status == 200)
                aCallback(anHttpRequest.responseText);
        }

        anHttpRequest.open( "POST", aUrl, true );            
        anHttpRequest.send( postdata );
    }
}

function validateInt(value) {
    if (isNaN(value))  
        return false;
    return ~~value;
};

function validateIntMinMax(value, min, max) {
    if (parseInt(value) < min)
        return min;
    if (isNaN(value))  
	return false;
    if (parseInt(value) > max) 
        return max; 
    return ~~value;
};

function option_exists(id, value) {
    var length=document.getElementById(id).options.length;

    for ( var i=0; i < length; i++ ) {
        if (document.getElementById(id).options[i].value == value)  {
            return true;
        }
    }
    return false;
}

function populateForms(data) {
    var opt;
    var select;
    var x;
    var clevel, accucap, dodidx;

    document.getElementById('foxid').value = data['foxid'];
    document.getElementById('xfoxid').value = data['foxid'];
    document.getElementById('numfoxes').value = data['numfoxes'];
    document.getElementById('xnumfoxes').value = data['numfoxes'];
    if ( option_exists('seancelen', data['seancelen']) ) {
        document.getElementById('seancelen').value = data['seancelen'];
    } else {
        select = document.getElementById('seancelen');
        opt = document.createElement('option');
        opt.value = data['seancelen'];
        opt.text = data['seancelen'];
        opt.selected = true;
        select.appendChild(opt);
    }
    document.getElementById('xseancelen').value = data['seancelen'];
    document.getElementById('cwspeed').value = data['cwspeed'];
    if ( option_exists('txfreq', data['txfreq']) ) {
        document.getElementById('txfreq').value = data['txfreq'];
    } else {
        select = document.getElementById('txfreq');
        opt = document.createElement('option');
        opt.value = data['txfreq'];
        opt.text = data['txfreq'];
        opt.selected = true;
        select.appendChild(opt);
    }
    accucap = data['accucap'];
    dodidx = data['dodidx']; if ( dodidx < 0 ) { dodidx = 0; }
    document.getElementById('xtxfreq').value = data['txfreq'];
    document.getElementById('tone').value = data['tone'];
    document.getElementById('accucap').value = accucap;
    x = data['hwver'].split('');
    document.getElementById('hwver').value = parseInt(x[0])+'.'+parseInt(x[1]);
    x = data['swver'].split('');
    document.getElementById('swver').value = parseInt(x[0])+'.'+parseInt(x[1]);
    if (accucap <= 0)
    {
        clevel = 0;
    } else
    {
        clevel = (accucap - dodidx) * 100 / accucap;
    }
    x=clevel.toFixed(0) + '%';
    document.getElementById('clevel').value = x;
    document.getElementById('totaltime').value = data['totaltime'];
}

function submitFormBasic(url) {
    var client = new HttpClient();
    var obj = {};

    obj['foxid'] = Number(document.getElementById('foxid').value);
    obj['numfoxes'] = Number(document.getElementById('numfoxes').value);
    obj['seancelen'] = Number(document.getElementById('seancelen').value);
    obj['txfreq'] = Number(document.getElementById('txfreq').value);

    client.post(url, JSON.stringify(obj), function(response) {
        console.log(response);
        obj = JSON.parse(response);
        if ( obj['status'] == 'ok' ) {
            alert("Конфігурацію відправлено.");
            window.location.reload();
        } else {
            alert("Помилка відправлення або невірна комбінація параметрів.");
        }
    });
}

function submitFormXpert(url) {
    var client = new HttpClient();
    var obj = {};

    obj['foxid'] = Number(document.getElementById('xfoxid').value);
    obj['numfoxes'] = Number(document.getElementById('xnumfoxes').value);
    obj['seancelen'] = Number(document.getElementById('xseancelen').value);
    obj['cwspeed'] = Number(document.getElementById('cwspeed').value);
    obj['txfreq'] = Number(document.getElementById('xtxfreq').value);
    obj['tone'] = Number(document.getElementById('tone').value);
    obj['accucap'] = Number(document.getElementById('accucap').value);

    client.post(url, JSON.stringify(obj), function(response) {
        console.log(response);
        obj = JSON.parse(response);
        if ( obj['status'] == 'ok' ) {
            alert("Конфігурацію відправлено.");
            window.location.reload();
        } else {
            alert("Помилка відправлення або невірна комбінація параметрів.");
        }
    });
}

window.onload = function() {
    var client = new HttpClient();
    var url = '/fox/cfg_get?' + (new Date()).getTime();
    var obj = {};

    client.get(url, function(response) {
        console.log(response);
        obj = JSON.parse(response);
        populateForms(obj);
    });
}
