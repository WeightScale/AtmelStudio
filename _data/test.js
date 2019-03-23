var utcSeconds, timezone, ajaxobj, haspages, logdata, esprfidcontent, version = "0.8.1",
    websock = null,
    wsUri = "ws://" + window.location.hostname + "/ws",
    data = [],
    isOfficialBoard = !1,
    config = {
        command: "configfile",
        network: {
            bssid: "",
            ssid: "",
            wmode: 1,
            hide: 0,
            pswd: "",
            offtime: 0,
            dhcp: 1,
            ip: "",
            subnet: "",
            gateway: "",
            dns: ""
        },
        hardware: {
            readerType: 1,
            wgd0pin: 4,
            wgd1pin: 5,
            sspin: 0,
            rfidgain: 32,
            rtype: 1,
            rpin: 4,
            rtime: 400
        },
        general: {
            hostnm: "esp-rfid",
            restart: 0,
            pswd: "admin"
        },
        mqtt: {
            enabled: 0,
            host: "",
            port: 1883,
            topic: "",
            user: "",
            pswd: ""
        },
        ntp: {
            server: "pool.ntp.org",
            interval: 30,
            timezone: 0
        }
    },
    page = 1,
    recordstorestore = 0,
    slot = 0,
    completed = !1,
    file = {},
    backupstarted = !1,
    restorestarted = !1;

function browserTime() {
    var e = new Date(0),
        t = new Date,
        n = Math.floor(t.getTime() / 1e3 + 60 * t.getTimezoneOffset() * -1);
    e.setUTCSeconds(n), document.getElementById("rtc").innerHTML = e.toUTCString().slice(0, -3)
}

function deviceTime() {
    var e = new Date(0),
        t = Math.floor(utcSeconds + 60 * e.getTimezoneOffset() * -1);
    e.setUTCSeconds(t), document.getElementById("utc").innerHTML = e.toUTCString().slice(0, -3)
}

function syncBrowserTime() {
    var e = new Date,
        t = Math.floor(e.getTime() / 1e3),
        n = {
            command: "settime"
        };
    n.epoch = t, websock.send(JSON.stringify(n)), $("#ntp").click()
}

function handleReader() {
    0 === parseInt(document.getElementById("readerType").value) ? (document.getElementById("wiegandForm").style.display = "none", document.getElementById("mfrc522Form").style.display = "block", document.getElementById("rc522gain").style.display = "block") : 1 === parseInt(document.getElementById("readerType").value) ? (document.getElementById("wiegandForm").style.display = "block", document.getElementById("mfrc522Form").style.display = "none") : 2 === parseInt(document.getElementById("readerType").value) && (document.getElementById("wiegandForm").style.display = "none", document.getElementById("mfrc522Form").style.display = "block", document.getElementById("rc522gain").style.display = "none")
}

function handleDHCP() {
    "1" === document.querySelector('input[name="dhcpenabled"]:checked').value ? $("#staticip").slideUp() : ($("#staticip").slideDown(), $("#staticip").show())
}

function listhardware() {
    isOfficialBoard ? (document.getElementById("readerType").value = 1, document.getElementById("wg0pin").value = 5, document.getElementById("wg1pin").value = 4, document.getElementById("gpiorly").value = 13, document.getElementById("wg0pin").disabled = !0, document.getElementById("wg1pin").disabled = !0, document.getElementById("gpiorly").disabled = !0, document.getElementById("readerType").disabled = !0, document.getElementById("typerly").value = config.hardware.rtype, document.getElementById("delay").value = config.hardware.rtime) : (document.getElementById("readerType").value = config.hardware.readerType, document.getElementById("wg0pin").value = config.hardware.wgd0pin, document.getElementById("wg1pin").value = config.hardware.wgd1pin, document.getElementById("gpioss").value = config.hardware.sspin, document.getElementById("gain").value = config.hardware.rfidgain, document.getElementById("typerly").value = config.hardware.rtype, document.getElementById("gpiorly").value = config.hardware.rpin, document.getElementById("delay").value = config.hardware.rtime), handleReader()
}

function listlog() {
    websock.send('{"command":"getlatestlog", "page":' + page + "}")
}

function listntp() {
    websock.send('{"command":"gettime"}'), document.getElementById("ntpserver").value = config.ntp.server, document.getElementById("intervals").value = config.ntp.interval, document.getElementById("DropDownTimezone").value = config.ntp.timezone, browserTime(), deviceTime()
}

