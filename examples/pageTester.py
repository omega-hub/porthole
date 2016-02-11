
import porthole

# Setup web server
porthole.initialize()
ps = porthole.getService()
ps.setDisconnectedCommand('ps.clearCache()')
