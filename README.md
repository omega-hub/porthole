## Porthole Python API ##
All the porthole API is contained in the `porthole` python module. To start using porthole simply do
```python
import porthole

# The two optional args are the port to be used by the web serter and the 
# default page. If left unspecified, porthole will use port 4080 and will 
# serve index.html by default
porthole.initialize() 

# Get the porthole service
svc = porthole.getService()
```


### `PortholeService` ###
The `PortholeService` class exposes all the basic methods offered by the porthole interface

| **Method** | **Description** |
---|---
`setServerStartedCommand(string cmd)` |
`setConnectedCommand(string cmd)` |
`setDisconnectedCommand(string cmd)` | Command to be called when a client connects, the token `%id%` will be substituted by the client id
`setCameraCreatedCommand(string cmd)` | Command to be called when a client disconnects, the token `%id%` will be substituted by the client id
`setCameraDestroyedCommand(string cmd)` |
`setPointerBounds(Vector2i bounds)`, `Vector2i getPointerBounds()` | Sets the bounds for pointer events coming from web clients. If this is left to (0, 0), bounds will be set to the display canvas size. Values set through this method will only apply to newly connected clients.|
`setPointerSpeed(float speed)`, `float getPointerSpeed()`| Sets the speed for pointer events coming from web clients. Values set through this method will only apply to newly connected clients.


## Porthole Javascript API ##
| **Function** | **Description** |
---|---
`setInterface(string interfaceId)`
