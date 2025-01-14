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
    document.getElementById("rtd_u_i").innerHTML = (obj.u_in * 0.001).toFixed(1) + " V";
    document.getElementById("rtd_u_inv").innerHTML = (obj.u_p24 * 0.001).toFixed(1) + " / " + (obj.u_n24 * 0.001).toFixed(1) + " V";
    document.getElementById("rtd_i_24").innerHTML = (obj.i_24 * 0.001).toFixed(1) + " A";
    document.getElementById("rtd_p").innerHTML = (obj.u_p24 * obj.i_24 * 0.000001).toFixed(1) + " W";
    document.getElementById("rtd_t_amp").innerHTML = (obj.t_amp * .1).toFixed(0) + " °C";
    document.getElementById("rtd_t_inv").innerHTML = (obj.t_inv_p * .1).toFixed(0) + "/" + (obj.t_inv_n * .1).toFixed(0) + " °C";
    document.getElementById("rtd_t_head").innerHTML = (obj.t_head * .1).toFixed(0) + " °C";
    document.getElementById("rtd_t_stm").innerHTML = (obj.t_stm * .1).toFixed(0) + " °C";

    document.getElementById("rtd_fan").innerHTML = (obj.fan).toFixed(0) + " RPM";
    document.getElementById("rtd_lum").innerHTML = (obj.lum).toFixed(0) + " Lux";

    document.getElementById("rtd_stm_err").innerHTML = obj.stm_err + "/" + obj.stm_err_l;

    document.getElementById("rtd_xlx").innerHTML = (obj.xlx * 0.001).toFixed(3) + " G";
    document.getElementById("rtd_xly").innerHTML = (obj.xly * 0.001).toFixed(3) + " G";
    document.getElementById("rtd_xlz").innerHTML = (obj.xlz * 0.001).toFixed(3) + " G";

    document.getElementById("rtd_gx").innerHTML = (obj.gx * 0.001).toFixed(3) + " °";
    document.getElementById("rtd_gy").innerHTML = (obj.gy * 0.001).toFixed(3) + " °";
    document.getElementById("rtd_gz").innerHTML = (obj.gz * 0.001).toFixed(3) + " °";

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

function startUpload() {
    document.getElementById("status").innerHTML = "Downloading...";
    var otafile = document.getElementById("otafile").files;
    var file = otafile[0];
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                // document.open();
                // document.write(xhr.responseText);
                document.getElementById("status").innerHTML = xhr.responseText;
                // document.close();
            } else if (xhr.status == 0) {
                alert("Server closed the connection");
                location.reload();
            } else {
                alert(xhr.status + " Error!\n" + xhr.responseText);
                location.reload();
            }
        }
    };
    xhr.upload.onprogress = function (e) { document.getElementById("status").innerHTML = ((e.loaded / e.total * 100).toFixed(0)) + "%"; };
    xhr.open("POST", "/update", true);
    xhr.send(file);
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

window.startUpload = startUpload;