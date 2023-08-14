"use strict";

var orientIsHoriz;
var activeTab;

var CfgElements = [];

var onresize = function (e) {
    trackOrient();
}

function openTab(evt, tabName) {
    activeTab = tabName;
    var tabcontent = document.getElementsByClassName("tabcontent");
    for (var i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    var tablinks = document.getElementsByClassName("tablinks");
    for (var i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }

    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";

    onresize();
}

function trackOrient(force = false) {
    var newOrient = window.innerWidth < window.innerHeight;
    if (orientIsHoriz !== newOrient || force) {
        orientIsHoriz = newOrient;
        if (newOrient) {
            var list = document.getElementsByClassName("tab");
            for (var index = 0; index < list.length; ++index) {
                list[index].style.float = 'auto';
                list[index].style.height = 'auto';
                list[index].style.width = '100%';
            }
            list = document.getElementsByClassName("tablinks");
            for (index = 0; index < list.length; ++index) {
                list[index].style.padding = '10px 10px';
                list[index].style.width = 'auto';
                list[index].style.marginRight = '2px';
            }
        }
        else {
            var list = document.getElementsByClassName("tab");
            for (var index = 0; index < list.length; ++index) {
                list[index].style.float = 'left';
                list[index].style.boxSizing = 'border-box';
                list[index].style.width = '60px';
            }
            list = document.getElementsByClassName("tablinks");
            for (index = 0; index < list.length; ++index) {
                list[index].style.padding = '10px 10px';
                list[index].style.width = '100%';
                list[index].style.marginBottom = '2px';
            }
        }
    }
}

function isVisible() {
    return document.visibilityState === 'visible';
}

function logAppend(m) {
    var d = new Date();
    document.getElementById('messageLog').innerHTML += "<a style=\"color:blue;font-size:12px\">" +
        d.toLocaleTimeString('en-US', { hour12: false }) + "." + ('00' + d.getMilliseconds()).slice(-3) +
        ": </a><a style=\"font-size:12px\">" +
        m +
        "</a><br/>";
    var elem = document.getElementById('messageLog');
    elem.scrollTop = elem.scrollHeight;
}

function setCookie(name, value, days = 365) {
    var date = new Date();
    date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
    var expires = "; expires=" + date.toGMTString();
    document.cookie = name + "=" + value + expires + "; path=/";
}

function getCookie(cname) {
    let name = cname + "=";
    let decodedCookie = decodeURIComponent(document.cookie);
    let ca = decodedCookie.split(';');
    for (let i = 0; i < ca.length; i++) {
        let c = ca[i];
        while (c.charAt(0) == ' ') {
            c = c.substring(1);
        }
        if (c.indexOf(name) == 0) {
            return c.substring(name.length, c.length);
        }
    }
    return "";
}

function rt_data_cb(resp) {
    // console.log("resp rcv: " + resp);
    const obj = JSON.parse(resp);
    document.getElementById("rtd_vi").innerHTML = (obj.v_i * 0.001).toFixed(1) + " V";
    document.getElementById("rtd_vinv").innerHTML = (obj.v_p * 0.001).toFixed(1) + " / " + (obj.v_n * 0.001).toFixed(1) + " V";
    document.getElementById("rtd_ip").innerHTML = (obj.i_p * 0.001).toFixed(1) + " A";
    document.getElementById("rtd_p").innerHTML = (obj.v_i * obj.i_p * 0.000001).toFixed(1) + " W";
    document.getElementById("rtd_tdrv").innerHTML = (obj.t_drv * .1).toFixed(0) + " °C";
    document.getElementById("rtd_inv").innerHTML = (obj.t_inv_p * .1).toFixed(0) + "/" + (obj.t_inv_n * .1).toFixed(0) + " °C";
    document.getElementById("rtd_lsr").innerHTML = (obj.t_lsr * .1).toFixed(0) + " °C";
    if (obj.hasOwnProperty("console")) {
        logAppend(obj.console);
    }
}

function req_rt_data() {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
            rt_data_cb(xmlHttp.responseText);
    }
    xmlHttp.open("GET", "/api/rtd", true); // true for asynchronous 
    xmlHttp.send(null);
}

function sendWsMsg() {
    console.log("MSG...");
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function () {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
            rt_data_cb(xmlHttp.responseText);
    }
    xmlHttp.open("GET", "/api/cmd/test_cmd", true); // true for asynchronous 
    xmlHttp.send(null);
}

function consoleCmd() {
    if (event.keyCode == 13) {
        console.log("Sending console: '" + document.getElementById("consoleInputField").value + "'");
        var xmlHttp = new XMLHttpRequest();
        xmlHttp.open("GET", "/api/console?" + document.getElementById("consoleInputField").value, true); // true for asynchronous 
        xmlHttp.send(null);
    }
}

window.onload = function () {
    if (("WebSocket" in window) === false) {
        alert("The Browser does not supports WebSockets!");
        location.reload();
    }

    CfgElements.modal = document.getElementById("modalConfig");
    CfgElements.ipBackend = document.getElementById("inputIpBakend");
    CfgElements.span = document.getElementsByClassName("modalClose")[0];

    trackOrient(true);
    window.addEventListener('resize', onresize, false);

    document.getElementById("btn0").click(); // click first tab button
    document.getElementById('pagewrapper').style.display = '';

    logAppend("WebSocket support: " + ("WebSocket" in window));
    logAppend("Self URL: " + window.location.href);
    logAppend("Screen size: " + window.innerWidth + "x" + window.innerHeight);
    logAppend("Pixel density: " + window.devicePixelRatio);
    const loadTime =
        window.performance.timing.domContentLoadedEventEnd -
        window.performance.timing.navigationStart;
    logAppend("Load time: " + loadTime + " ms");

    CfgElements.span.onclick = function () {
        CfgElements.modal.style.display = "none";
        setCookie("backend_ip", CfgElements.ipBackend.value);
    }

    setInterval(req_rt_data, 250);
}

function openSettings() {
    CfgElements.modal.style.display = "block";
    CfgElements.ipBackend.value = getCookie("backend_ip");
}

window.onclick = function (event) {
    if (event.target == CfgElements.modal) {
        CfgElements.modal.style.display = "none";
    }
}

window.openSettings = openSettings;
window.trackOrient = trackOrient;
window.openTab = openTab;
window.isVisible = isVisible;
window.logAppend = logAppend;
window.openSettings = openSettings;
window.sendWsMsg = sendWsMsg;
window.consoleCmd = consoleCmd;