function revcommit() {
    document.getElementById("jsonholder").innerText = JSON.stringify(config, null, 2), $("#revcommit").modal("show")
}

function uncommited() {
    $("#commit").fadeOut(200, function() {
        $(this).css("background", "gold").fadeIn(1e3)
    }), document.getElementById("commit").innerHTML = "<h6>You have uncommited changes, please click here to review and commit.</h6>", $("#commit").click(function() {
        return revcommit(), !1
    })
}

function savehardware() {
    config.hardware.readerType = parseInt(document.getElementById("readerType").value), config.hardware.wgd0pin = parseInt(document.getElementById("wg0pin").value), config.hardware.wgd1pin = parseInt(document.getElementById("wg1pin").value), config.hardware.sspin = parseInt(document.getElementById("gpioss").value), config.hardware.rfidgain = parseInt(document.getElementById("gain").value), config.hardware.rtype = parseInt(document.getElementById("typerly").value), config.hardware.rpin = parseInt(document.getElementById("gpiorly").value), config.hardware.rtime = parseInt(document.getElementById("delay").value), uncommited()
}

function saventp() {
    config.ntp.server = document.getElementById("ntpserver").value, config.ntp.interval = parseInt(document.getElementById("intervals").value), config.ntp.timezone = parseInt(document.getElementById("DropDownTimezone").value), uncommited()
}

function savegeneral() {
    var e = document.getElementById("adminpwd").value;
    null !== e && "" !== e ? (config.general.pswd = e, config.general.hostnm = document.getElementById("hostname").value, config.general.restart = parseInt(document.getElementById("autorestart").value), uncommited()) : alert("Administrator Password cannot be empty")
}

function savemqtt() {
    config.mqtt.enabled = 0, 1 === parseInt($('input[name="mqttenabled"]:checked').val()) && (config.mqtt.enabled = 1), config.mqtt.host = document.getElementById("mqtthost").value, config.mqtt.port = parseInt(document.getElementById("mqttport").value), config.mqtt.topic = document.getElementById("mqtttopic").value, config.mqtt.user = document.getElementById("mqttuser").value, config.mqtt.pswd = document.getElementById("mqttpwd").value, uncommited()
}

function checkOctects(e) {
    var t = document.getElementById(e);
    return !!t.value.match(/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/) || (alert("You have entered an invalid address on " + e), t.focus(), !1)
}

function savenetwork() {
    var e = 0;
    if (config.network.dhcp = 0, config.network.hide = 0, "none" === document.getElementById("inputtohide").style.display) {
        var t = document.getElementById("ssid");
        config.network.ssid = t.options[t.selectedIndex].value
    } else config.network.ssid = document.getElementById("inputtohide").value;
    if (document.getElementById("wmodeap").checked) e = 1, config.network.bssid = 0, 1 === parseInt(document.querySelector('input[name="hideapenable"]:checked').value) ? config.network.hide = 1 : config.network.hide = 0;
    else if (config.network.bssid = document.getElementById("wifibssid").value, 1 === parseInt(document.querySelector('input[name="dhcpenabled"]:checked').value)) config.network.dhcp = 1;
    else {
        if (config.network.dhcp = 0, !checkOctects("ipaddress")) return;
        if (!checkOctects("subnet")) return;
        if (!checkOctects("dnsadd")) return;
        if (!checkOctects("gateway")) return;
        config.network.ip = document.getElementById("ipaddress").value, config.network.dns = document.getElementById("dnsadd").value, config.network.subnet = document.getElementById("subnet").value, config.network.gateway = document.getElementById("gateway").value
    }
    config.network.wmode = e, config.network.pswd = document.getElementById("wifipass").value, config.network.offtime = parseInt(document.getElementById("disable_wifi_after_seconds").value), uncommited()
}
var formData = new FormData;

