var weight;
var w;
var d = document;

function ScalesSocket(h, p, fm, fe) {
    let tw;
    let ts;
    let ws;
    let startWeightTimeout = function() {
        clearTimeout(tw);
        tw = setTimeout(function() {
            fe();
        }, 5000);
    };
    this.getWeight = function() {
        ws.send("{'cmd':'wt'}");
        startWeightTimeout();
    };
    this.openSocket = function() {
        ws = new WebSocket(h, p);
        ws.onopen = this.getWeight;
        ws.onclose = function() {
            clearTimeout(tw);
            starSocketTimeout();
            fe();
        };
        ws.onerror = function() {
            fe();
        };
        ws.onmessage = function(e) {
            fm(JSON.parse(e.data));
        }
    };
    var starSocketTimeout = function() {
        clearTimeout(ts);
        ts = setTimeout(function() {
            this.prototype.openSocket();
        }, 5000);
    }
}

function go() {
    d.getElementById('wt_id').innerHTML = '---';
}

function setMax() {
    var f = new FormData(d.getElementById('form_c_id'));
    f.append('update', true);
    var r = new XMLHttpRequest();
    r.onreadystatechange = function() {
        if (r.readyState === 4) {
            if (r.responseText !== null) {
                if (d.getElementById("form_zero") === null) {
                    d.getElementById("id_bs").value = 'ОБНОВИТЬ';
                    var f = d.createElement('fieldset');
                    f.id = 'form_zero';
                    f.innerHTML = "<legend>Нулевой вес</legend><form action='javascript:setZero()'><p>Перед установкой убедитесь что весы не нагружены.</p><input type='submit' value='УСТАНОВИТЬ НОЛЬ'/></form><br><div id='wt_id'>---</div>";
                    d.body.appendChild(f);
                    setupWeight();
                }
            }
        }
    };
    r.open('POST', 'calibr.html', true);
    r.send(f);
}

function setZero() {
    let r = new XMLHttpRequest();
    let f = new FormData();
    f.append('zero', true);
    f.append('weightCal', '0');
    r.onreadystatechange = function() {
        if (r.readyState === 4 && r.status === 200) {
            if (r.responseText !== null) {
                d.getElementById('form_zero').disabled = true;
                var f = d.createElement('fieldset');
                f.id = 'form_weight';
                f.innerHTML = "<legend>Калиброваный вес</legend><form action='javascript:setWeight()'><p>Перед установкой весы нагружаются контрольным весом. Дать некоторое время для стабилизации.Значение вводится с точностью которое выбрано в пункте Точность измерения.</p><table><tr><td>ГИРЯ:</td><td><input id='gr_id' value='0' type='number' step='any' required placeholder='Калиброваная гиря'/></td></tr><tr><td>ГРУЗ:</td><td><input id='id_cal_wt' value='0' type='number' step='any' title='Введите значение веса установленого на весах' max='100000' required placeholder='Калиброваный вес'/></td></tr><tr><td>ОШИБКА:</td><td><div id='dif_gr_id'></div></td></tr><tr><td><input type='submit' value='УСТАНОВИТЬ'/></td><td><a href='javascript:calculate();'>подобрать</a></td></tr></table></form>";
                d.body.appendChild(f);
            }
        }

    };
    r.open('POST', 'calibr.html', true);
    r.send(f);
}

function setWeight() {
    var fd = new FormData();
    var w = parseFloat(d.getElementById('id_cal_wt').value) + parseFloat(d.getElementById('gr_id').value);
    fd.append('weightCal', w.toString());
    var r = new XMLHttpRequest();
    r.onreadystatechange = function() {
        if (r.readyState === 4) {
            if (r.status === 200) {
                if (r.responseText !== null) {
                    if (d.getElementById('form_seal') === null) {
                        var f = d.createElement('fieldset');
                        f.id = 'form_seal';
                        f.innerHTML = "<legend>Пломбировка</legend><form action='javascript:setSeal()'><left><p>Сохранение процесса калибровки. Данные калибровки сохраняются в память весов.</p><input type='submit' value='ОПЛОМБИРОВАТЬ'/></left></form>";
                        d.body.appendChild(f);
                    }
                }
            } else if (r.status === 400) {
                alert(r.responseText);
            }
        }
    };
    r.open('POST', 'calibr.html', true);
    r.send(fd);
}

function setSeal() {
    let r = new XMLHttpRequest();
    r.onreadystatechange = function() {
        if (r.readyState === 4 && this.status === 200) {
            alert('Номер пломбы: ' + this.responseText);
            window.location.replace('/');
        }
    };
    r.open('GET', '/sl', true);
    r.send(null);
}

function GetSettings() {
    let r = new XMLHttpRequest();
    r.overrideMimeType('application/json');
    r.onreadystatechange = function() {
        if (r.readyState === 4 && r.status === 200) {
            try {
                let j = JSON.parse(r.responseText.replace(/NaN/g,'null'));
                for (let e in j) {
                    try {
                        if (d.getElementById(e) !== null) d.getElementById(e).value = j[e];
                    } catch (e) {}
                }
            } catch (e) {
                alert(e.toString());
            }
            d.body.style.visibility = 'visible';
        }
    };
    r.open('GET', '/cdate.json', true);
    r.send(null);
}

function saveValue() {
    var f = new FormData(d.getElementById('form_c_id'));
    f.append('update', true);
    var r = new XMLHttpRequest();
    r.onreadystatechange = function() {
        if (r.readyState === 4) {
            if (r.status === 200)
                window.open('/', '_self');
            else if(r.status === 400){
                if(confirm('Настройки не изменились. Выйти?')){
                    window.open('/', '_self');
                }
            }
        }
    };
    r.onerror = function() {
        alert('Ошибка');
    };
    r.open('POST', 'calibr.html', true);
    r.send(f);
}
function getWeight() {
    w = new ScalesSocket('ws://' + d.location.host + '/ws', ['arduino'], handleWeight, function() {
        go();
        w.openSocket();
    });
    w.openSocket();
}

function calculate() {
    let dif = weight - parseFloat(d.getElementById('id_cal_wt').value);
    dif = parseFloat(d.getElementById('gr_id').value) / dif;
    dif = parseFloat(d.getElementById('id_cal_wt').value) * dif;
    d.getElementById('id_cal_wt').value = dif.toFixed(d.getElementById('ac_id').value);
    setWeight();
}

function handleWeight(c) {
    if (c.hasOwnProperty('cmd')) {
        switch (c.cmd) {
            case 'wt':
                weight = c.w;
                d.getElementById('wt_id').innerHTML = weight;
                try {
                    d.getElementById('form_seal').disabled = (c.s === false);
                } catch (e) {}
                if (c.s) {
                    d.getElementById('wt_id').setAttribute('style', 'background: #fff;');
                } else {
                    d.getElementById('wt_id').setAttribute('style', 'background: #C4C4C4;');
                }
                try {
                    var dif_gr = parseFloat(d.getElementById('id_cal_wt').value) + parseFloat(d.getElementById('gr_id').value);
                    dif_gr -= weight;
                    d.getElementById('dif_gr_id').innerHTML = dif_gr.toFixed(d.getElementById('ac_id').value);
                } catch (e) {}
                w.getWeight();
                break;
        }
    }
}

function setupWeight() {
    getWeight();
}