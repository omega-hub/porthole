# Basic porthole example
import porthole

# Setup porthole
porthole.initialize(4080, './chat.html')
ps = porthole.getService()


# Client call this function when posting a message.
def postMessage(message, sender):
    ps.broadcastjs("messageReceived('{0}: {1}')".format(sender, message), '')