function inProgress(e) {
    $("body").load("esprfid.htm #progresscontent", function(t, n, a) {
        if ("success" === n) {
            $(".progress").css("height", "40"), $(".progress").css("font-size", "xx-large");
            var o = 0,
                s = setInterval(function() {
                    if ($(".progress-bar").css("width", o + "%").attr("aria-valuenow", o).html(o + "%"), 101 === ++o) {
                        clearInterval(s);
                        var e = document.createElement("a");
                        e.href = "http://" + config.general.hostnm + ".local", e.innerText = "Try to reconnect ESP", document.getElementById("reconnect").appendChild(e), document.getElementById("reconnect").style.display = "block", document.getElementById("updateprog").className = "progress-bar progress-bar-success", document.getElementById("updateprog").innerHTML = "Completed"
                    }
                }, 500);
            switch (e) {
                case "upload":
                    $.ajax({
                        url: "/update",
                        type: "POST",
                        data: formData,
                        processData: !1,
                        contentType: !1
                    });
                    break;
                case "commit":
                    websock.send(JSON.stringify(config));
                    break;
                case "destroy":
                    websock.send('{"command":"destroy"}');
                    break;
                case "restart":
                    websock.send('{"command":"restart"}')
            }
        }
    }).hide().fadeIn()
}

function commit() {
    inProgress("commit")
}

function handleAP() {
    document.getElementById("hideap").style.display = "block", document.getElementById("hideBSSID").style.display = "none", document.getElementById("scanb").style.display = "none", document.getElementById("ssid").style.display = "none", document.getElementById("dhcp").style.display = "none", document.getElementById("staticip").style.display = "none", document.getElementById("inputtohide").style.display = "block"
}

function handleSTA() {
    document.getElementById("hideap").style.display = "none", document.getElementById("hideBSSID").style.display = "block", document.getElementById("scanb").style.display = "block", document.getElementById("dhcp").style.display = "block"
}

function listnetwork() {
    document.getElementById("inputtohide").value = config.network.ssid, document.getElementById("wifipass").value = config.network.pswd, 1 === config.network.wmode ? (document.getElementById("wmodeap").checked = !0, 1 === config.network.hide && $('input[name="hideapenable"][value="1"]').prop("checked", !0), handleAP()) : (document.getElementById("wmodesta").checked = !0, document.getElementById("wifibssid").value = config.network.bssid, 0 === config.network.dhcp && ($('input[name="dhcpenabled"][value="0"]').prop("checked", !0), handleDHCP()), document.getElementById("ipaddress").value = config.network.ip, document.getElementById("subnet").value = config.network.subnet, document.getElementById("dnsadd").value = config.network.dns, document.getElementById("gateway").value = config.network.gateway, handleSTA()), document.getElementById("disable_wifi_after_seconds").value = config.network.offtime
}

function listgeneral() {
    document.getElementById("adminpwd").value = config.general.pswd, document.getElementById("hostname").value = config.general.hostnm, document.getElementById("autorestart").value = config.general.restart
}

function listmqtt() {
    1 === config.mqtt.enabled && $('input[name="mqttenabled"][value="1"]').prop("checked", !0), document.getElementById("mqtthost").value = config.mqtt.host, document.getElementById("mqttport").value = config.mqtt.port, document.getElementById("mqtttopic").value = config.mqtt.topic, document.getElementById("mqttuser").value = config.mqtt.user, document.getElementById("mqttpwd").value = config.mqtt.pswd
}

function listBSSID() {
    var e = document.getElementById("ssid");
    document.getElementById("wifibssid").value = e.options[e.selectedIndex].bssidvalue
}

function listSSID(e) {
    for (var t = document.getElementById("ssid"), n = 0; n < e.list.length; n++) {
        var a = parseInt(e.list[n].rssi),
            o = Math.min(Math.max(2 * (a + 100), 0), 100),
            s = document.createElement("option");
        s.value = e.list[n].ssid, s.bssidvalue = e.list[n].bssid, s.innerHTML = "BSSID: " + e.list[n].bssid + ", Signal Strength: %" + o + ", Network: " + e.list[n].ssid, t.appendChild(s)
    }
    document.getElementById("scanb").innerHTML = "Re-Scan", listBSSID()
}

function scanWifi() {
    websock.send('{"command":"scan"}'), document.getElementById("scanb").innerHTML = "...", document.getElementById("inputtohide").style.display = "none";
    var e = document.getElementById("ssid");
    for (e.style.display = "inline"; e.hasChildNodes();) e.removeChild(e.lastChild)
}

function getUsers() {
    websock.send('{"command":"userlist", "page":' + page + "}")
}

function getEvents() {
    websock.send('{"command":"geteventlog", "page":' + page + "}")
}

