### Global Functions ###

#### initialize ####
> initialize(int port = 4080, string defaultPage = "index.html")

Initializes and starts the porthole web server
- `port` (optional): the port used by the web server. Default: 4080
- `defaultPage` (optional): the page to serve when no file is specified. Default: index.html

#### base64EncodeImage ####
> string base64EncodeImage( [PixelData](https://github.com/uic-evl/omegalib/wiki/PixelData) image, [ImageFormat](https://github.com/uic-evl/omegalib/wiki/PixelData#image-formats) format)

Converts an image to a base64 string.
- `image`: the image to be converted.
- `format`: encoding to be used in the conversion.

----------------------------------------------------------------------------------------------------
### PortholeService ###
The `PortholeService` class exposes all the basic methods offered by the porthole interface

#### setServerStartedCommand ####
> setServerStartedCommand(string cmd)

#### setConnectedCommand ####
#### setDisconnectedCommand ####
> - setDisconnectedCommand(string cmd) 
> - setConnectedCommand(string cmd)

Sets a command to be called when a client connects or disconnects.
- `cmd`: command to be called when a client connects or disconnected, the token `%id%` will be substituted by the client id.

#### sendjs ####
> sendjs(string js, string destination)

Sends javascript code to a client.
- `js`: the code to be executed.
- `destination` the client id of the destination.

#### broadcastjs ####
> broadcastjs(string js, string origin = "")

Broadcasts javascript to all clients, excluding an optional origin.
- `js`: the code to be executed
- `origin`: optional client id of an origin client. The origin will be excluded from the broadcast.

#### clearCache ####
> clearCache()

Clears the cache of all preprocessed html and js files, forcing a reload the next time the files are requested.

**Example**

This example sets up a basic web server that clear the cache every time a user disconnects. This 
lets' users edit page code and test the changes without restarting the server.
```python
import porthole
porthole.initialize()
ps = porthole.getService()
ps.setDisconnectedCommand('ps.clearCache()')
```


----------------------------------------------------------------------------------------------------
### Porthole.js ###
The `Porthole.js` interface is used in html files server by porthole to interface back with the server.

*Example*
```html
<html>
<head>
    <script src="porthole/res/porthole.js"></script>
</head>
<body>
    <script>
        porthole.connected = function() {
            // Call the print function on the server, printing this client name
            {{py print('%client_id%') }}
        }
    </script>
</body>
</html>
```

#### connected ####
Stores a function that will be called when a connection with the server is established.

#### socket ####
The websocket object used for the server connection

#### sendMouseMove ####
> porthole.sendMouseMove(event)

#### sendMouseUp ####
#### sendMouseDown ####
> porthole.sendMouseUp(event)

> porthole.sendMouseDown(event)

#### sendKeyUp ####
#### sendKeyDown ####
> porthole.sendKeyUp(event)

> porthole.sendKeyDown(event)

### Invoking server commands ###
From a javascript file or html page, you can use the **double bracket notation** to invoke a command on another target (the server, another connected application or another webpage).

#### Server call ####
> `{{py **command**}}`

Invokes `command` on the porthole server. Command can be any valid python expression, or an [omegalib quick command](https://github.com/uic-evl/omegalib/wiki/QuickCommands) such as :q. The special token `%client_id% will be substituted with the id string of this client.

**Example: passing values**
```javascript
x = 10
function(event, y) {
    // This call will print on the server the value of client
    // global variable x (10)
    {{py print(%x%) }}
    
    event.y = y; 
    {{py print(%event.y%) }}

    // This call will print on the server the value of SERVER
    // variable z
    {{py print(z) }}
}
```

#### Mission Control call ####
> `{{mc[@client:] **command**}}`

Invokes `command` on a specific mission control target (if specified after `@`) or on all clients if a target is not 
specified. [Read more about mission control here](https://github.com/uic-evl/omegalib/wiki/MissionControl)

#### Javascript call ####
> `{{js **command**}}`

Invokes `command` on **all** other connected porthole clients.

**Example: Synchronized dragging**

Client
```javascript
<html lang="en">
<head>
    <title>Drag Sync</title>
    <script src="http://ajax.googleapis.com/ajax/libs/jquery/1.9.1/jquery.min.js"></script>
    <script src="//code.jquery.com/ui/1.11.4/jquery-ui.js"></script>
    <script src="porthole/res/porthole.js"></script>
</head>
<body>
    <style>
        #box {
            width: 200px; 
            height: 200px; 
            position: absolute; 
            background-color: yellow
        };
    </style>
    <div id="box"></div>
    <script>
        porthole.connected = function() {
            $("#box").draggable({
                drag: function(){
                    var offset = $(this).offset();
                    xPos = offset.left;
                    yPos = offset.top;
                    
                    // Save the current position on the server, so new clients 
                    // can retrieve it
                    {{py savedX=%xPos%; savedY=%yPos% }}
                    
                    // Update the box position on other clients
                    {{js o = $('#box').offset({ left:%xPos%, top:%yPos%}) }}
                }
            });
        }
    </script>
</body>
</html>
```

Server
```python
# Basic porthole example
import porthole

# The box position. Will be updated by clients and read by new clients when they
# connect.
savedX = 0
savedY = 0

# Setup porthole
porthole.initialize(4080, './dragSync.html')
ps = porthole.getService()
ps.setConnectedCommand("sendBoxPosition('%id%')")

def sendBoxPosition(clientId):
    ps.sendjs(
        "$('#box').offset({left:" + str(savedX) + ", top:" + str(savedY) + "})", 
        clientId)
```
