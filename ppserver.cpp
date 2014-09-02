#include <omega.h>
#include "omicron/InputServer.h"

using namespace omicron;
using namespace omega;

///////////////////////////////////////////////////////////////////////////
extern "C" void sigproc(int signal_number)
{
    SystemManager::instance()->postExitRequest();
}

///////////////////////////////////////////////////////////////////////////
class PPServerApplication : public EngineModule
{
public:
    virtual void initialize()
    {
        // Start accepting connections.
        SystemManager* sys = getEngine()->getSystemManager();
        Config* cfg = sys->getSystemConfig();
        myInputServer.startConnection(cfg);

        // Use the python interpreter to load and initialize the porthole web server.
        PythonInterpreter* interpreter = sys->getScriptInterpreter();
        interpreter->eval("import porthole");
        interpreter->eval("porthole.initialize('porthole/portholePointer.xml')");
        interpreter->eval("print('porthole web server ready! connect to http://localhost:4080')");
    }

    virtual void update(const UpdateContext& context)
    {
        // Listen for clients (non-blocking)
        myInputServer.loop();
        myInputServer.startListening();
    }

    virtual void handleEvent(const Event& e)
    {
        // Dispatch events to connected clients.
        myInputServer.handleEvent(e);
    }

private:
    InputServer myInputServer;
};

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    // The application name will be used to find the default config
    // file for ppserver (porthole/ppserver.cfg)
    Application<PPServerApplication> app("porthole/ppserver");
    omain(app, argc, argv);
}
