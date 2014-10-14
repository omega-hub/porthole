# This example shows how to draw a slider in a porhole interface,
# draw it on screen using the webViewmodule
# and synchronize multiple sliders using javascript and python
# opena borwser to http://127.0.0.1:4080 and chang the slider
# the slider in the omegalib window should move as well, and vice versa.
from cyclops import *
from webView import*
from browser import *
import porthole

# create a box that will be oriented through sliders
box = BoxShape.create(1,1,1)
box.setEffect('colored -d red')
box.setPosition(0, 2, -4)
l = Light.create()
l.setPosition(0, 4, 0)
l.setColor(Color('white'))
l.setAmbient(Color(0.2, 0.2, 0.2, 1))

def createUi():
    global bw
    # create a webView to show the porthole interface.
    width = 440
    height = 200
    ui = UiModule.createAndInitialize()
    uiroot = ui.getUi()
    bw = BrowserWindow('Box Orientation', uiroot, width, height)
    bw.loadUrl("http://127.0.0.1:4080")
    bw.setDraggable(True)
        
# Setup porthole
porthole.initialize('syncSlider.xml')
porthole.getService().setServerStartedCommand("createUi()")

# Web interface callbacks
def onOrientationChanged(pitch, yaw, roll, clientId):
    box.setOrientation(quaternionFromEulerDeg(pitch, yaw, roll))
    # Update slider state on all connected clients.
    porthole.getService().broadcastjs('updateSliders({0}, {1}, {2})'.format(pitch,yaw,roll), clientId)
    