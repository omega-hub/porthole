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
#include "HTML.h"
#include <omicron/xml/tinyxml.h>
#include <iostream>

using namespace omega;
using namespace std;

std::map<int, PortholeCamera*> PortholeClient::CamerasMap;

// define initial image quality: {0,1}
#define IMAGE_QUALITY 1

///////////////////////////////////////////////////////////////////////////////
PortholeClient::PortholeClient(PortholeService* owner, const String& cliid):
service(owner), clientId(cliid), pointerSpeed(1)
{
    ofmsg("Porthole GUI client connected: %1%", %clientId);
    
    this->sessionCamera = NULL;

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
/* 
*	Camera creation function
*/
void PortholeClient::createCustomCamera(int width, int height, uint cameraMask)
{
    // Get the global engine
    Engine* myEngine = Engine::instance();

    // Initialize camera size
    // Workaround. This avoid a canvas drawing bug
    // Round down width to a multiple of 4.
    //int width = (int)( widthPercent * IMAGE_QUALITY * device->deviceWidth / 4 ) * 4;
    //int height = (int)( heightPercent * IMAGE_QUALITY * device->deviceHeight / 4 ) * 4;
    //cout << "Width: " << width  << " - height: " << height << endl;

    uint flags = Camera::DrawScene | Camera::DrawOverlay;
    Camera* camera = myEngine->createCamera(flags);
    camera->setMask(cameraMask);
    // Set the camera name using the client id and camera id
    String cameraName = ostr("%1%-%2%", %clientId %camera->getCameraId());
    camera->setName(cameraName);
    service->notifyCameraCreated(camera);
    
    oflog(Verbose, "[PortholeClient::createCustomCamera]: %1% <%2%x%3%>", %cameraName %width %height);

    DisplayTileConfig* dtc = camera->getCustomTileConfig();
    // Setup projection
    dtc->enabled = true;
    dtc->setPixelSize(width, height);
    // Setup a default projection

    float as = (float)width / height;
    float base = 1.0f;

    Camera* defaultCamera = myEngine->getDefaultCamera();
    Vector3f ho = defaultCamera->getHeadOffset();

    dtc->setCorners(
        Vector3f(-base * as, base, -2) + ho,
        Vector3f(-base * as, -base, -2) + ho,
        Vector3f(base * as, -base, -2) + ho);

    // Initialize the camera position to be the same as the main camera.
    camera->setPosition(defaultCamera->getPosition());
    camera->setHeadOffset(defaultCamera->getHeadOffset());

    // Create the Porthole camera object, storing the new camera and other
    // related objects.
    Ref<PortholeCamera> pc = new PortholeCamera();
    pc->id = camera->getCameraId();
    pc->camera = camera;
    //pc->camera = defaultCamera;
    pc->canvasWidth = width;
    pc->canvasHeight = height;
    pc->size = IMAGE_QUALITY;

    pc->fpsStat = Stat::create(ostr("Stream %1% fps", %pc->id), StatsManager::Fps);
    pc->streamStat = Stat::create(ostr("Stream %1% bw(Kbps)", %pc->id), StatsManager::Count1);
    pc->fpsStat->addSample(pc->targetFps);
    pc->streamStat->addSample(0);

    // Save new camera
    PortholeClient::CamerasMap[pc->id] = pc; // Global map
    this->sessionCamera = pc; // Session

    // If low latency hardware encoding is available, use that. Otherwise, 
    // fall back to the old JPEG encoding & streaming.
    if(service->isHardwareEncoderEnabled())
    {
        if(pc->camera->getListener() != NULL)
        {
            pc->streamer = (CameraStreamer*)pc->camera->getListener();
        }
        else
        {
            pc->streamer = new CameraStreamer("llenc");
            pc->camera->addListener(pc->streamer);
        }
    } 
    else
    {
        PixelData* sessionCanvas = new PixelData(PixelData::FormatRgb,  width,  height);
        pc->camera->getOutput(0)->setReadbackTarget(sessionCanvas);
        pc->camera->getOutput(0)->setEnabled(true);

        pc->canvas = sessionCanvas;
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeClient::modCustomCamera(int width, int height)
{ 
    // Retrieve the camera to be modified
    PortholeCamera* portholeCamera = this->sessionCamera;

    Camera* sessionCamera = portholeCamera->camera;

    // Get the global engine
    Engine* myEngine = Engine::instance();

    // Initialize camera size
    // Workaround. This avoid a canvas drawing bug
    // Round down width to a multiple of 4.
    //int width = (int)(widthPercent * portholeCamera->size * device->deviceWidth / 4) * 4;
    //int height = (int)(heightPercent * portholeCamera->size * device->deviceHeight / 4) * 4;

    oflog(Verbose, "[PortholeClient::modCustomCamera]: %1% (%2%x%3%)", 
        %sessionCamera->getName() %width %height);

    DisplayTileConfig* dtc = sessionCamera->getCustomTileConfig();
    // Setup projection
    dtc->enabled = true;
    dtc->setPixelSize(width, height);

    float as = (float)width / height;
    float base = 1.0f;

    Camera* defaultCamera = myEngine->getDefaultCamera();
    Vector3f ho = defaultCamera->getHeadOffset();

    dtc->setCorners(
        Vector3f(-base * as, base, -2) + ho,
        Vector3f(-base * as, -base, -2) + ho,
        Vector3f(base * as, -base, -2) + ho);
        
    if(!service->isHardwareEncoderEnabled() || portholeCamera->streamer == NULL)
    {
        // Set new camera target
        portholeCamera->canvas = new PixelData(PixelData::FormatRgb, width, height);
        sessionCamera->getOutput(0)->setReadbackTarget(portholeCamera->canvas);
        sessionCamera->getOutput(0)->setEnabled(true);
    }

    portholeCamera->canvasWidth = width;
    portholeCamera->canvasHeight = height;
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
