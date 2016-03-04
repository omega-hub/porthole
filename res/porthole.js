var socket; 
var JSONToSend = ''; 
    
////////////////////////////////////////////////////////////////////////////////
// Porthole Javascript API
porthole = {}
porthole.cameraCanvas = null;
porthole.pointerX = 0;
porthole.pointerY = 0;
porthole.connected = null;
porthole.socket = null;

porthole.sendMouseMove = function(e)
{
    if(e.movementX == e.screenX &&
        e.movementY == e.screenY) {
        return;
    }
   movementX = e.movementX;
   movementY = e.movementY;

    var JSONEvent = {
        "event_type": "drag",
        "x": movementX,
        "y": movementY
    };
    socket.send(JSON.stringify(JSONEvent));
}

porthole.sendMouseUp = function(e)
{
    var JSONEvent = {
        "event_type": "mouseup",
        "button": e.button,
        "x": movementX, 
        "y": movementY
    };
    socket.send(JSON.stringify(JSONEvent));
}

porthole.sendMouseDown = function(e)
{
    var JSONEvent = {
        "event_type": "mousedown",
        "button": e.button,
        "x": movementX, 
        "y": movementY
    };
    socket.send(JSON.stringify(JSONEvent));
}

porthole.sendKeyUp = function(e)
{
    var JSONEvent = {
        "event_type": "keyup",
        "key": e.keyCode
    };
    socket.send(JSON.stringify(JSONEvent));
}

porthole.sendKeyDown = function(e)
{
    var JSONEvent = {
        "event_type": "keydown",
        "key": e.keyCode
    };
    socket.send(JSON.stringify(JSONEvent));
}

////////////////////////////////////////////////////////////////////////////////
// Add an event handler to an element
function addEvent(elem, type, eventHandle) {
    if (elem == null || elem == undefined) return;
    if (elem.addEventListener) {
        elem.addEventListener(type, eventHandle, false);
    } else if (elem.attachEvent) {
        elem.attachEvent("on" + type, eventHandle);
    } else {
        elem["on" + type] = eventHandle;
    }
};

// Mobile or desktop
var isTouchable = 'ontouchstart' in document.documentElement;

////////////////////////////////////////////////////////////////////////////////
// Websocket Canvas
function get_appropriate_ws_url() {
    /*
    * We open the websocket encrypted if this page came on an
    * https:// url itself, otherwise unencrypted
    */
    var pcol;
    var u = document.URL;

    if (u.substring(0, 5) == "https") {
        pcol = "wss://";
        u = u.substr(8);
    } else {
        pcol = "ws://";
        if (u.substring(0, 4) == "http")
            u = u.substr(7);
    }

    u = u.split('/');

    return pcol + u[0];
}

////////////////////////////////////////////////////////////////////////////////
// Read URL query argument
function getURLArg() 
{ 
    var query = location.search.substr(1);
    var data = query.split("&");
    var result = {};
    for(var i=0; i<data.length; i++) 
    {
        var item = data[i].split("=");
        result[item[0]] = item[1];
    }
    return result;
} 

// Camera stream vars
var img = new Image();
var player = null;
var ctx;
var camera;

