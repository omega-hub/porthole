# NOTE: to run this example you need to have the cyclops module installed.
from cyclops import *
from math import *
from random import *
import porthole

root = SceneNode.create('root')

n = 3
for i in range(0, n):
    for j in range(0, n):
        for k in range(0, n):
            x = float(i) / n * 2 - 1
            y = float(j) / n * 2 - 1
            z = float(k) / n * 2 - 1
            s = 0.8 / n
            cube = BoxShape.create(s, s, s)
            cube.setPosition(Vector3(x + 0.5, y + 1, -4.5 + z))
            cube.setEffect('colored -s 10 -g 1.0')
            root.addChild(cube)
            r = random()
            g = random()
            b = random()
            cube.getMaterial().setColor(Color(r,g,b,1), Color(0,0,0,1))

l = Light.create()
l.setPosition(0, 4, 2)
l.setColor(Color('white'))
l.setLightType(LightType.Directional)
l.setLightDirection(Vector3(-0.2,-0.5,-0.1))
l.setAmbient(Color(0.2, 0.2, 0.2, 1))
sm = ShadowMap()
sm.setSoft(True)
l.setShadow(sm)


# create ground
plane = PlaneShape.create(10, 10)
plane.setPosition(Vector3(0, -0.5, -4))
plane.pitch(radians(-90))
plane.setEffect("colored -d white")


camera = getDefaultCamera()
camera.setPosition(-0.5, 2, 0.5)
camera.lookAt(Vector3(0, 1, -3.5), Vector3(0,1,0))


# Setup porthole
porthole.initialize()
porthole.getService().load('manipulator.xml')

# Web interface callbacks
#def onOrientationChanged(pitch, yaw, roll, clientId):
#    box.setOrientation(quaternionFromEulerDeg(pitch, yaw, roll))
#    # Update slider state on all connected clients.
#    porthole.getService().broadcastjs('updateSliders({0}, {1}, {2})'.format(pitch,yaw,roll), clientId)
    