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

using namespace omega;
using namespace omegaToolkit;
using namespace omicron;
using namespace std;

// Xml Document
//static omega::xml::TiXmlDocument* xmlDoc;

class PortholeService;

///////////////////////////////////////////////////////////////////////////////
// A omega Camera wrapper for Porthole
struct PortholeCamera : ReferenceType
{
    int id;
    float targetFps; // The desired output fps for this camera.
    int highFps; // When the frame rate passes this fps, increase stream quality.
    int lowFps; // When the frame rate is lower than this fps, decrease stream quality.
    Camera* camera;
    Ref<PixelData> canvas;
    int canvasWidth, canvasHeight;
    float size; // 1.0 is default value = device size
    //unsigned int oldusStreamSent; // Timestamp of last stream sent via socket

    Ref<Stat> fpsStat;
    Ref<Stat> streamStat;

    // H264 hardware encoder support
    Ref<CameraStreamer> streamer;

    PortholeCamera() :
        targetFps(60),
        highFps(15),
        lowFps(5),
        camera(NULL)
    {}

    ~PortholeCamera()
    {
        if(streamer != NULL) camera->removeListener(streamer);
        if(camera != NULL) Engine::instance()->destroyCamera(camera);
    }
};


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