// Websocket Messaging code
var firstTime = 1;
try {
    ////////////////////////////////////////////////////////////////////////////
    // Create the socket
    if ("WebSocket" in window) {
        socket = new WebSocket(get_appropriate_ws_url(), "porthole_websocket");
    //} else if ("MozWebSocket" in window) {
    //    socket = new MozWebSocket(get_appropriate_ws_url(), "porthole_websocket");
    } else {
        document.getElementById("porthole_content").innerHTML =
        "This Browser does not support WebSockets.<br />If using Opera, make sure to enable WebSockets.";
    }
    socket.binaryType="arraybuffer";

    ////////////////////////////////////////////////////////////////////////////
    // Open socket: send the device specification
    socket.onopen = function () {
        // If we have a porthole_content elements to be filled with a 
        // server-generated interface (see porthole xml interface definition files)
        // send the device specification. This will trigger the server to generate
        // an html interface for this device and send it over websocked inside
        // an html_elements message
        if(document.getElementById("porthole_content") != null) {
            sendSpec();
            firstTime = 0;
        }
        
        // Request the porthole functions binder script. This script is generated 
        // by the porthole server and contains the code necessary to forward 
        // html events to python/C++ server callbacks.
        var js = document.createElement("script");
        js.src = "./porthole_functions_binder.js";
        document.body.appendChild(js);
    }

    ////////////////////////////////////////////////////////////////////////////
    socket.onmessage = function gotPacket(msg) {
        if(msg.data instanceof ArrayBuffer) {
           handlePacket(msg.data)
        } else {
            var message = JSON.parse(msg.data);
            handleMessage(message);
        }
    }   

    ////////////////////////////////////////////////////////////////////////////
    // Process a received binary data packet
    function handlePacket(data) {
        // Only binary data supported by porthole is an h264 bitstream.
        if(player != null) {
            //console.log(data.byteLength);
            player.decode(new Uint8Array(data));
        } 
    }

    ////////////////////////////////////////////////////////////////////////////
    function frameDrawEnd() {
        // Keep framerate of the camera
        now = new Date().getTime();
        dt = now - lastT;
        lastT = now;
        //console.log("fps: " + parseInt(1000 / dt));

        if ( ( 1000 / dt ) > FPS_MIN) {
            overFPS = false;
        }
        else {
            // if first time we are over fps, save now
            if ( !overFPS ) overstart = now;
            overFPS = true;
            if ( ( now - overstart ) > FPS_DELTA ) {
                var JSONEvent = {
                    "event_type": "fps_adjust",
                    "camera_id": parseInt(camera.getAttribute("data-camera_id")),
                    "fps": (( 1000 / dt ) * 1.2)
                };
                //console.log("asking for fps: " + parseInt(1000 / dt));
                socket.send(JSON.stringify(JSONEvent));
                overstart = now;
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Process a received JSON message
    function handleMessage(message) {
        // Received image stream. Save it in the img variable
        if (message.event_type == "stream") {
            image = "data:image/png;base64," + message.base64image;
            camera = document.getElementById(message.element_id);
            img.src = image;
            
            if(!cameraLoopEnabled) {
                cameraLoopEnabled = true;
                // Combine setInterval and requestAnimationFrame in order to get a desired fps
                ctx = camera.getContext("2d");
                clearInterval(cameraLoopCallback);
                cameraLoopCallback = setInterval("window.requestAnimFrame(cameraLoop)",
                   1000 / FPS_TARGET);
            }
            
            // Keep framerate of the camera
            /*now = new Date().getTime();
            dt = now - lastT;
            lastT = now;
            //console.log("fps: " + parseInt(1000 / dt));

            if ( ( 1000 / dt ) > FPS_MIN) {
                overFPS = false;
            }
            else {
                // if first time we are over fps, save now
                if ( !overFPS ) overstart = now;
                overFPS = true;
                if ( ( now - overstart ) > FPS_DELTA ) {
                    var JSONEvent = {
                        "event_type": "fps_adjust",
                        "camera_id": parseInt(camera.getAttribute("data-camera_id")),
                        "fps": ( 1000 / dt ) 
                    };
                    //console.log("asking for fps: " + parseInt(1000 / dt));
                    socket.send(JSON.stringify(JSONEvent));
                    overstart = now;
                }
            }*/
        }
        else if(message.event_type == "camera_init") {
            camera = document.getElementById(message.element_id);
            if(camera == null) return;
            camera.width = camera.parentNode.clientWidth;
            camera.height = camera.parentNode.clientHeight;
            if(message.encoder_type == 'llenc') {
                player = new Player({useWorker: true});
                player.canvas = camera;
                player.onFrameDrawEnd = frameDrawEnd;
            } else {
                ctx = camera.getContext("2d");

                // OK, we have the camera, so start camera loop
                cameraLoopEnabled = true;

                // Combine setInterval and requestAnimationFrame in order to get a desired fps
                clearInterval(cameraLoopCallback);
                cameraLoopCallback = setInterval("window.requestAnimFrame(cameraLoop)",
                                   1000 / FPS_TARGET);
            }
        }
        // Received Javascript code. Execute it.
        else if(message.event_type == "javascript")
        {
            for(var i = 0; i < message.commands.length; i++)
            {
                //console.log('exec ' + message.commands[i].js)
                window.eval(message.commands[i].js);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    socket.onclose = function () {
        document.getElementById("porthole_content").innerHTML = "websocket connection CLOSED ";
        // Stop camera loop.
        cameraLoopEnabled = false;
    }
} catch (exception) {
    alert('<p>Error' + exception);
}

porthole.socket = socket;

////////////////////////////////////////////////////////////////////////////////
// Camera stream stuff

// Delta time of request animation frame function calls, in ms
var lastT, dt;
var overFPS = false;
var overSTART = 0;

var FPS_MIN = 20;
var FPS_TARGET = 50;
var FPS_DELTA = 1000; // In ms - adjust frame rate every second.

var cameraLoopCallback = 0;
var cameraLoopEnabled = false;

////////////////////////////////////////////////////////////////////////////////
// Cross-browser find requestAnimFrame function. Will be used to call cameraLoop
window.requestAnimFrame = (function () {
    return window.requestAnimationFrame ||
          window.webkitRequestAnimationFrame ||
          window.mozRequestAnimationFrame ||
          window.oRequestAnimationFrame ||
          window.msRequestAnimationFrame ||
          function (/* function */callback, /* DOMElement */element) {
              // Since our requestAnimFrame is already in a loop in order to
              // control the preferred FPS, just call callback, not an interval
              callback();
          };
      })();

////////////////////////////////////////////////////////////////////////////////
// Main camera rendering loop 
function cameraLoop() {
    if (cameraLoopEnabled && ctx != null) {
        ctx.drawImage(img, 0, 0, camera.width, camera.height);
    }
}
