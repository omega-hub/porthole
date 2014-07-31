omegalib offers support for HTML5-capable browsers interactions. Specifically, the interface is multi-touchable, in order to support most of the hand helded devices, such as iPads, iPhones, Android tablets, and Android smartphones. Moreover, the interfaces are tailored on application needs and on devices specifications, without writing any Javascript or JQuery code, but only through an XML file and an, optional, CSS file. An overview of the system implementation is given in the following figure.

<p align="middle"><img src="http://uic-evl.github.io/omegalib/Porthole/porthole_overview.jpg" width="400"/></p>

Basically, the Porthole Service creates a Server in a separate thread, that handles all the incoming connections. But all the steps between the XML description and the actual management of the different devices, including camera streaming and manipulations, are transparent to the application developer.

## Porthole Python API ##
All the porthole API is contained in the `porthole` python module. To start using porthole simply do
```python
import porthole

# The two optional args are the paths to the xml description file and a CSS 
# style sheet for the interface. Leave unspecified to use a default demonstration interface.
porthole.initialize("interface.xml", "interface.css") 

# Get the porthole service
svc = porthole.getService()
```

### `PortholeService` ###
The `PortholeService` class exposes all the basic methods offered by the porthole interface

| **Method** | **Description** |
---|---
|`setServerStartedCommand(string cmd)` |
|`setConnectedCommand(string cmd)` |
|`setDisconnectedCommand(string cmd)` |
|`setCameraCreatedCommand(string cmd)` |
|`setCameraDestroyedCommand(string cmd)` |
|`setPointerBounds(Vector2i bounds)`, `Vector2i getPointerBounds()` | Sets the 
bounds for pointer events coming from web clients. If this is left to (0, 0), 
bounds will be set to the display canvas size. Values set through this method 
will only apply to newly connected clients.|
|`setPointerSpeed(float speed)`, `float getPointerSpeed()`|


## Porthole Javascript API
| **Function** | **Description** |
---|---
`setInterface(string interfaceId)`
