#include <omega.h>
#include "omicron/InputServer.h"

using namespace omicron;
using namespace omega;

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    omsg("OmicronSDK - oinputserver");
    omsg("Copyright (C) 2010-2014 Electronic Visualization Laboratory\nUniversity of Illinois at Chicago");
    omsg("======================================================");
    omsg("");

    InputServer app;
    
    // Read config file name from command line or use default one.
    const char* cfgName = "porthole/ppserver.cfg";
    if(argc == 2) cfgName = argv[1];

    Config* cfg = new Config(cfgName);

    SystemManager* sys = SystemManager::instance();
    // Initialize the data manager
    Ref<DataManager> dataManager = sys->getDataManager();
    dataManager->addSource(new FilesystemDataSource("./"));
    dataManager->addSource(new FilesystemDataSource(""));
    String modulePath = OMEGA_HOME;
    modulePath = modulePath + "/modules";
    dataManager->addSource(new FilesystemDataSource(modulePath));

    sys->setup(cfg);
    sys->initialize();


    ServiceManager* sm = sys->getServiceManager();

    app.startConnection(cfg);

    Ref<PythonInterpreter> interpreter = sys->getScriptInterpreter();
    interpreter->eval("import porthole");
    interpreter->eval("porthole.initialize('porthole/portholePointer.xml')");
    interpreter->eval("print('porthole web server ready! connect to http://localhost:4080')");

    omsg("ppserver: Starting to listen for clients...");
    int i = 0;
    while(true)
    {
        sm->poll();
        app.loop();

        // Start listening for clients (non-blocking)
        app.startListening();

        // Get events
        int av = sm->getAvailableEvents();
        //ofmsg("------------------------loop %1%  av %2%", %i++ %av);
        if(av != 0)
        {
            // TODO: Instead of copying the event list, we can lock the main one.
            Event evts[OMICRON_MAX_EVENTS];
            sm->getEvents(evts, OMICRON_MAX_EVENTS);
            for( int evtNum = 0; evtNum < av; evtNum++)
            {
                app.handleEvent(evts[evtNum]);
            }
        }
        osleep(1);
    }

    sm->stop();
    delete sm;
    delete cfg;
}
