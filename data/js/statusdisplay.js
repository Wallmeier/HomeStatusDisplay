function reboot() {
    console.log("Rebooting...");
    socket.send(JSON.stringify({method: "reboot"}));
    setTimeout(function() {
        console.log("Reloading page after reboot");
        location.reload();
    }, 15000);
};

function saveTable(table, tableName) {
    socket.send(JSON.stringify({method: "updateTable", table: tableName, data: table.getData()}));
    console.log("Sent table " + tableName);
};

function saveConfig() {
    var formCfg = document.getElementById("formConfig");
    var inputs = document.getElementsByTagName('input');
    var result = {};
    console.log("saveConfig() - " + inputs.length);
    for (var i = 0; i < inputs.length; i++) {
        var input = inputs[i];
        if (input.form === formCfg) {
            if (input.name.length > 0) {
                if (input.type == "checkbox")
                    result[input.name] = input.checked;
                else
                    result[input.name] = input.value;
            }
        }
    }
    socket.send(JSON.stringify({method: "saveCfg", data: result}));
};

function uploadConfig(id) {
    var input = document.getElementById(id);
    if (input.files.length > 0) {
        var reader = new FileReader();
        reader.onload = function(){
            socket.send(JSON.stringify({method: "importCfg", filename:input.files[0].name, data: reader.result}));
        };
        reader.readAsText(input.files[0]);
    } else {
        console.warn("No file selected");
    }
};

function getMaxIdx(table) {
    var res = 0;
    var data = table.getData();
    for (var idx = 0; idx < data.length; idx++)
        if (data[idx].id > res)
            res = data[idx].id;
    return res + 1;
};

function updSlideValue(val, elemId) {
    document.getElementById(elemId).innerHTML = val;
};

function loadScript(url, callback) {
    var script = document.createElement('script');
    script.type = 'text/javascript';
    script.src = url;

    // Then bind the event to the callback function. There are several events for cross browser compatibility.
    script.onreadystatechange = callback;
    script.onload = callback;

    // Fire the loading
    document.head.appendChild(script);
};

function loadCss(url, callback) {
    var link = document.createElement('link');
    link.type = 'text/css';
    link.rel = 'stylesheet';
    link.href = url;

    // Then bind the event to the callback function. There are several events for cross browser compatibility.
    link.onreadystatechange = callback;
    link.onload = callback;

    // Fire the loading
    document.head.appendChild(link);
};

function onPageLoad() {
    console.log("onPageLoad()");
    var divSpinner = document.getElementById("status.spinner");
    divSpinner.classList.remove("hidden");
    divSpinner.classList.add("visible");
    divSpinner.classList.add("is-active");
    loadCss("css/file.css", function() {
        console.log("css/file.css loaded");
    });
    loadCss("css/getmdl-select.min.css", function() {
        console.log("css/getmdl-select.min.css loaded");
        loadScript("js/getmdl-select.min.js", function() {
            console.log("js/getmdl-select.min.js loaded");
            getmdlSelect.init('.getmdl-select');
        });
    });
    loadScript("js/tabulator_core.min.js", function() {
        console.log("js/tabulator_core.min.js loaded");
        loadScript("js/tabulator_mods.min.js", function() {
            console.log("js/tabulator_mods.min.js loaded");
            loadCss("css/tabulator-mat.min.css", function() {
                console.log("css/tabulator-mat.min.css loaded");
                initColMapTable();
                initDevMapTable();
                initLedTable();

                console.log("Requesting status page information");
                var request = new XMLHttpRequest();
                request.addEventListener('load', function(event) {
                    divSpinner.classList.remove("is-active");
                    divSpinner.classList.remove("visible");
                    divSpinner.classList.add("hidden");
                    if (request.status == 200) {
                        console.log("Received: " + request.responseText);
                        createStatusPage(JSON.parse(request.responseText));

                        loadScript("js/rec-websocket.min.js", function() {
                            socket = new ReconnectingWebSocket("ws://" + location.host + "/ws", null, {debug: true, reconnectInterval: 3000});
                            socket.onopen = function() {
                                console.log("WebSocket connected");
                            };
                            socket.onclose = function() {
                                console.log("WebSocket.closed");
                            };
                            socket.onmessage = function(event) {
                                console.log("WebSocket message received: ", event);

                                // create a JSON object
                                var jsonObject = JSON.parse(event.data);
                                var method = jsonObject.method;
                                if (method === "update") {
                                    for (let key in jsonObject.fields) {
                                        var elem = document.getElementById(key);
                                        if (elem != undefined)
                                            elem.textContent = jsonObject.fields[key];
                                        else
                                            console.warn("No document element with id '" + key + "'");
                                    }
                                } else if (method === "updLeds") {
                                    updateLedTable(jsonObject.data);
                                }
                            };
                        });
                    } else {
                        console.warn(request.statusText, request.responseText);
                    }
                });
                request.open("GET", "/ajax/status.json");
                request.send();
                
                var request2 = new XMLHttpRequest();
                request2.addEventListener('load', function(event) {
                    if (request.status == 200) {
                        console.log("Received: " + request2.responseText);
                        createConfigPage(JSON.parse(request2.responseText));
                    } else {
                        console.warn(request.statusText, request2.responseText);
                    }
                });
                request2.open("GET", "/ajax/config.json");
                request2.send();
            });
        });
    });
};

