# Basic porthole example
import porthole

# The box position. Will be updated by clients and read by new clients when they
# connect.
savedX = 0
savedY = 0

# Setup porthole
porthole.initialize(4080, './dragSync.html')
ps = porthole.getService()
ps.setConnectedCommand("sendBoxPosition('%id%')")

def sendBoxPosition(clientId):
    ps.sendjs(
        "$('#box').offset({left:" + str(savedX) + ", top:" + str(savedY) + "})", 
        clientId)
