/******************************************************************************
* THE OMEGA LIB PROJECT
*-----------------------------------------------------------------------------
* Copyright 2010-2014		Electronic Visualization Laboratory,
*							University of Illinois at Chicago
* Authors:
*  Daniele Donghi			d.donghi@gmail.com
*  Alessandro Febretti		febret@gmail.com
*-----------------------------------------------------------------------------
* Copyright (c) 2010-2014, Electronic Visualization Laboratory,
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
#ifndef __PORTHOLE_FUNCTIONS_BINDER__
#define __PORTHOLE_FUNCTIONS_BINDER__

#include <omegaToolkit.h>
#include "PortholeService.h"

using namespace omicron;
using namespace omega;
using namespace omegaToolkit;

///////////////////////////////////////////////////////////////////////////////
// An obj bound with a Javascript event
struct PortholeEvent
{
    PortholeEvent(const String& clid) :
        clientId(clid)
    {}

    std::string htmlEvent;
    int mouseButton;
    char key;
    const String& clientId;
    std::string value;
    PortholeCamera* sessionCamera;
    Dictionary<String, String> args;
};

///////////////////////////////////////////////////////////////////////////////
class PortholeFunctionsBinder : public ReferenceType
{
public:
    void addPythonScript(std::string script, string key, string type)
    {
        oflog(Verbose, "[porthole] added function binding %1%: %2% -> %3%", %type %key %script);
        pythonFunMap[key] = script;
        pythonFunTypeMap[key] = type;
        scriptNumber++;
    }

    void callFunction(PortholeService* svc, std::string funcName, PortholeEvent &ev)
    {
        std::map<std::string, string>::const_iterator py_it;
        py_it = pythonFunMap.find(funcName);
        if(py_it != pythonFunMap.end())
        {
            PythonInterpreter* pi = SystemManager::instance()->getScriptInterpreter();

            // Substitute special %value% token
            String pythonScript = py_it->second;

            // Substitute other argument tokens.
            typedef KeyValue<String, String> ArgItem;
            foreach(ArgItem i, ev.args)
            {
                String key = ostr("%%%1%%%", %i.getKey());
                pythonScript = omicron::StringUtils::replaceAll(pythonScript, key, i.getValue());
            }

            String call = omicron::StringUtils::replaceAll(pythonScript, "%client_id%", "\"" + ev.clientId + "\"");

            String& type = pythonFunTypeMap[funcName];
            if(type == "py")
            {
                oflog(Debug, "[Porthole::py] %1%", %call);
                pi->queueCommand(call);
            }
            else if(type == "mc")
            {
                oflog(Debug, "[Porthole::mc] %1%", %call);
                MissionControlClient* mc = SystemManager::instance()->getMissionControlClient();
                if(mc != NULL) mc->postCommand(call);
            }
            else if(type == "js")
            {
                oflog(Debug, "[Porthole::js] %1%", %call);
                svc->broadcastjs(call, ev.clientId);
            }
        }
        else
        {
            owarn("[porthole] function binding not found: " + funcName);
        }
        return;
    }

    void clear()
    {
        olog(Verbose, "[porthole] function binding reset");
        scriptNumber = 0;
        pythonFunMap.clear();
        pythonFunTypeMap.clear();
    }

    std::map<std::string, string> pythonFunMap;
    std::map<std::string, string> pythonFunTypeMap;
    int scriptNumber;
};
#endif