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
    int n;
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
        data->guiManager = service->createClient(cliName);
        data->userId = sUserIdStart + sUserIdCounter++;
        data->oldus = 0;

        break;
    }

    /* On socket writable from server to client */
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        // Check if we have stream to send: if not, pass the token
        if (data->guiManager->isCameraReadyToStream() )
        {
            int res = 0;    
#ifdef llenc_ENABLED
            if(data->guiManager->getSessionCamera()->streamer != NULL)
            {
                res = streamH264(context, wsi, data);
            } else
#endif
            {
                res = streamJpeg(context, wsi, data);
            }
            if(res != 0) return res;
        }

        // See if we have javascript callbacks that we need to send to the
        // client
        if(data->guiManager->isJavascriptQueued())
        {
            String toSend = data->guiManager->flushJavascriptQueueToJson();
            //ofmsg("send %1%", %toSend);
            // Send the base64 image
            unsigned char* buf;
            buf = new unsigned char[LWS_SEND_BUFFER_PRE_PADDING + toSend.length() + LWS_SEND_BUFFER_POST_PADDING];
            unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
            n = sprintf((char *)p, "%s",toSend.c_str());

            // WEBSOCKET WRITE
            n = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
            if (n < 0) {
                fprintf(stderr, "ERROR writing to socket");
                return 1;
            }
            // Free the buffer
            delete[] buf;
        }
        // Pass the token
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
        service->destroyClient(data->guiManager);
        libwebsocket_callback_on_writable(context, wsi);
        break;
    }

    default:
        libwebsocket_callback_on_writable(context, wsi);
        break;
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
int ServerThread::streamJpeg(libwebsocket_context *context, libwebsocket *wsi, per_session_data* data)
{
    PortholeCamera* pc = data->guiManager->getSessionCamera();
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long millisecondsSinceEpoch =
        (unsigned long long)(tv.tv_sec) * 1000 +
        (unsigned long long)(tv.tv_usec) / 1000;

    if((millisecondsSinceEpoch - data->oldus) < (1000 / pc->targetFps))
    {
        libwebsocket_callback_on_writable(context, wsi);
        return 0;
    }

    // Each time we send a frame, we also try to increase the frame rate, one
    // frame at a time (cap at 50fps max)
    //if(pc->targetFps < 50) pc->targetFps++;

    // Get the corresponding camera to be modified
    Camera* camera = pc->camera;
    PixelData* canvas = pc->canvas;

    // Get camera image as JPEG/PNG and base64 encode it.
    ByteArray* png = ImageUtils::encode(canvas, ImageUtils::FormatJpeg);
    std::string base64image = base64_encode(png->getData(), png->getSize());

    // String to be send: base64 image and camera id
    string toSend = "{\"event_type\":\"stream\",\"base64image\":\"";
    toSend.append(base64image.c_str());
    toSend.append("\",\"camera_id\":" + boost::lexical_cast<string>(pc->id) +
        "}");

    // Send the base64 image
    unsigned char* buf;
    buf = new unsigned char[LWS_SEND_BUFFER_PRE_PADDING + toSend.length() + LWS_SEND_BUFFER_POST_PADDING];
    unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
    int n = sprintf((char *)p, "%s", toSend.c_str());

    // WEBSOCKET WRITE
    n = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
    if(n < 0) {
        fprintf(stderr, "ERROR writing to socket");
        return 1;
    }

    // Free the buffer
    delete[] buf;

    // Save new timestamp
    data->oldus = millisecondsSinceEpoch;
    libwebsocket_callback_on_writable(context, wsi);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int ServerThread::streamH264(libwebsocket_context *context, libwebsocket *wsi, per_session_data* data)
{
    PortholeCamera* pc = data->guiManager->getSessionCamera();
    IEncoder* e = pc->streamer->lockEncoder();

    if(e == NULL)
    {
        //libwebsocket_callback_on_writable(context, wsi);
        pc->streamer->unlockEncoder();
        return 0;
    }

    if(!e->dataAvailable())
    {
        //libwebsocket_callback_on_writable(context, wsi);
        pc->streamer->unlockEncoder();
        return 0;
    }

    const void* bitstream;
    uint32_t size;
    if(e->lockBitstream(&bitstream, &size))
    {
        // Now send the bitstream as binary data
        unsigned char* buf = new unsigned char[LWS_SEND_BUFFER_PRE_PADDING + size + LWS_SEND_BUFFER_POST_PADDING];
        unsigned char* p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

        // Record bandwidth. 
        // Estimate bandwidth by multiplying current bitstream size by fps.
        // then convert bytes/sec to Kbps
        double bw = (double)size * (double)pc->targetFps / 128.0f;
        pc->streamStat->addSample(bw);

        memcpy(p, bitstream, size);

        //ofmsg("H264 sending %1% bytes", %size);

        // WEBSOCKET WRITE
        int n = libwebsocket_write(wsi, p, size, LWS_WRITE_BINARY);
        if(n < 0) {
            fprintf(stderr, "ERROR writing to socket");
            pc->streamer->unlockEncoder();
            return 1;
        }

        delete[] buf;


        e->unlockBitstream();
    }
    else
    {
        omsg("Bitstream lock failed");
        oexit(0);
    }

    //libwebsocket_callback_on_writable(context, wsi);
    pc->streamer->unlockEncoder();
    return 0;
}