function initColMapTable() {
    console.log("initColMapTable()");
    var colorEditor = function(cell, onRendered, success, cancel, editorParams) {
        var editor = document.createElement("input");
        editor.setAttribute("type", "color");
        editor.style.padding = "3px";
        editor.style.width = "100%";
        editor.style.boxSizing = "border-box";
        editor.value = "#" + cell.getValue();
        editor.addEventListener("change", function() {
            var res = editor.value.toUpperCase(); // .substr(1)
            console.log("Selected color: " + res);
            if (res != cell.getValue())
                success(res);
            else
                cancel();
        });

        onRendered(function() {
            editor.focus();
            editor.style.css = "100%";
            editor.click();
        });

        return editor;
    };

    coltable = new Tabulator("#colmap-table", {
        layout: "fitDataStretch",
        history: true,
        columns: [
            {title:"No", field:"id", formatter:"rownum", align:"center"},
            {title:"Message", field:"msg", editor:"input", validator:["required", "unique"]},
            {title:"Color", field:"col", formatter:"color", editor:colorEditor},
            {title:"Behavior", field:"beh", formatter:"lookup", formatterParams:{
                    "1": "On",
                    "2": "Blinking",
                    "3": "Flashing",
                    "4": "Flickering"
                }, editor:"select", editorParams:{
                    defaultValue: "On",
                    values:{
                        "1": "On",
                        "2": "Blinking",
                        "3": "Flashing",
                        "4": "Flickering"
                    }
                }, validator:"required"},
            {formatter:"buttonCross", align:"left", cellClick:function(e, cell){cell.getRow().delete()}}
        ]
    });
};

function initDevMapTable() {
    console.log("initDevMapTable()");
    devtable = new Tabulator("#devmap-table", {
        layout: "fitDataStretch",
        history: true,
        columns: [
            {title:"No", formatter:"rownum", align:"center"},
            {title:"Device", field:"device", editor:"input", validator:["required", "unique"]},
            {title:"LED", field:"led", editor:"number", editorParams:{ min:0, max:255, step:1, elementAttributes:{ maxlength:"3", }}, validator:["required", "max:255"]},
            {formatter:"buttonCross", align:"left", cellClick:function(e, cell){cell.getRow().delete()}}
        ]
    });
};

function initLedTable() {
    console.log("initLedTable()");
    ledTable = new Tabulator("#led-table", {
        layout: "fitDataStretch",
        columns: [
            {title:"No", formatter:"rownum", align:"center"},
            {title:"LED", field:"led"},
            {title:"Device", field:"device"},
            {title:"Color", field:"col", formatter:"color"},
            {title:"Behavior", field:"beh", formatter:"lookup", formatterParams:{
                "1": "On",
                "2": "Blinking",
                "3": "Flashing",
                "4": "Flickering"
            }}
        ]
    });
};

