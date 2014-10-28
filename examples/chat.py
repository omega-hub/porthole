# Basic porthole-based chat example (no graphics)
# open a browser to http://127.0.0.1:4080
import porthole

# Setup porthole
porthole.initialize()
ps = porthole.getService()
ps.load('./chat.xml')

# Client call this function when posting a message.
def postMessage(message, sender):
    ps.broadcastjs("messageReceived('{0}: {1}')".format(sender, message), '')