function listSCAN(e) {
    1 === e.known ? ($(".fooicon-remove").click(), document.querySelector("input.form-control[type=text]").value = e.uid, $(".fooicon-search").click()) : ($(".footable-add").click(), document.getElementById("uid").value = e.uid, document.getElementById("picctype").value = e.type, document.getElementById("username").value = e.user, document.getElementById("acctype").value = e.acctype)
}

function getnextpage(e) {
    if (backupstarted || (document.getElementById("loadpages").innerHTML = "Loading " + page + "/" + haspages), page < haspages) {
        page += 1;
        var t = {};
        t.command = e, t.page = page, websock.send(JSON.stringify(t))
    }
}

function builddata(e) {
    data = data.concat(e.list)
}

function testRelay() {
    websock.send('{"command":"testrelay"}')
}

function colorStatusbar(e) {
    var t = e.style.width.slice(0, -1);
    t > 50 ? e.className = "progress-bar progress-bar-success" : t > 25 ? e.className = "progress-bar progress-bar-warning" : e.class = "progress-bar progress-bar-danger"
}

function listStats() {
    document.getElementById("chip").innerHTML = ajaxobj.chipid, document.getElementById("cpu").innerHTML = ajaxobj.cpu + " Mhz", document.getElementById("uptime").innerHTML = ajaxobj.uptime, document.getElementById("heap").innerHTML = ajaxobj.heap + " Bytes", document.getElementById("heap").style.width = 100 * ajaxobj.heap / 40960 + "%", colorStatusbar(document.getElementById("heap")), document.getElementById("flash").innerHTML = ajaxobj.availsize + " Bytes", document.getElementById("flash").style.width = 100 * ajaxobj.availsize / 1044464 + "%", colorStatusbar(document.getElementById("flash")), document.getElementById("spiffs").innerHTML = ajaxobj.availspiffs + " Bytes", document.getElementById("spiffs").style.width = 100 * ajaxobj.availspiffs / ajaxobj.spiffssize + "%", colorStatusbar(document.getElementById("spiffs")), document.getElementById("ssidstat").innerHTML = ajaxobj.ssid, document.getElementById("ip").innerHTML = ajaxobj.ip, document.getElementById("gate").innerHTML = ajaxobj.gateway, document.getElementById("mask").innerHTML = ajaxobj.netmask, document.getElementById("dns").innerHTML = ajaxobj.dns, document.getElementById("mac").innerHTML = ajaxobj.mac, document.getElementById("sver").innerText = version, $("#mainver").text(version)
}

function getContent(e) {
    $("#dismiss").click(), $(".overlay").fadeOut().promise().done(function() {
        var t = $(e).html();
        $("#ajaxcontent").html(t).promise().done(function() {
            switch (e) {
                case "#statuscontent":
                    listStats();
                    break;
                case "#backupcontent":
                    break;
                case "#ntpcontent":
                    listntp();
                    break;
                case "#mqttcontent":
                    listmqtt();
                    break;
                case "#generalcontent":
                    listgeneral();
                    break;
                case "#hardwarecontent":
                    listhardware();
                    break;
                case "#networkcontent":
                    listnetwork();
                    break;
                case "#logcontent":
                    page = 1, data = [], listlog();
                    break;
                case "#userscontent":
                    page = 1, data = [], getUsers();
                    break;
                case "#eventcontent":
                    page = 1, data = [], getEvents()
            }
            $('[data-toggle="popover"]').popover({
                container: "body"
            }), $(this).hide().fadeIn()
        })
    })
}

function backupuser() {
    backupstarted = !0, data = [];
    var e = {
        command: "userlist"
    };
    e.page = page, websock.send(JSON.stringify(e))
}

function backupset() {
    var e = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(config, null, 2)),
        t = document.getElementById("downloadSet");
    t.setAttribute("href", e), t.setAttribute("download", "esp-rfid-settings.json"), t.click()
}

function piccBackup(e) {
    var t = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(e, null, 2)),
        n = document.getElementById("downloadUser");
    n.setAttribute("href", t), n.setAttribute("download", "esp-rfid-users.json"), n.click(), backupstarted = !1
}

