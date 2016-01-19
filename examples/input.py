# Basic porthole example
import porthole

# Setup porthole
porthole.initialize(4080, './input.html')
ps = porthole.getService()


# print input events
def onEvent():
    e = getEvent()
    if(e.getServiceType() == ServiceType.Keyboard):
        if(e.getType() == EventType.Down):
            print("Key down: " + chr(e.getSourceId()))
    if(e.getServiceType() == ServiceType.Pointer):
        if(e.getType() == EventType.Down):
            print("Pointer button down")
        elif(e.getType() == EventType.Up):
            print("Pointer button up")
        elif(e.getType() == EventType.Move):
            print("Pointer move:" + str(e.getPosition()))
        

setEventFunction(onEvent)