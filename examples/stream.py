# This example shows how to do basic camera streaming to 
# a web interface using porthole.
# open a browser to http://127.0.0.1:4080
from cyclops import *
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
ps = porthole.initialize(4080, './stream.html')
camera = None
def createCamera(clientId):
    global camera
    camera = getOrCreateCamera('cam');
    camera.setOverlayEnabled(False)
    print("camera created " + camera.getName())
    pc = porthole.PortholeCamera()
    pc.initialize(camera, 'camera-stream', 400, 400)
    ps.findClient(clientId).addCamera(pc)


#------------------------------------------------------------------------------
# MAIN UPDATE FUNCTION
# Spin the box, and update the porthole cameras
def onUpdate(frame, t, dt):
    box.pitch(dt)
    box.yaw(dt / 3)
setUpdateFunction(onUpdate)
