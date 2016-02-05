### Global Functions ###

#### initialize ####
#### otherMethod ####
> `initialize([int port], [string defaultPage])`

> `initialize(float x, float y, float z)`

Initializes and starts the porthole web server
- `int port` (optional): the port used by the web server. Default: 4080
- `string defaultPage` (optional): the page to serve when no file is specified. Default: index.html

> **Example**
```python
import porthole
porthole.initialize()
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
