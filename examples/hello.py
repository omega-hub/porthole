#@config: { orun: { initScript = ""; appStart = ""; }; }; 
# Basic porthole example
import porthole

# Setup porthole
porthole.initialize()
ps = porthole.getService()
ps.load('./chat.xml')

# Client call this function when posting a message.
def postMessage(message, sender):
    ps.broadcastjs("messageReceived('{0}: {1}')".format(sender, message), '')