function restoreSet() {
    var e = document.getElementById("restoreSet"),
        t = new FileReader;
    "files" in e && (0 === e.files.length ? alert("You did not select file to restore!") : (t.onload = function() {
        var e;
        try {
            e = JSON.parse(t.result)
        } catch (e) {
            return void alert("Not a valid backup file!")
        }
        "configfile" === e.command && (confirm("File seems to be valid, do you wish to continue?") && (config = e, uncommited()))
    }, t.readAsText(e.files[0])))
}

function restore1by1(e, t, n) {
    var a, o, s, i, d = 100 / t;
    document.getElementById("dynamic").style.width = d * (e + 1) + "%";
    var c = {};
    a = n[e].uid, o = n[e].username, s = n[e].acctype, i = n[e].validuntil, c.command = "userfile", c.uid = a, c.user = o, c.acctype = s, c.validuntil = i, websock.send(JSON.stringify(c)), ++slot === t && (document.getElementById("dynamic").className = "progress-bar progress-bar-success", document.getElementById("dynamic").innerHTML = "Completed", document.getElementById("dynamic").style.width = "100%", restorestarted = !1, completed = !0, document.getElementById("restoreclose").style.display = "block")
}

function restoreUser() {
    var e = document.getElementById("restoreUser"),
        t = new FileReader;
    "files" in e && (0 === e.files.length ? alert("You did not select any file to restore") : (t.onload = function() {
        var e;
        try {
            e = JSON.parse(t.result)
        } catch (e) {
            return void alert("Not a valid backup file")
        }
        "esp-rfid-userbackup" === e.type && (confirm("File seems to be valid, do you wish to continue?") && (recordstorestore = e.list.length, data = e.list, restorestarted = !0, $("#restoremodal").modal({
            backdrop: "static",
            keyboard: !1
        }), restore1by1(slot, recordstorestore, data)))
    }, t.readAsText(e.files[0])))
}

function twoDigits(e) {
    return e < 10 ? "0" + e : e
}

function initEventTable() {
    for (var e = [], t = 0; t < data.length; t++) {
        var n = JSON.parse(data[t]);
        switch (n.uid = t, e[t] = {}, e[t].options = {}, e[t].value = {}, e[t].value = n, n.type) {
            case "WARN":
                e[t].options.classes = "warning";
                break;
            case "INFO":
                e[t].options.classes = "info";
                break;
            case "ERRO":
                e[t].options.classes = "danger"
        }
    }
    jQuery(function(t) {
        window.FooTable.init("#eventtable", {
            columns: [{
                name: "uid",
                title: "ID",
                type: "text",
                sorted: !0,
                direction: "DESC"
            }, {
                name: "type",
                title: "Event Type",
                type: "text"
            }, {
                name: "src",
                title: "Source"
            }, {
                name: "desc",
                title: "Description"
            }, {
                name: "data",
                title: "Additional Data",
                breakpoints: "xs sm"
            }, {
                name: "time",
                title: "Date",
                parser: function(e) {
                    if (e < 1520665101) return e;
                    var t = new Date;
                    e = Math.floor(e + 60 * t.getTimezoneOffset() * -1);
                    var n = new Date(1e3 * e);
                    return n.getUTCFullYear() + "-" + twoDigits(n.getUTCMonth() + 1) + "-" + twoDigits(n.getUTCDate()) + "-" + twoDigits(n.getUTCHours()) + ":" + twoDigits(n.getUTCMinutes()) + ":" + twoDigits(n.getUTCSeconds())
                },
                breakpoints: "xs sm"
            }],
            rows: e
        })
    })
}

function initLatestLogTable() {
    for (var e = [], t = 0; t < data.length; t++) {
        var n = JSON.parse(data[t]);
        switch (e[t] = {}, e[t].options = {}, e[t].value = {}, e[t].value = n, n.acctype) {
            case 1:
                e[t].options.classes = "success";
                break;
            case 99:
                e[t].options.classes = "info";
                break;
            case 0:
                e[t].options.classes = "warning";
                break;
            case 98:
                e[t].options.classes = "danger"
        }
    }
    jQuery(function(t) {
        window.FooTable.init("#latestlogtable", {
            columns: [{
                name: "timestamp",
                title: "Date",
                parser: function(e) {
                    var t = new Date;
                    e = Math.floor(e + 60 * t.getTimezoneOffset() * -1);
                    var n = new Date(1e3 * e);
                    return n.getUTCFullYear() + "-" + twoDigits(n.getUTCMonth() + 1) + "-" + twoDigits(n.getUTCDate()) + "-" + twoDigits(n.getUTCHours()) + ":" + twoDigits(n.getUTCMinutes()) + ":" + twoDigits(n.getUTCSeconds())
                },
                sorted: !0,
                direction: "DESC"
            }, {
                name: "uid",
                title: "UID",
                type: "text"
            }, {
                name: "username",
                title: "User Name or Label"
            }, {
                name: "acctype",
                title: "Access",
                breakpoints: "xs sm",
                parser: function(e) {
                    return 1 === e ? "Granted" : 99 === e ? "Admin" : 0 === e ? "Disabled" : 98 === e ? "Unknown" : void 0
                }
            }],
            rows: e
        })
    })
}

