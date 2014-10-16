# Basic porthole-based chat example (no graphics)
# open a browser to http://127.0.0.1:4080
import porthole

# Setup porthole
porthole.initialize()
ps = porthole.getService()
ps.setServerStartedCommand("print('porthole web server ready! connect to http://localhost:4080')")
ps.load('./chat.xml')

#------------------------------------------------------------------------------
def msg(text):
    ps.broadcastjs("messageReceived('{0}')".format(text), '')
    
def postMessage(message, sender):
    ps.broadcastjs("messageReceived('{0}: {1}')".format(sender, message), '')
