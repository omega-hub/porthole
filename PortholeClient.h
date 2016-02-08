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
#include "PortholeFunctionsBinder.h"

using namespace omega;
using namespace omicron;
using namespace std;

// Xml Document
//static omega::xml::TiXmlDocument* xmlDoc;

class PortholeService;

///////////////////////////////////////////////////////////////////////////////
//! Implements the HTML GUI Manager for Porthole Service
class PortholeClient: public ReferenceType
{
public:

    // Constructor
    PortholeClient(PortholeService* owner, const String& clientId);

    // Destructor
    ~PortholeClient();

    const String& getId() { return clientId; }

    // Return an object that contains the device specifications
    PortholeDevice* getDevice() { return device; }

    bool isCameraReadyToStream() 
    { return (sessionCamera != NULL && sessionCamera->camera->isEnabled()); } 

    // Get Porthole camera object for this client connected
    PortholeCamera* getSessionCamera() { return sessionCamera; } 
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

    // Mod the camera with id cameraId 
    // size: the ratio of camera: 1.0 is full size
    void modCustomCamera(int width, int height);

    //! Global map of cameras by id
    static std::map<int, PortholeCamera*> CamerasMap;

    const Vector2f& getPointerPosition() { return pointerPosition; }
    void updatePointerPosition(int dx, int dy);

    //! Resets the cache of preprocessed javascript / html files, forcing
    //! a reload the next time they are requested from a client.
    void clearCache();

private:
    PortholeService* service;

    // The device for which an interface will be created
    Ref<PortholeDevice> device;

    // The camera of this session
    Ref<PortholeCamera> sessionCamera;

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
};

#endif