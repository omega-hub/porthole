import porthole
porthole.initialize()
porthole.getService().load('porthole/portholePointer.xml')
porthole.getService().setServerStartedCommand("print('porthole web server ready! connect to http://localhost:4080')")