var devtable = new Tabulator("#devmap-table", {
    ajaxURL:"/devicemapping.json",
 	layout:"fitDataStretch",
    history:true,
 	columns:[
        {title:"No", formatter:"rownum", align:"center"},
	 	{title:"Device", field:"device", editor:"input", validator:["required", "unique"]},
	 	{title:"LED", field:"led", editor:"number", editorParams:{ min:0, max:255, step:1, elementAttributes:{ maxlength:"3", }}, validator:["required", "max:255"]}, 
        {formatter:"buttonCross", align:"center", cellClick:function(e, cell){cell.getRow().delete()}}
 	]
});
