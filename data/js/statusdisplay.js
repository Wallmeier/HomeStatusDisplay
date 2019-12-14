function reboot() {
    var request = new XMLHttpRequest();
    request.open('GET', '/reboot');
    request.send();
}

function saveTable(table, path) {
    var json = JSON.stringify(table.getData());
    var request = new XMLHttpRequest();
    request.open('PUT', path);
    request.send(json);
    console.log("Sending table data: " + json + ", url: " + path);
}

function getMaxIdx(table) {
    var res = 0;
    var data = table.getData();
    for (var idx = 0; idx < data.length; idx++) {
        if (data[idx].id > res)
            res = data[idx].id;
    }
    return res + 1;
}

function updatePage(path) {
    console.log("Updating page, url: " + path);
    var request = new XMLHttpRequest();
    request.addEventListener('load', function(event) {
        if (request.status == 200) {
            var json = JSON.parse(request.responseText);
            for (let key in json) {
                var elem = document.getElementById(key);
                if (elem != undefined)
                    elem.innerHTML = json[key];
            }            
        } else {
            console.warn(request.statusText, request.responseText);
        }
    });    
    request.open('GET', path);
    request.send();
}