function updateLedTable(data) {
    var divLedTable = document.getElementById("led-table");
    var divNoLeds = document.getElementById("noLeds");
    if (data != undefined && data.length > 0) {
        divLedTable.classList.remove("hidden");
        divLedTable.classList.add("visible");
        divNoLeds.classList.remove("visible");
        divNoLeds.classList.add("hidden");
        ledTable.setData(data);
    } else {
        divLedTable.classList.remove("visible");
        divLedTable.classList.add("hidden");
        divNoLeds.classList.remove("hidden");
        divNoLeds.classList.add("visible");
    }
}

function createConfigPage(jsonRoot) {
    if (jsonRoot.entries != undefined && jsonRoot.entries.length > 0) {
        var json = jsonRoot.entries;
        console.log("Creating config page");
        for (var idx = 0; idx < json.length; idx++) {
            var lastDiv = false;
            var div = document.getElementById("tab-cfg" + json[idx].name);
            for (var child = div.firstElementChild; child != null; child = div.firstElementChild)
                child.remove();            
        
            for (var i = 0; i < json[idx].entries.length; i++) {
                var fieldName = json[idx].name + "." + json[idx].entries[i].key;
                var elem;
                switch (json[idx].entries[i].type) {
                    case 0: // String
                    case 1: // Password
                    case 5: // Word
                        if (lastDiv)
                            div.appendChild(document.createElement("br"));
                        lastDiv = true;
                        elem = document.createElement("div");
                        elem.classList.add("mdl-textfield");
                        elem.classList.add("mdl-js-textfield");
                        elem.classList.add("mdl-textfield--floating-label");
                        var input = document.createElement("input");
                        input.name = fieldName;
                        input.id = fieldName;
                        input.classList.add("mdl-textfield__input");
                        input.type = json[idx].entries[i].type == 1 ? "password" : "text";
                        if (json[idx].entries[i].pattern != undefined)
                            input.pattern = json[idx].entries[i].pattern;
                        input.size = 30;
                        input.setAttribute("value", json[idx].entries[i].value);
                        if (json[idx].entries[i].type == 5)
                            input.setAttribute("maxlength", 5);
                        elem.appendChild(input);
                        var label = document.createElement("label");
                        label.classList.add("mdl-textfield__label");
                        label.setAttribute("for", fieldName);
                        label.textContent = json[idx].entries[i].label;
                        elem.appendChild(label);
                        if (json[idx].entries[i].patternMsg != undefined) {
                            var msg = document.createElement("span");
                            msg.classList.add("mdl-textfield__error");
                            msg.textContent = json[idx].entries[i].patternMsg;
                            elem.appendChild(msg);
                        }
                        break;
                    
                    case 2: // Boolean
                        if (lastDiv)
                            div.appendChild(document.createElement("br"));
                        lastDiv = false;
                        elem = document.createElement("label");
                        elem.classList.add("mdl-switch");
                        elem.classList.add("mdl-js-switch");
                        elem.classList.add("mdl-js-ripple-effect");
                        elem.setAttribute("for", fieldName);
                        var input = document.createElement("input");
                        input.name = fieldName;
                        input.id = fieldName;
                        input.classList.add("mdl-switch__input");
                        input.type = "checkbox";
                        if (json[idx].entries[i].value)
                            input.setAttribute("checked", "");
                        elem.appendChild(input);
                        var label = document.createElement("span");
                        label.classList.add("mdl-switch__label");
                        label.textContent = json[idx].entries[i].label;
                        elem.appendChild(label);
                        break;
                        
                    case 3: // Gpio
                        if (lastDiv)
                            div.appendChild(document.createElement("br"));
                        lastDiv = true;
                        elem = document.createElement("div");
                        elem.classList.add("mdl-textfield");
                        elem.classList.add("mdl-js-textfield");
                        elem.classList.add("mdl-textfield--floating-label");
                        elem.classList.add("getmdl-select");
                        elem.classList.add("getmdl-select__fix-height");
                        var input = document.createElement("input");
                        input.type = "text";
                        input.setAttribute("value", "");
                        input.classList.add("mdl-textfield__input");
                        input.id = fieldName;
                        input.readOnly = true;
                        elem.appendChild(input);
                        input = document.createElement("input");
                        input.type = "hidden";
                        input.setAttribute("value", json[idx].entries[i].value);
                        input.name = fieldName;
                        elem.appendChild(input);
                        var img = document.createElement("i");
                        img.classList.add("mdl-icon-toggle__label");
                        img.classList.add("material-icons");
                        img.textContent = "keyboard_arrow_down";
                        elem.appendChild(img);
                        var label = document.createElement("label");
                        label.setAttribute("for", fieldName);
                        label.classList.add("mdl-textfield__label");
                        label.textContent = json[idx].entries[i].label;
                        elem.appendChild(label);
                        var list = document.createElement("ul");
                        list.setAttribute("for", fieldName);
                        list.classList.add("mdl-menu");
                        list.classList.add("mdl-menu--bottom-left");
                        list.classList.add("mdl-js-menu");
                        
                        for (var j = 0; j < jsonRoot.gpios.length; j++) {
                            var item = document.createElement("li");
                            item.classList.add("mdl-menu__item");
                            item.setAttribute("data-val", jsonRoot.gpios[j]);
                            if (jsonRoot.gpios[j] == json[idx].entries[i].value)
                                item.setAttribute("data-selected", "true");
                            item.textContent = "GPIO-" + jsonRoot.gpios[j];
                            list.appendChild(item);
                        }                        
                        elem.appendChild(list);
                        break;
                
                    case 4: // slider
                        lastDiv = false;
                        elem = document.createElement("p");
                        elem.style.width = "300px";
                        var label = document.createElement("label");
                        label.setAttribute("for", fieldName);
                        label.appendChild(document.createTextNode(json[idx].entries[i].label + ": "));
                        var span = document.createElement("span");
                        span.id = fieldName + ".value";
                        span.textContent = json[idx].entries[i].value;
                        label.appendChild(span);
                        elem.appendChild(label);
                        var input = document.createElement("input");
                        input.classList.add("mdl-slider");
                        input.classList.add("mdl-js-slider");
                        input.type = "range";
                        input.name = fieldName;
                        input.id = fieldName;
                        input.min = 0;
                        input.max = json[idx].entries[i].maxVal;
                        input.setAttribute("value", json[idx].entries[i].value);
                        input.step = 1;
                        input.setAttribute("oninput", "updSlideValue(this.value, '" + fieldName + ".value')");
                        input.setAttribute("onchange", "updSlideValue(this.value, '" + fieldName + ".value')");
                        elem.appendChild(input);
                        break;
                }
                if (elem != undefined) 
                    div.appendChild(elem);
            }
        }
        componentHandler.upgradeAllRegistered();
        getmdlSelect.init(".getmdl-select");
    }
};