function initUserTable() {
    jQuery(function(e) {
        var t = e("#editor-modal"),
            n = e("#editor"),
            a = e("#editor-title"),
            o = window.FooTable.init("#usertable", {
                columns: [{
                    name: "uid",
                    title: "UID",
                    type: "text"
                }, {
                    name: "username",
                    title: "User Name or Label"
                }, {
                    name: "acctype",
                    title: "Access Type",
                    breakpoints: "xs",
                    parser: function(e) {
                        return 1 === e ? "Always" : 99 === e ? "Admin" : 0 === e ? "Disabled" : e
                    }
                }, {
                    name: "validuntil",
                    title: "Valid Until",
                    breakpoints: "xs sm",
                    parser: function(e) {
                        var t = new Date;
                        e = Math.floor(e + 60 * t.getTimezoneOffset() * -1);
                        var n = new Date(1e3 * e);
                        return n.getFullYear() + "-" + twoDigits(n.getMonth() + 1) + "-" + twoDigits(n.getDate())
                    }
                }],
                rows: data,
                editing: {
                    showText: '<span class="fooicon fooicon-pencil" aria-hidden="true"></span> Edit Users',
                    addText: "New User",
                    addRow: function() {
                        n[0].reset(), a.text("Add a new User"), t.modal("show")
                    },
                    editRow: function(e) {
                        var o, s = e.val();
                        "Always" === s.acctype ? o = 1 : "Admin" === s.acctype ? o = 99 : "Disabled" === s.acctype && (o = 0), n.find("#uid").val(s.uid), n.find("#username").val(s.username), n.find("#acctype").val(o), n.find("#validuntil").val(s.validuntil), t.data("row", e), a.text("Edit User # " + s.username), t.modal("show")
                    },
                    deleteRow: function(e) {
                        var t = e.value.uid,
                            n = e.value.username;
                        if (confirm("This will remove " + t + " : " + n + " from database. Are you sure?")) {
                            var a = '{"uid":"' + t + '","command":"remove"}';
                            websock.send(a), e.delete()
                        }
                    }
                },
                components: {
                    filtering: window.FooTable.MyFiltering
                }
            }),
            s = 10001;
        n.on("submit", function(e) {
            if (!this.checkValidity || this.checkValidity()) {
                e.preventDefault();
                var a = t.data("row"),
                    i = {
                        uid: n.find("#uid").val(),
                        username: n.find("#username").val(),
                        acctype: parseInt(n.find("#acctype").val()),
                        validuntil: new Date(n.find("#validuntil").val()).getTime() / 1e3
                    };
                a instanceof window.FooTable.Row ? (a.delete(), i.id = s++, o.rows.add(i)) : (i.id = s++, o.rows.add(i));
                var d = {
                    command: "userfile"
                };
                d.uid = n.find("#uid").val(), d.user = n.find("#username").val(), d.acctype = parseInt(n.find("#acctype").val());
                var c = n.find("#validuntil").val(),
                    r = new Date(c).getTime() / 1e3;
                d.validuntil = r, websock.send(JSON.stringify(d)), t.modal("hide")
            }
        })
    })
}

function restartESP() {
    inProgress("restart")
}
var nextIsNotJson = !1;

