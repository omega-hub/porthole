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
#ifndef __PORTHOLE_CLIENT_H__
#define __PORTHOLE_CLIENT_H__

#include <omega.h>
#include <omegaToolkit.h>
#include "PortholeCamera.h"

using namespace omega;
using namespace omegaToolkit;
using namespace omicron;
using namespace std;

// Xml Document
//static omega::xml::TiXmlDocument* xmlDoc;

class PortholeService;

struct libwebsocket;

///////////////////////////////////////////////////////////////////////////////
//! Implements the HTML GUI Manager for Porthole Service
class PortholeClient: public ReferenceType
{
public:
    enum WebsocketSendType { WsSendText, WsSendBinary };

public:

    // Constructor
    PortholeClient(PortholeService* owner, const String& clientId, libwebsocket* wsi);

    // Destructor
    ~PortholeClient();

    const String& getId() { return clientId; }

    PortholeService* getService()
    { return service; }

    //! @internal Return true if javascript commands are queued to be sent
    //! to the client.
    bool isJavascriptQueued();

    //! Queues a javascript command to be invoked on the client.
    void calljs(const String& command);

    //! Creates a JSON message containing all the queued javascript callbacks
    //! then flushes the queue.
    String flushJavascriptQueueToJson();

    const Vector2f& getPointerPosition() { return pointerPosition; }
    void updatePointerPosition(int dx, int dy);

    //! Resets the cache of preprocessed javascript / html files, forcing
    //! a reload the next time they are requested from a client.
    void clearCache();

    void sendCameraStreams(libwebsocket* wsi);
    void addCamera(PortholeCamera*);
    void removeCamera(PortholeCamera*);

    void sendClientData();
    int send(const void* data, size_t size, WebsocketSendType sendType = WsSendBinary);
    int send(const String& text);

private:
    PortholeService* service;

    typedef List< Ref<PortholeCamera> > CameraList;
    CameraList myCameras;
    Dictionary<String, bool> myCameraInitialized;

    String clientId;

    // Queue of javascript calls from the server to the client
    List<String> javascriptQueue;
    Lock javascriptQueueLock;

    // Create a Porthole custom camera and a PixelData associated
    void createCustomCamera(int width, int height, uint cameraMask = 0); 

    // Global canvas width and height and current pointer position. 
    // Used to convert differential mouse positions into absolute ones.
    Vector2f pointerPosition;
    Vector2i canvasSize;
    bool normalizedPointerPosition;
    float pointerSpeed;

    size_t mySendBufferSize;
    char* mySendBuffer;
    libwebsocket* mySocket;
};

#endif