function createStatusPage(json) {
    if (json.table != undefined) {
        var statusDiv = document.getElementById("status.content");
        for (var child = statusDiv.firstElementChild; child != null; child = statusDiv.firstElementChild) // clear previous content
            child.remove();
        var table = document.createElement("table");
        for (var groupIdx = 0; groupIdx < json.table.length; groupIdx++) {
            var group = json.table[groupIdx];
            if (groupIdx > 0)
                table.appendChild(document.createElement("tr"));
            for (var idx = 0; idx < group.entries.length; idx++) {
                var entry = group.entries[idx];
                var tableRow = document.createElement("tr");
                var tableCol = document.createElement("td");
                if (idx === 0)
                    tableCol.textContent = group.name;
                tableRow.appendChild(tableCol);

                tableCol = document.createElement("td");
                tableCol.textContent = entry.label;
                tableRow.appendChild(tableCol);

                tableCol = document.createElement("td");
                if (entry.id != undefined) {
                    if (entry.unit != undefined) {
                        var spanElem = document.createElement("span");
                        spanElem.id = entry.id;
                        spanElem.textContent = entry.value;
                        tableCol.appendChild(spanElem);
                        tableCol.appendChild(document.createTextNode(" " + entry.unit));
                    } else {
                        tableCol.id = entry.id;
                        tableCol.textContent = entry.value;
                    }
                } else {
                    if (entry.unit != undefined)
                        tableCol.textContent = entry.value + " " + entry.unit;
                    else
                        tableCol.textContent = entry.value;
                }
                tableRow.appendChild(tableCol);

                table.appendChild(tableRow);
            }
        }
        updateLedTable(json.leds);
        statusDiv.appendChild(table);
    }
};
