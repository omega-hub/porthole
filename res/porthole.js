var socket; 
var JSONToSend = ''; 

var runningInWebView = false;

if(typeof OMEGA != 'undefined') {
    console.log("Running in WebView")
    runningInWebView = true;
}
else {
    //setInterval("window.requestAnimFrame(omegaFrame)", 18)
}
    
////////////////////////////////////////////////////////////////////////////////
// PORTHOLE JAVASCRIPT API
function phSetInterface(interfaceId) {
    interface_id = interfaceId;
    sendSpec();
}

function phSetSliderValue(sliderId, value) {
    eval(sliderId + "_skip_next_event = true;")
    document.getElementById(sliderId).value = value;
}

function phCall() {
    var theString = arguments[0];
    
    // start with the second argument (i = 1)
    for (var i = 1; i < arguments.length; i++) {
        // "gm" = RegEx options for Global search (more than one instance)
        // and for Multiline search
        var regEx = new RegExp("\\{" + (i - 1) + "\\}", "gm");
        theString = theString.replace(regEx, arguments[i]);
    }
    var callMsg = {
        "event_type": "input",
        "value": "phCall",
        "function": theString
    };
    socket.send(JSON.stringify(callMsg));
}

function phJSCall() {
    var theString = arguments[0];
    
    // start with the second argument (i = 1)
    for (var i = 1; i < arguments.length; i++) {
        // "gm" = RegEx options for Global search (more than one instance)
        // and for Multiline search
        var regEx = new RegExp("\\{" + (i - 1) + "\\}", "gm");
        theString = theString.replace(regEx, arguments[i]);
    }
    var callMsg = {
        "event_type": "input",
        "value": "phJSCall",
        "function": theString
    };
    socket.send(JSON.stringify(callMsg));
}

// Better-looking API, 
porthole = {}
porthole.jscall = phJSCall
porthole.call = phCall
porthole.setInterface = phSetInterface
porthole.cameraCanvas = null;

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

////////////////////////////////////////////////////////////////////////////////
// SEND SPEC TO SERVER
var SEND_SPEC_INTERVAL = 1000; // in Millis 
var send_spec_waiting = false;
var interface_id = undefined;
    
function sendSpec() 
{
    var w=window,d=document,e=d.documentElement,g=d.getElementsByTagName('body')[0],
        viewportwidth=w.innerWidth||e.clientWidth||g.clientWidth,
        viewportheight=w.innerHeight||e.clientHeight||g.clientHeight;

    var orientation;
    
    var iid = getURLArg()["id"];
    if(iid == undefined) iid = "default";
    if(interface_id != undefined) iid = interface_id;

    if (viewportwidth > viewportheight) {
        orientation = "landscape";
    } else {
        orientation = "portrait";
    }

    var SpecMessage = {
        "event_type": "device_spec",
        "width": viewportwidth,
        "height": viewportheight,
        "orientation": orientation,
        "first_time": firstTime,
        "value": iid
    }
    socket.send(JSON.stringify(SpecMessage));

    send_spec_waiting = false; // sendSpecTimeout could be called again
}

porthole.requestInterfaceRefresh = sendSpec;

function sendSpecTimeout() {
    if (!send_spec_waiting) {
        send_spec_waiting = true;
        window.setTimeout(sendSpec, SEND_SPEC_INTERVAL);
    }
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
            image = "data:image/jpeg;base64," + message.base64image;
            img.src = image;
            
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
                        "fps": ( 1000 / dt ) 
                    };
                    //console.log("asking for fps: " + parseInt(1000 / dt));
                    socket.send(JSON.stringify(JSONEvent));
                    overstart = now;
                }
            }
        }
        
        // Received HTML data, place it in porthole_content div.
        else if (message.event_type == "html_elements") {
            document.getElementById("porthole_content").innerHTML = message.innerHTML;
            
            // If we have a camera stream element (i.e. a canvas called camera-canvas)
            // We register a frame callback. Otherwise, this function will do 
            // nothing.
            initializeCameraStreams();
            
            // Add events listeners
            if (!isTouchable) {
                // avoid resize message when touchable keyboard appears on mobile devices
                addEvent(window, "resize", sendSpecTimeout);
            }
        }
        
        // Received Javascript code. Execute it.
        else if(message.event_type == "javascript")
        {
            for(var i = 0; i < message.commands.length; i++)
            {
                console.log('exec ' + message.commands[i].js)
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
function initializeCameraStreams() {
    // camera canvas can have two different ids. If we find a camera-canvas, we
    // initialize jpeg streaming. If we find camera-h264-stream, we initialize
    // H264 stream.
    camera = document.getElementById('camera-canvas');

    if (camera == null) {
        camera = document.getElementById('camera-h264-stream');
        if(camera == null) return;

        player = new Player({useWorker: true});
        player.canvas = camera;
        player.onFrameDrawEnd = frameDrawEnd;
        
        camera.width = camera.parentNode.clientWidth;
        camera.height = camera.parentNode.clientHeight;
    } else {
        camera.width = camera.parentNode.clientWidth;
        camera.height = camera.parentNode.clientHeight;
        
        ctx = camera.getContext("2d");

        // OK, we have the camera, so start camera loop
        cameraLoopEnabled = true;

        // Combine setInterval and requestAnimationFrame in order to get a desired fps
        clearInterval(cameraLoopCallback);
        cameraLoopCallback = setInterval("window.requestAnimFrame(cameraLoop)",
                           1000 / FPS_TARGET);
    }

    porthole.cameraCanvas = camera;
}

////////////////////////////////////////////////////////////////////////////////
// Main camera rendering loop 
function cameraLoop() {
    if (cameraLoopEnabled && ctx != null) {
        ctx.drawImage(img, 0, 0, camera.width, camera.height);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Wrapper around omegalib frame function, to use on a normal browser when we
// are not running inside an omegalib webView
var context = {};
context.frame = 0;
context.time = 0;
context.dt = 0;
var initialTime = Date.now() / 1000;
context.time = initialTime;

context.modelview = null;
context.projection = null;
context.cameraPosition = [0, 2, 0]

context.tileTopLeft = null
context.tileBottomLeft = null
context.tileBottomRight = null
context.activeCanvasRect = null
context.activeRect = null

function omegaFrame() {
    // Update context
    context.frame++;
    curTime = (Date.now() / 1000) - initialTime;
    context.dt = curTime - context.time;
    context.time = curTime;
    
    // Call the user code frame function
    frame(context)
}

