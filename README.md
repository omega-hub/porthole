### Global Functions ###

#### initialize ####
> initialize( [int port], [string defaultPage] )

Initializes and starts the porthole web server
- `int port` (optional): the port used by the web server. Default: 4080
- `string defaultPage` (optional): the page to serve when no file is specified. Default: index.html

#### base64EncodeImage ####
> string base64EncodeImage( [PixelData](https://github.com/uic-evl/omegalib/wiki/PixelData) image, [ImageFormat](https://github.com/uic-evl/omegalib/wiki/PixelData#image-formats) format)

Converts an image to a base64 string.
- `PixelData image`: the image to be converted.
- `ImageFormat format`: encoding to be used in the conversion.

----------------------------------------------------------------------------------------------------
### PortholeService ###
The `PortholeService` class exposes all the basic methods offered by the porthole interface

#### setServerStartedCommand ####
> setServerStartedCommand(string cmd)

#### setConnectedCommand ####
##### setDisconnectedCommand #####
> setDisconnectedCommand(string cmd) 

> setConnectedCommand(string cmd)

Sets a command to be called when a client connects or disconnects.
- `string cmd`: command to be called when a client connects or disconnected, the token `%id%` will be substituted by the client id.
