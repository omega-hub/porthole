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
#ifndef __PORTHOLE_PAGE_H__
#define __PORTHOLE_PAGE_H__

#include <omega.h>
#include "PortholeFunctionsBinder.h"

using namespace omega;
using namespace omicron;
using namespace std;

// Xml Document
static omega::xml::TiXmlDocument* xmlDoc;

class PortholeService;

///////////////////////////////////////////////////////////////////////////////
//! Implements the HTML GUI Manager for Porthole Service
class PortholePage: public ReferenceType
{
public:

    // Constructor
    PortholePage(PortholeService* owner, const String& clientId);

    // Destructor
    ~PortholePage();

    const String& getId() { return clientId; }

    // Create the device specifc html interface
    String create(const String& filename);

    PortholeService* getService()
    { return service; }

    //! @internal Return true if javascript commands are queued to be sent
    //! to the client.
    bool isJavascriptQueued()
    { 
        javascriptQueueLock.lock();
        bool queued = !javascriptQueue.empty();
        javascriptQueueLock.unlock();
        return queued;
    }

    //! Queues a javascript command to be invoked on the client.
    void calljs(const String& command)
    { 
        javascriptQueueLock.lock();
        javascriptQueue.push_back(command);
        javascriptQueueLock.unlock();
    }

    //! Creates a JSON message containing all the queued javascript callbacks
    //! then flushes the queue.
    String flushJavascriptQueueToJson();

    // Functions binder getter and setter
    static PortholeFunctionsBinder* getPortholeFunctionsBinder() { return functionsBinder; }
    static void setPortholeFunctionsBinder(PortholeFunctionsBinder* binder) { functionsBinder = binder;  functionsBinder->scriptNumber=0;}

private:
    void parseNode(omega::xml::TiXmlElement* node);

private:
    PortholeService* service;
    String clientId;

    // Queue of javascript calls from the server to the client
    List<String> javascriptQueue;
    Lock javascriptQueueLock;

    // Functions binder object
    static PortholeFunctionsBinder* functionsBinder;
};

#endif