function socketMessageListener(e) {
    var t = JSON.parse(e.data);
    if (t.hasOwnProperty("command")) switch (t.command) {
        case "status":
            t.hasOwnProperty("board") && (isOfficialBoard = !0), ajaxobj = t, getContent("#statuscontent");
            break;
        case "userlist":
            if (0 === (haspages = t.haspages)) {
                backupstarted || (document.getElementById("loading-img").style.display = "none", initUserTable(), $(".footable-show").click(), $(".fooicon-remove").click());
                break
            }
            builddata(t);
            break;
        case "eventlist":
            if (0 === (haspages = t.haspages)) {
                document.getElementById("loading-img").style.display = "none", initEventTable();
                break
            }
            builddata(t);
            break;
        case "latestlist":
            if (0 === (haspages = t.haspages)) {
                document.getElementById("loading-img").style.display = "none", initLatestLogTable();
                break
            }
            builddata(t);
            break;
        case "gettime":
            utcSeconds = t.epoch, timezone = t.timezone, deviceTime();
            break;
        case "piccscan":
            listSCAN(t);
            break;
        case "ssidlist":
            listSSID(t);
            break;
        case "configfile":
            config = t
    }
    if (t.hasOwnProperty("resultof")) switch (t.resultof) {
        case "latestlog":
            !1 === t.result && (logdata = [], initLatestLogTable(), document.getElementById("loading-img").style.display = "none");
            break;
        case "userlist":
            if (page < haspages && !0 === t.result) getnextpage("userlist");
            else if (page === haspages) {
                backupstarted ? (file.type = "esp-rfid-userbackup", file.version = "v0.6", file.list = data, piccBackup(file)) : (initUserTable(), document.getElementById("loading-img").style.display = "none", $(".footable-show").click(), $(".fooicon-remove").click());
                break
            }
            break;
        case "eventlist":
            page < haspages && !0 === t.result ? getnextpage("geteventlog") : page === haspages && (initEventTable(), document.getElementById("loading-img").style.display = "none");
            break;
        case "latestlist":
            page < haspages && !0 === t.result ? getnextpage("getlatestlog") : page === haspages && (initLatestLogTable(), document.getElementById("loading-img").style.display = "none");
            break;
        case "userfile":
            restorestarted && (completed || !0 !== t.result || restore1by1(slot, recordstorestore, data))
    }
}

function clearevent() {
    websock.send('{"command":"clearevent"}'), $("#eventlog").click()
}

function clearlatest() {
    websock.send('{"command":"clearlatest"}'), $("#latestlog").click()
}

function compareDestroy() {
    config.general.hostnm === document.getElementById("compare").value ? $("#destroybtn").prop("disabled", !1) : $("#destroybtn").prop("disabled", !0)
}

function destroy() {
    inProgress("destroy")
}
$("#dismiss, .overlay").on("click", function() {
    $("#sidebar").removeClass("active"), $(".overlay").fadeOut()
}), $("#sidebarCollapse").on("click", function() {
    $("#sidebar").addClass("active"), $(".overlay").fadeIn(), $(".collapse.in").toggleClass("in"), $("a[aria-expanded=true]").attr("aria-expanded", "false")
}), $("#status").click(function() {
    return websock.send('{"command":"status"}'), !1
}), $("#network").on("click", function() {
    return getContent("#networkcontent"), !1
}), $("#hardware").click(function() {
    return getContent("#hardwarecontent"), !1
}), $("#general").click(function() {
    return getContent("#generalcontent"), !1
}), $("#mqtt").click(function() {
    return getContent("#mqttcontent"), !1
}), $("#ntp").click(function() {
    return getContent("#ntpcontent"), !1
}), $("#users").click(function() {
    getContent("#userscontent")
}), $("#latestlog").click(function() {
    return getContent("#logcontent"), !1
}), $("#backup").click(function() {
    return getContent("#backupcontent"), !1
}), $("#reset").click(function() {
    return $("#destroy").modal("show"), !1
}), $("#eventlog").click(function() {
    return getContent("#eventcontent"), !1
}), $(".noimp").on("click", function() {
    $("#noimp").modal("show")
}), window.FooTable.MyFiltering = window.FooTable.Filtering.extend({
    construct: function(e) {
        this._super(e), this.acctypes = ["1", "99", "0"], this.acctypesstr = ["Always", "Admin", "Disabled"], this.def = "Access Type", this.$acctype = null
    },
    $create: function() {
        this._super();
        var e = this,
            t = $("<div/>", {
                class: "form-group"
            }).append($("<label/>", {
                class: "sr-only",
                text: "Status"
            })).prependTo(e.$form);
        e.$acctype = $("<select/>", {
            class: "form-control"
        }).on("change", {
            self: e
        }, e._onStatusDropdownChanged).append($("<option/>", {
            text: e.def
        })).appendTo(t), $.each(e.acctypes, function(t, n) {
            e.$acctype.append($("<option/>").text(e.acctypesstr[t]).val(e.acctypes[t]))
        })
    },
    _onStatusDropdownChanged: function(e) {
        var t = e.data.self,
            n = $(this).val();
        n !== t.def ? t.addFilter("acctype", n, ["acctype"]) : t.removeFilter("acctype"), t.filter()
    },
    draw: function() {
        this._super();
        var e = this.find("acctype");
        e instanceof window.FooTable.Filter ? this.$acctype.val(e.query.val()) : this.$acctype.val(this.def)
    }
});
var xDown = null,
    yDown = null;

