var hammer = null;

if(porthole.cameraCanvas != null) {
    //porthole.cameraCanvas.addEventListener("mousemove", onMouseMove);
    //porthole.cameraCanvas.addEventListener("mouseup", onMouseUp);
    //porthole.cameraCanvas.addEventListener("mousedown", onMouseDown);
    porthole.cameraCanvas.addEventListener("keypress", onKeyDown, true);
    porthole.cameraCanvas.addEventListener("keyup", onKeyUp, true);
    
    //document.body.addEventListener('touchstart', function(e){ e.preventDefault(); });
    
    
    hammer = new Hammer(porthole.cameraCanvas, {
        // this prevent default disables keyboard events too...
        //prevent_default: true,

        drag: true,
        drag_block_vertical: true,
        drag_block_horizontal: true,
        drag_min_distance: 1,

        transform: true,
        scale_treshold: 0,
        rotation_treshold: 1,

        tap: true,
        tap_double: false,
        hold: false
    });

}

var movementX = 0;
var movementY = 0;
var lastX = 0;
var lastY = 0;

function onMouseMove(e)
{
   movementX =  e.screenX - lastX;
   movementY =  e.screenY - lastY;

   lastX = e.screenX;
   lastY = e.screenY;

    console.log(movementX);
	var JSONEvent = {
		"event_type": "drag",
		"camera_id": 2,
        "zoom": 1,
		"x": movementX,
		"y": movementY
	};
	socket.send(JSON.stringify(JSONEvent));
}

function onMouseUp(e)
{
	var JSONEvent = {
		"event_type": "mouseup",
		"camera_id": 2,
        "zoom": 1,
        "button": 2,//e.button,
		"x": movementX, 
		"y": movementY
	};
	socket.send(JSON.stringify(JSONEvent));
}

function onMouseDown(e)
{
	var JSONEvent = {
		"event_type": "mousedown",
		"camera_id": 2,
        "zoom": 1,
        "button": 2,//e.button,
		"x": movementX, 
		"y": movementY
	};
	socket.send(JSON.stringify(JSONEvent));
}

function onKeyUp(e)
{
	var JSONEvent = {
		"event_type": "keyup",
        "key": e.keyCode
	};
    console.log(e);
	socket.send(JSON.stringify(JSONEvent));
}

function onKeyDown(e)
{
    var key = e.keyCode;
    if(key == 0) key = e.charCode;

	var JSONEvent = {
		"event_type": "keydown",
        "key": key
	};

    console.log(key);
	socket.send(JSON.stringify(JSONEvent));
}

// Multitouch event handlers
hammer.on("drag", function (ev) 
{
   movementX =  ev.gesture.center.pageX - lastX;
   movementY =  ev.gesture.center.pageY - lastY;

   lastX = ev.gesture.center.pageX;
   lastY = ev.gesture.center.pageY;

    console.log(movementX);
	var JSONEvent = {
		"event_type": "drag",
		"camera_id": 2,
        "zoom": 1,
		"x": movementX,
		"y": movementY
	};
	socket.send(JSON.stringify(JSONEvent));    
});

hammer.on("dragstart", function (ev) 
{
   lastX = ev.gesture.center.pageX;
   lastY = ev.gesture.center.pageY;
   var JSONEvent = {
		"event_type": "mousedown",
		"camera_id": 2,
        "zoom": 1,
        "button": 2,//e.button,
		"x": movementX, 
		"y": movementY
	};
	socket.send(JSON.stringify(JSONEvent));
});

hammer.on("dragend", function (ev) 
{
    var JSONEvent = {
		"event_type": "mouseup",
		"camera_id": 2,
        "zoom": 1,
        "button": 2,//e.button,
		"x": movementX, 
		"y": movementY
	};
	socket.send(JSON.stringify(JSONEvent));
});