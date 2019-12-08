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