function handleTouchStart(e) {
    xDown = e.touches[0].clientX, yDown = e.touches[0].clientY
}

function handleTouchMove(e) {
    if (xDown && yDown) {
        var t = e.touches[0].clientX,
            n = e.touches[0].clientY,
            a = xDown - t,
            o = yDown - n;
        Math.abs(a) > Math.abs(o) && (a > 0 ? $("#dismiss").click() : $("#sidebarCollapse").click()), xDown = null, yDown = null
    }
}

function logout() {
    return jQuery.ajax({
        type: "GET",
        url: "/login",
        async: !1,
        username: "logmeout",
        password: "logmeout"
    }).done(function() {}).fail(function() {
        document.location = "index.html"
    }), !1
}

function connectWS() {
    "https:" === window.location.protocol ? wsUri = "wss://" + window.location.hostname + "/ws" : "file:" === window.location.protocol && (wsUri = "ws://localhost/ws"), (websock = new WebSocket(wsUri)).addEventListener("message", socketMessageListener), websock.onopen = function(e) {
        websock.send('{"command":"getconf"}'), websock.send('{"command":"status"}')
    }
}

function upload() {
    formData.append("bin", $("#binform")[0].files[0]), inProgress("upload")
}

function login() {
    if ("neo" === document.getElementById("password").value) $("#signin").modal("hide"), connectWS();
    else {
        var e = document.getElementById("password").value,
            t = new XMLHttpRequest;
        t.open("get", "/login", !0, "admin", e),
            t.onload = function(e) {
            4 === t.readyState && (200 === t.status ? ($("#signin").modal("hide"), connectWS()) : alert("Incorrect password!"))
        }, t.send(null)
    }
}

function getLatestReleaseInfo() {
    $.getJSON("https://api.github.com/repos/omersiar/esp-rfid/releases/latest").done(function(e) {
        for (var t = e.assets[0], n = 0, a = 0; a < e.assets.length; a++) n += e.assets[a].download_count;
        var o, s = new Date - new Date(e.published_at);
        o = s < 864e5 ? (s / 36e5).toFixed(1) + " hours ago" : (s / 864e5).toFixed(1) + " days ago";
        var i = e.name + " was updated " + o + " and downloaded " + n.toLocaleString() + " times.";
        $("#downloadupdate").attr("href", t.browser_download_url), $("#releasehead").text(i), $("#releasebody").text(e.body), $("#releaseinfo").fadeIn("slow"), $("#versionhead").text(version)
    }).error(function() {
        $("#onlineupdate").html("<h5>Couldn't get release info. Are you connected to the Internet?</h5>")
    })
}

function allowUpload() {
    $("#upbtn").prop("disabled", !1)
}

function start() {
    (esprfidcontent = document.createElement("div")).id = "mastercontent", esprfidcontent.style.display = "none", document.body.appendChild(esprfidcontent), $("#signin").on("shown.bs.modal", function() {
        $("#password").focus().select()
    }), $("#mastercontent").load("esprfid.htm", function(e, t, n) {
        "success" === t && ($("#signin").modal({
            backdrop: "static",
            keyboard: !1
        }), $('[data-toggle="popover"]').popover({
            container: "body"
        }))
    })
}
$("#update").on("shown.bs.modal", function(e) {
    getLatestReleaseInfo()
}), document.addEventListener("touchstart", handleTouchStart, !1), document.addEventListener("touchmove", handleTouchMove, !1);