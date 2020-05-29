/*
 * Copyright 2015 Massimo Del Fedele
 * Date: May 2015
 */

// objects types, fixed by now
// kinds are:
//	do	digital output
//	di	digital input
//	ao	analog output
//	ai	analog input
var kinds = {
	'do' : {'handler': toggleDigital, 'updater': showDigital },
	'di' : {'handler': nullHandler,   'updater': showDigital },
	'ao' : {'handler': setAnalog,     'updater': showAnalog  },
	'ai' : {'handler': nullHandler,   'updater': showAnalog  }
};

// items, filled by 'map.txt' file
var items;

var lastUpdateTime;

function nullHandler()
{
}

function changeIcon(domImg, srcImage)
{
	var img = new Image();
	domImg.src = srcImage;
	img.onload = (
		function(domImg, srcImage){
			return function(){
				domImg.src = srcImage;
			};
		})(domImg, srcImage);
	img.src = srcImage;
}

function showDigital(id, val)
{
	if(val == 1)
		changeIcon(document.getElementById('itemimage_' + id), items[id].imageon);
	else
		changeIcon(document.getElementById('itemimage_' + id), items[id].imageoff);
}

function showAnalog(id, val)
{
	// get current item info
	var item = items[id];
	
	// read scale, delta and display precision
	// value is displayed as val * scale + delta
	// with given precision
	var scale, delta, prec;
	if('delta' in item)
		delta = item.delta;
	else
		delta = 0;
	if('scale' in item)
		scale = item.scale;
	else
		scale = 1;
	if('prec' in item)
		prec = item.prec;
	else
		prec = 0;
	val = val * scale + delta;
	val = val.toFixed(prec);
	
	// add pre and suffixes if any
	if('textpref' in item)
		val = item.textpref + val;
	if('textsuff' in item)
		val = val + item.textsuff;
		
	$('#itemtextcont_' + id).text(val);

}

function toggleDigital(id)
{
	var img = $('#itemimage_' + id).attr('src');
	var on;
	if(items[id].imageon == img)
		on = '0';
	else
		on = '1';
	$.ajax({
		type: "POST",
		data: id + '=' + on,
		dataType: "text",
		cache: false,
		url: "/toggleDigital",
		beforeSend: function(xhr){
			if (xhr.overrideMimeType)
			{
				xhr.overrideMimeType("text/plain");
			}
		},
		success: function(r) {
			// refresh immediately the status upon toggling lamp
			getStatus();
		},
		error: function(s, xhr, status, e){
			console.log("POST failed: " + s.responseText);
		}
	});
}

function setAnalog(id, val)
{
}

function showStatus(data)
{
	for(i in data) {
		var val = parseInt(data[i]);
		var item = items[i];
		if(item.kind == 'do' || item.kind == 'di')
			showDigital(i, val);
		else if(item.kind == 'ao' || item.kind == 'ai')
			showAnalog(i, val);
	}
}

var inStatus = false;
function getStatus() {
	if(inStatus)
		return;
	inStatus = true;
	$.ajax({
		dataType: "json",
		beforeSend: function(xhr){
			if (xhr.overrideMimeType){
				xhr.overrideMimeType("application/json");
			}
		},
		data: {_: new Date().getTime()},
  		url: "/status",
		success: function(data) {
			showStatus(data);
			inStatus = false;
			lastUpdateTime = Date.now();
		},
		error: function(s, xhr, status, e){
			console.log("getStatus failed: " + s.responseText);
			inStatus = false;
			lastUpdateTime = Date.now();
		}
	});
};

function PlaceItem(id, item)
{
	// create a container for current item
	var cont = $('<div>');
	cont.attr('id', 'itemcont_' + id);
	cont.attr('class', 'itemcontainer');
	
	// append item to html page
	$('#container').append(cont);

	// create item's image
	var image = $('<img>');
	image.attr('id', 'itemimage_' + id);

	// setup callback to position item inside html page
	image.unbind("load");
	image.bind("load", function ()
	{
		var w = $(this).width();
		var h = $(this).height();
		var x = item.x - w / 2;
		var y = item.y - h / 2;
		cont.attr('style', 'left:' + x + ';bottom:' + y + ';');
	});

	// load item's image
	if('imageoff' in item)
		image.attr('src', item.imageoff);
	else
		image.attr('src', item.image);
		
	// append image to container
	cont.append(image);
	
	// if item has a text, create it and append it
	if('textx' in item) {
		var txtcont = $('<div>');
		txtcont.attr('id', 'itemtextcont_' + id);
		txtcont.attr('class', 'texttip');
		cont.append(txtcont);
		txtcont.attr('left', item.textx);
		txtcont.attr('style', 'left:' + item.textx +';bottom:' + item.texty);
	}
	
	cont.click(function() { kinds[item.kind].handler(id); });
}

function PlaceItems()
{
	for(i in items)
		PlaceItem(i, items[i]);
}

function updateView()
{
	// if updated short time ago, skip
	if(Date.now() > lastUpdateTime + 500)
		getStatus();
	
	// prepare for next update
	window.setTimeout(function() {
		updateView();}, 500
	);
}

$(document).ready(
    function() {

		$.ajax({
			dataType: "json",
			cache: false,
			async: true,

			beforeSend: function(xhr){
				if (xhr.overrideMimeType)
				{
					xhr.overrideMimeType("application/json");
				}
			},
	  		url: "/map.txt",
			success: function(data) {
				items = data;
	
				// place items over home image
				PlaceItems();
				
				// refresh immediately the status upon loading
				getStatus();
				updateView();
			},
			error: function(xhr, status, errorThrown){
				alert(errorThrown + '\n' + status + '\n' + xhr.statusText);
			}
		});
  });
