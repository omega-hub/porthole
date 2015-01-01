/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, 
 *							University of Illinois at Chicago
 * Authors:										
 *  Daniele Donghi			d.donghi@gmail.com
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory,  
 * University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this 
 * list of conditions and the following disclaimer. Redistributions in binary 
 * form must reproduce the above copyright notice, this list of conditions and 
 * the following disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
#include <omega.h>

#include "PortholeService.h"
#include "PortholeGUI.h"

#define PORTHOLE_VERSION "1.1"

using namespace omega;

PortholeService* sServiceInstance = NULL;
    
///////////////////////////////////////////////////////////////////////////////
PortholeService* getService()
{
    return sServiceInstance;
}

///////////////////////////////////////////////////////////////////////////////
bool initialize(int port = 4080)
{
    // The service gets created only on the master node.
    if(SystemManager::instance()->isMaster() && !sServiceInstance)
    {
        sServiceInstance = new PortholeService();
        ServiceManager* svcManager = SystemManager::instance()->getServiceManager();
        svcManager->addService(sServiceInstance);
        sServiceInstance->start(port);

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Python wrapper code.
#ifdef OMEGA_USE_PYTHON
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_FUNCTION_OVERLOADS(initializeOverloads, initialize, 0, 1)
BOOST_PYTHON_MODULE(porthole)
{
    PYAPI_REF_BASE_CLASS(PortholeService)
        PYAPI_METHOD(PortholeService, setConnectedCommand)
        PYAPI_METHOD(PortholeService, setDisconnectedCommand)
        PYAPI_METHOD(PortholeService, setCameraCreatedCommand)
        PYAPI_METHOD(PortholeService, setCameraDestroyedCommand)
        PYAPI_METHOD(PortholeService, setServerStartedCommand)
        PYAPI_METHOD(PortholeService, sendjs)
        PYAPI_METHOD(PortholeService, broadcastjs)
        PYAPI_METHOD(PortholeService, setPointerBounds)
        PYAPI_GETTER(PortholeService, getPointerBounds)
        PYAPI_METHOD(PortholeService, setPointerSpeed)
        PYAPI_METHOD(PortholeService, getPointerSpeed)
        PYAPI_METHOD(PortholeService, load)
        ;

    def("initialize", initialize, initializeOverloads());
    def("getService", getService, PYAPI_RETURN_REF);

    ofmsg(">>>>> Porthole version %1% ready", %PORTHOLE_VERSION);
}
#endif