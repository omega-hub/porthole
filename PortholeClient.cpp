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
#include "PortholeClient.h"
#include "PortholeService.h"
#include "PortholeCamera.h"
#include "HTML.h"
#include <omicron/xml/tinyxml.h>
#include <iostream>
#include "websockets/private-libwebsockets.h"

using namespace omega;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
PortholeClient::PortholeClient(PortholeService* owner, const String& cliid, libwebsocket* wsi) :
service(owner), clientId(cliid), pointerSpeed(1), mySocket(wsi), mySendBuffer(NULL), mySendBufferSize(0)
{
    ofmsg("Porthole GUI client connected: %1%", %clientId);
    
    // Get canvas size from service first. If canvas size is (0,0), read it 
    // from display config. Having a canvas size different from display config
    // ne is useful for graphical servers that want to send web events to other 
    // clients that may have a canvas size different than the server window.
    canvasSize = service->getPointerBounds();
    pointerSpeed = service->getPointerSpeed();
    
    if(canvasSize == Vector2i::Zero())
    {
        // Get the canvas size, used to convert differential mouse coords into 
        // absolute ones. If no display system is available (i.e. for headless
        // configs), set a 1x1 canvas size (will return normalized coordnates).
        DisplaySystem* ds = SystemManager::instance()->getDisplaySystem();
        if(ds != NULL)
        {
            canvasSize = ds->getDisplayConfig().getCanvasRect().size();
            normalizedPointerPosition = false;
            // Sort of hack: if canvas size is (1,1), it means we are doing headless server rendering.
            // So, enable pointer position normalization and set a reasonable canvas size here.
            if((canvasSize[0] == 0 && canvasSize[1] == 0) || (canvasSize[0] == 1 && canvasSize[1] == 1))
            {
                canvasSize = Vector2i(1920, 1080);
                //normalizedPointerPosition = true;
            }
        }
        else
        {
            canvasSize = Vector2i(1920, 1080);
            normalizedPointerPosition = true;
        }
    }
    pointerPosition = Vector2f::Zero();
}

///////////////////////////////////////////////////////////////////////////////
PortholeClient::~PortholeClient()
{
    ofmsg("Porthole GUI client disconnected: %1%", %clientId);
}

///////////////////////////////////////////////////////////////////////////////
void PortholeClient::updatePointerPosition(int dx, int dy)
{
    dx *= pointerSpeed;
    dy *= pointerSpeed;
    if(normalizedPointerPosition)
    {
        float fdx = dx;
        float fdy = dy;
        fdx /= canvasSize[0];
        fdy /= canvasSize[1];
        pointerPosition[0] += dx;
        pointerPosition[1] += dy;
        if(pointerPosition[0] < 0) pointerPosition[0] = 0;
        if(pointerPosition[1] < 0) pointerPosition[1] = 0;
        if(pointerPosition[0] >= 1) pointerPosition[0] = 1;
        if(pointerPosition[1] >= 1) pointerPosition[1] = 1;
    }
    else
    {
        pointerPosition[0] += dx;
        pointerPosition[1] += dy;
        if(pointerPosition[0] < 0) pointerPosition[0] = 0;
        if(pointerPosition[1] < 0) pointerPosition[1] = 0;
        if(pointerPosition[0] >= canvasSize[0]) pointerPosition[0] = canvasSize[0] - 1;
        if(pointerPosition[1] >= canvasSize[1]) pointerPosition[1] = canvasSize[1] - 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
String PortholeClient::flushJavascriptQueueToJson()
{
    String json = "{\"event_type\":\"javascript\",\"commands\":[";
    // TODO: escape double quotes in js command
    javascriptQueueLock.lock();
    int l = 0;
    foreach(String cmd, javascriptQueue)
    {
        json.append("{\"js\": \"" + cmd + "\"}");
        l++;
        if(l < javascriptQueue.size()) json.append(",");
    }
    javascriptQueue.clear();
    javascriptQueueLock.unlock();

    json.append("]}");
    return json;
}

///////////////////////////////////////////////////////////////////////////////
bool PortholeClient::isJavascriptQueued()
{
    javascriptQueueLock.lock();
    bool queued = !javascriptQueue.empty();
    javascriptQueueLock.unlock();
    return queued;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeClient::calljs(const String& command)
{
    javascriptQueueLock.lock();
    javascriptQueue.push_back(command);
    javascriptQueueLock.unlock();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeClient::addCamera(PortholeCamera* cam)
{
    myCameras.push_back(cam);
}

///////////////////////////////////////////////////////////////////////////////
void PortholeClient::removeCamera(PortholeCamera* cam)
{
    myCameras.remove(cam);
}

///////////////////////////////////////////////////////////////////////////////
void PortholeClient::sendClientData()
{
    foreach(PortholeCamera* c, myCameras)
    {
        // If the camera has not been initialized yet for this client, do it now.
        if(myCameraInitialized.find(c->getElementId()) == myCameraInitialized.end())
        {
            myCameraInitialized[c->getElementId()] = true;
            String json = ostr("{'event_type':'camera_init', "
                "'element_id':'%1%', 'encoder_type':'%2%'}",
                %c->getElementId() % c->getStreamer()->getEncoderName());
            send(json);
        }
        c->sendStream(this);
    }
    if(isJavascriptQueued()) send(flushJavascriptQueueToJson());
}

///////////////////////////////////////////////////////////////////////////////
int PortholeClient::send(const void* data, size_t size, WebsocketSendType sendType)
{
    size_t totalSize = size + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING;
    if(totalSize > mySendBufferSize)
    {
        free(mySendBuffer);
        mySendBuffer = (char*)malloc(totalSize);
        mySendBufferSize = totalSize;
    }

    void* p = &mySendBuffer[LWS_SEND_BUFFER_PRE_PADDING];
    memcpy(p, data, size);
    
    if(sendType == WsSendBinary) return libwebsocket_write(mySocket, (unsigned char*)p, size, LWS_WRITE_BINARY);
    return libwebsocket_write(mySocket, (unsigned char*)p, size, LWS_WRITE_TEXT);
}

///////////////////////////////////////////////////////////////////////////////
int PortholeClient::send(const String& text)
{
    return send(text.c_str(), text.length(), WsSendText);
}
