var colorEditor = function(cell, onRendered, success, cancel, editorParams){
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

    onRendered(function(){
        editor.focus();
        editor.style.css = "100%";
        editor.click();
    });

    return editor;
};

var coltable = new Tabulator("#colmap-table", {
    ajaxURL:"/colormapping.json",
 	layout:"fitDataStretch", 
    history:true,
 	columns:[
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
        {formatter:"buttonCross", align:"center", cellClick:function(e, cell){cell.getRow().delete()}}
 	]
});
