# This example shows how to do basic camera streaming to 
# a web interface using porthole.
# opena browser to http://127.0.0.1:4080
import porthole

# create a spinning box
box = BoxShape.create(1,1,1)
box.setEffect('colored -d red')
box.setPosition(0, 2, -4)
l = Light.create()
l.setPosition(0, 4, 0)
l.setColor(Color('white'))
l.setAmbient(Color(0.2, 0.2, 0.2, 1))

# Setup porthole
porthole.initialize('porthole/examples/basicPorthole.xml')
porthole.getService().setServerStartedCommand("print('porthole web server ready! connect to http://localhost:4080')")
porthole.getService().setCameraCreatedCommand("onCameraCreated(%id%)")

# this dictionary will store the camera associated to each connected client
clientCamera = {}

def onCameraCreated(camId):
    global clientCamera
    camera = getCameraById(camId);
    camera.setOverlayEnabled(False)
    print("camera created " + camera.getName())
    # porthole camera names are in the format [clientId]-[cameraId]
    # where clientId is a string, and cameraId is the numeric
    # camera identifier.
    args = camera.getName().split('-')
    # for each camera entry store the camera reference and a vector with
    # the camera current speed.
    clientCamera[args[0]] = (camera, Vector3(0,0,0))

# Porthole UI event handlers
def setCameraMoveSpeed(x, y, z, clientId):
    if(isMaster()): 
        # update the camera speed
        camera = clientCamera[clientId][0]
        clientCamera[clientId] = (camera, Vector3(x,y,z))


#------------------------------------------------------------------------------
# MAIN UPDATE FUNCTION
# Spin the box, and update the porthole cameras
def onUpdate(frame, t, dt):
    box.pitch(dt)
    box.yaw(dt / 3)
    for item in clientCamera.items():
        item[1][0].translate(item[1][1] * dt, Space.Local)
setUpdateFunction(onUpdate)
