# This example shows how to do use the porthole pointer interface to send mouse
# and keyboard events to a remote application
# open a browser to http://127.0.0.1:4080
import porthole

# Setup porthole
porthole.initialize()
porthole.getService().load('porthole/portholePointer.xml')
porthole.getService().setServerStartedCommand("print('porthole web server ready! connect to http://localhost:4080')")

#------------------------------------------------------------------------------
def onEvent():
    e = getEvent()
    if(e.getServiceType() == ServiceType.Pointer):
        x = e.getPosition().x
        y = e.getPosition().y
        print("Pointer position: {0},{1}".format(x, y))
    elif(e.getServiceType() == ServiceType.Keyboard and e.getType() == EventType.Down):
        print("Key Pressed: {0} = {1}".format(e.getSourceId(), chr(e.getSourceId())))
    
setEventFunction(onEvent)
