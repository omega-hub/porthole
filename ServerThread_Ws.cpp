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
#include <modulesConfig.h>

#include <iostream>
#include <iomanip>
#include <fstream>


using namespace omega;
using namespace omicron;

///////////////////////////////////////////////////////////////////////////////
int ServerThread::callbackWebsocket(struct libwebsocket_context *context,
            struct libwebsocket *wsi,
            enum libwebsocket_callback_reasons reason,
                           void *user, void *in, size_t len)
{
    struct per_session_data *data = (per_session_data*) user;

    switch (reason) {

    /* On connection estabilished */
    case LWS_CALLBACK_ESTABLISHED:
    {
        char cliname[1024];
        char cliip[1024];
        int fd = libwebsocket_get_socket_fd(wsi);
        libwebsockets_get_peer_addresses(fd, cliname, 1024, cliip, 1024);
        String cliName = ostr("%1%:%2%:%3%", %fd %cliip %cliname);
        service->notifyConnected(cliName);
        // Allocate gui manager
        data->client = service->createClient(cliName, wsi);
        data->userId = sUserIdStart + sUserIdCounter++;
        data->oldus = 0;

        break;
    }

    /* On socket writable from server to client */
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        data->client->sendClientData();
        libwebsocket_callback_on_writable(context, wsi);
        break;
    }

    case LWS_CALLBACK_RECEIVE:
    {
        //cout << (char *)in <<endl;
        recv_message message = {"",0,0,1,0};
        char *errorPos = 0;
        char *errorDesc = 0;
        int errorLine = 0;
        block_allocator allocator(1 << 10); // 1 KB per block
        
        // Parse json message received
        json_value *root = json_parse((char*)in, &errorPos, &errorDesc, &errorLine, &allocator);
        if (root)
        {
            // Fill message object
            parseJsonMessage(root, data, &message);

            // Handle message received
            handleJsonMessage(data, &message, context, wsi);
        }

        libwebsocket_callback_on_writable(context, wsi);
        break;
    }
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        //dump_handshake_info((struct lws_tokens *)(long)user);
        /* you could return non-zero here and kill the connection */
        libwebsocket_callback_on_writable(context, wsi);
        break;

    case LWS_CALLBACK_CLOSED:
    {
        char cliname[1024];
        char cliip[1024];
        int fd = libwebsocket_get_socket_fd(wsi);
        libwebsockets_get_peer_addresses(fd, cliname, 1024, cliip, 1024);
        String cliName = ostr("%1%:%2%:%3%", %fd %cliip %cliname);
        service->notifyDisconnected(cliName);
        // Call gui destructor
        service->destroyClient(data->client);
        libwebsocket_callback_on_writable(context, wsi);
        break;
    }

    default:
        libwebsocket_callback_on_writable(context, wsi);
        break;
    }
    return 0;
}
