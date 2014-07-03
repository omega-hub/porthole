# This example shows how to do basic camera streaming to 
# a web interface using porthole.
# opena browser to http://127.0.0.1:4080
import porthole

# Setup porthole
porthole.initialize('porthole/default.xml')
porthole.getService().setServerStartedCommand("print('porthole web server ready! connect to http://localhost:4080')")
porthole.getService().setCameraCreatedCommand("onCameraCreated(%id%)")

def onCameraCreated(camId):
    global clientCamera
    camera = getCameraById(camId);
    #camera.setOverlayEnabled(False)
    dc = getDefaultCamera()
    camera.setHeadOffset(dc.getHeadOffset())
    camera.setPosition(0,0,0)
    #camera.setOrientation(dc.getOrientation())
    dc.addChild(camera)
    camera.setNearFarZ(dc.getNearZ(), dc.getFarZ())
    print("camera created " + camera.getName())
