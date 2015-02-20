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
#include "PortholeService.h"
#include "ServerThread.h"

using namespace omega;
using namespace omicron;

///////////////////////////////////////////////////////////////////////////////
PortholeService::PortholeService():
myPointerBounds(Vector2i::Zero()), myPointerSpeed(1), myBinder(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
PortholeService::~PortholeService()
{
    portholeServer->stop();
    delete portholeServer;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::start(int port, const String& defaultPage)
{
    myBinder = new PortholeFunctionsBinder();
    myBinder->clear();

    PortholeGUI::setPortholeFunctionsBinder(myBinder);

    portholeServer = new ServerThread(this, defaultPage);
    portholeServer->setPort(port);
    portholeServer->start();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::load(const String& interfaceFile)
{
    if(myBinder == NULL)
    {
        owarn("PortholeService::load called before PortholeService::start");
    }
    else
    {
        myBinder->clear();

        String interfaceFilePath;
        if(DataManager::findFile(interfaceFile, interfaceFilePath))
        {
            PortholeGUI::parseXmlFile(interfaceFilePath.c_str());
        }
        else
        {
            ofwarn("PortholeService::load: could not find interface file %1%", %interfaceFile);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyServerStarted()
{
    if(!myServerStartedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        i->queueCommand(myServerStartedCommand);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyConnected(const String& id)
{
    if(!myConnectedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myConnectedCommand, "%id%", id);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyDisconnected(const String& id)
{
    if(!myDisconnectedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myConnectedCommand, "%id%", id);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyCameraCreated(Camera* cam)
{
    if(!myCameraCreatedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String idstr = ostr("%1%", %cam->getCameraId());
        String cmd = StringUtils::replaceAll(myCameraCreatedCommand, "%id%", idstr);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyCameraDestroyed(Camera* cam)
{
    if(!myCameraDestroyedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String idstr = ostr("%1%", %cam->getCameraId());
        String cmd = StringUtils::replaceAll(myCameraDestroyedCommand, "%id%", idstr);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::postPointerEvent(Event::Type type, int sourceId, float x, float y, uint flags, unsigned int userId)
{
    lockEvents();
    Event* evt = writeHead();
    evt->reset(type, Service::Pointer, sourceId, getServiceId(), userId);
    evt->setPosition(x, y);
    evt->setFlags(flags);
    unlockEvents();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::postKeyEvent(Event::Type type, char key, uint flags, unsigned int userId)
{
    lockEvents();
    Event* evt = writeHead();
    evt->reset(type, Service::Keyboard, key, getServiceId(), userId);
    evt->setFlags(flags);
    unlockEvents();
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI* PortholeService::createClient(const String& name)
{
    PortholeGUI* cli = new PortholeGUI(this, name);
    myClients.push_back(cli);
    return cli;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::destroyClient(PortholeGUI* gui)
{
    oassert(gui != NULL);
    myClients.remove(gui);
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI* PortholeService::findClient(const String& name)
{
    foreach(PortholeGUI* cli, myClients)
    {
        if(cli->getId() == name) return cli;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::sendjs(const String& js, const String& destination)
{
    foreach(PortholeGUI* cli, myClients)
    {
        if(cli->getId() == destination)
        {
            cli->calljs(js);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::broadcastjs(const String& js, const String& origin)
{
    if(origin.empty())
    {
        foreach(PortholeGUI* cli, myClients)
        {
            cli->calljs(js);
        }
    }
    else
    {
        foreach(PortholeGUI* cli, myClients)
        {
            if(cli->getId() != origin) cli->calljs(js);
        }
    }
}
