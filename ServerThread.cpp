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
#include "ServerThread.h"
#include "PortholeService.h"
#include "vjson/json.h"
#include "websockets/private-libwebsockets.h"
#include "websockets/extension-deflate-stream.h"
#include "base64.h"


using namespace omega;
using namespace omicron;

// User Id start contains the first id used for porthole clients. We start with
// a pretty high value to avoid possible conflicts with user ids from other 
// services. When porthole service is initialized, it will set this value based
// on its own id.
unsigned int ServerThread::sUserIdStart = 1000;
// Global variables, yay!
unsigned int ServerThread::sUserIdCounter = 0;

////////////////////////////////////////////////////////////////////////////////
/* list of supported protocols and callbacks */
struct libwebsocket_protocols protocols[] = 
{
    { "http-only", ServerThread::callbackHttp, 0 },
    { "porthole_websocket", ServerThread::callbackWebsocket, sizeof(per_session_data) },
    { NULL, NULL, 0	}
};

PortholeService* ServerThread::service = NULL;

///////////////////////////////////////////////////////////////////////////////
ServerThread::ServerThread(PortholeService* owner, const String& defaultPage) :
use_ssl(0), opts(0), n(0)
{
    service = owner;
    // Set DATA FOLDER PATH
    string fullPath;
    DataManager::findFile(defaultPage, fullPath);

    if(fullPath == "")
    {
        ofwarn("Porthole: could not find default page %1%, using res/index.html", %defaultPage);
        fullPath = "porthole / res / index.html";
    }

    sWebserverDataRoot = fullPath.substr(0,fullPath.find_last_of("/\\"));
    sWebserverDefaultPage = fullPath.substr(fullPath.find_last_of("/\\"));

    minterface="";

    if (!use_ssl)
    {
        cert_path = "";
        key_path = "";
    }
    else
    {
        cert_path = (sWebserverDataRoot+"/server.pem").c_str();
        key_path = (sWebserverDataRoot+"/server.key.pem").c_str();
    }

    sUserIdStart = owner->getServiceId() * 100;
    oflog(Verbose, "PortholeService: initial user id = %1%", %sUserIdStart);
}

///////////////////////////////////////////////////////////////////////////////
ServerThread::~ServerThread()
{
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::setPort(int portNumber)
{
    this->port = portNumber;
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::threadProc()
{
    // Buffer used to send/receive data using websockets
    unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 1024 +
                          LWS_SEND_BUFFER_POST_PADDING];

    omsg("Porthole server thread started");
    
    // Initialize the websockets context
    context = libwebsocket_create_context(port, minterface, protocols,
                libwebsocket_internal_extensions,
                cert_path, key_path, -1, -1, opts);

    if (context == NULL) {
        fprintf(stderr, ">> !! libwebsocket init failed\n");
        // TODO Delete service from global service manager
        return;
    }

    // A dumb buffer to keep socket alive
    buf[LWS_SEND_BUFFER_PRE_PADDING] = 'x';

    service->notifyServerStarted();

    //fprintf(stderr, " Using no-fork service loop\n");
    oldus = 0;
    n = 0;
    while (/*n >= 0 &&*/ !SystemManager::instance()->isExitRequested()) 
    {
        struct timeval tv;

        gettimeofday(&tv, NULL);

        /*
         * This server does not fork or create a thread for
         * websocket service, it all runs in this single loop.  So,
         * we have to give the websockets an opportunity to service
         * "manually".
         *
         * If no socket is needing service, the call below returns
         * immediately and quickly.  Negative return means we are
         * in process of closing
         */
        n = libwebsocket_service(context, 50);
        // Even if we get n < 0 we attempt to keep running. -1 seems to return
        // if we start porthole earlier in an omegalib app, but does not seem to
        // cause any problem..
        if(n < 0)
        {
            oflog(Verbose, "Websocket libwebsocket_service returned: %1%", %n);
        }
    }

    // Destroy context when main loop ends
    libwebsocket_context_destroy(context);
}
