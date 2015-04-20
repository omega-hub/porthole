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
#include "websockets/private-libwebsockets.h"
#include "websockets/extension-deflate-stream.h"
#include "base64.h"

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace omega;
using namespace omicron;

///////////////////////////////////////////////////////////////////////////////
#define MSG_EVENT_TYPE "event_type"

#define MSG_EVENT_SPEC "device_spec"
#define MSG_WIDTH "width"
#define MSG_HEIGHT "height"
#define MSG_ORIENTATION "orientation"
#define MSG_FIRST_TIME "first_time"

#define MSG_EVENT_TAP "tap"
#define MSG_EVENT_MOUSEUP "mouseup"
#define MSG_EVENT_MOUSEDOWN "mousedown"
#define MSG_EVENT_KEYUP "keyup"
#define MSG_EVENT_KEYDOWN "keydown"
#define MSG_EVENT_DRAG "drag"
#define MSG_X "x"
#define MSG_Y "y"

#define MSG_CAMERA_ID "camera_id"

#define MSG_EVENT_PINCH "pinch"
#define MSG_DELTA_SCALE "scale"
#define MSG_DELTA_ROTATION "rotation"

#define MSG_EVENT_INPUT "input"
#define MSG_INPUT_FUNCTION "function"
#define MSG_INPUT_BUTTON "button"
#define MSG_INPUT_KEY "key"
#define MSG_INPUT_VALUE "value"

#define MSG_EVENT_FPS_ADJUST "fps_adjust"
#define MSG_FPS "fps"

///////////////////////////////////////////////////////////////////////////////
// This is the function that handle the event received by the client,
// that has the per_session_data structure associated
void ServerThread::parseJsonMessage(json_value *value, per_session_data* data, recv_message* message)
{
    switch(value->type)
    {
    case JSON_NULL:
        //printf("null\n");
        break;
    case JSON_OBJECT:
    case JSON_ARRAY:
        for (json_value *it = value->first_child; it; it = it->next_sibling)
        {
            parseJsonMessage(it, data, message);
        }
        break;
    case JSON_STRING:

        // Event type
        if (strcmp(value->name, MSG_EVENT_TYPE) == 0)
           message->event_type = value->string_value;

        // Orientation
        else if (strcmp(value->name, MSG_ORIENTATION) == 0)
            message->orientation = value->string_value;

        // Input Javascript function name
        else if (strcmp(value->name, MSG_INPUT_FUNCTION) == 0)
            message->jsFunction = value->string_value;

        // HTML tag value (ex: the value of a slider, a text input, ecc)
        else if (strcmp(value->name, MSG_INPUT_VALUE) == 0)
            message->value = value->string_value;

        // All of the other tags are added as message arguments
        else
            message->args[value->name] = value->string_value;

        break;
    case JSON_INT:
        // Width and Height
        if (strcmp(value->name, MSG_WIDTH) == 0)
            message->width = value->int_value;
        else if (strcmp(value->name, MSG_HEIGHT) == 0)
            message->height = value->int_value;

        // Camera id
        else if (strcmp(value->name, MSG_CAMERA_ID) == 0)
            message->cameraId = value->int_value;

        // Input mouse button value (0|1|2)
        else if(strcmp(value->name, MSG_INPUT_BUTTON) == 0)
            message->button = value->int_value;

        // Input key value
        else if(strcmp(value->name, MSG_INPUT_KEY) == 0)
            message->key = (char)value->int_value;

        // Is the first time we receive the device specification?
        else if (strcmp(value->name, MSG_FIRST_TIME) == 0)
            message->firstTime = (value->int_value == 1 )? true : false;

        else if (strcmp(value->name, MSG_X) == 0)
            message->x = value->int_value;
        else if (strcmp(value->name, MSG_Y) == 0)
            message->y = value->int_value;
        break;
    case JSON_FLOAT:
        // Scale and Rotation
        if (strcmp(value->name, MSG_DELTA_SCALE) == 0)
            message->scale = value->float_value;
        else if (strcmp(value->name, MSG_DELTA_ROTATION) == 0)
            message->deltaRotation = value->float_value;

        // Camera mod
        else if (strcmp(value->name, MSG_FPS) == 0)
            message->fps = value->float_value;

        break;
    case JSON_BOOL:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::handleJsonMessage(per_session_data* data, recv_message* message, 
        libwebsocket_context* context, libwebsocket *wsi)
{
    if(strcmp(message->event_type.c_str(), MSG_EVENT_DRAG) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        int id = message->cameraId;
        // When scale is 1, the position is differential
        if(message->scale == 1)
        {
            data->guiManager->updatePointerPosition(message->x, message->y);
            const Vector2f& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Move, id, pt[0], pt[1], 0, data->userId);
        }
        else
        {
            svc->postPointerEvent(Event::Move, id, message->x, message->y, 0, data->userId);
        }
    }

    if (strcmp(message->event_type.c_str(),MSG_EVENT_TAP)==0)
    {
        PortholeService* svc = data->guiManager->getService();
        int id = message->cameraId; 
        // When scale is 1, the position is differential
        if(message->scale == 1)
        {
            data->guiManager->updatePointerPosition(message->x, message->y);
            const Vector2f& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Move, id, pt[0], pt[1], Event::Left, data->userId);
        }
        else 
        {
            svc->postPointerEvent(Event::Click, id, message->x, message->y, Event::Left, data->userId);
        }
    }

    else if(strcmp(message->event_type.c_str(), MSG_EVENT_MOUSEUP) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        int id = message->cameraId;
        // Process button flag 
        // (see http://www.quirksmode.org/js/events_properties.html#button)
        uint flags = Event::Left;
        if(message->button == 1) flags = Event::Middle;
        if(message->button == 2) flags = Event::Right;

        // When scale is 1, the position is differential
        if(message->scale == 1)
        {
            data->guiManager->updatePointerPosition(message->x, message->y);
            const Vector2f& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Up, id, pt[0], pt[1], flags, data->userId);
        }
        else
        {
            svc->postPointerEvent(Event::Up, id, message->x, message->y, flags, data->userId);
        }
    }

    else if(strcmp(message->event_type.c_str(), MSG_EVENT_MOUSEDOWN) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        int id = message->cameraId;
        // Process button flag 
        // (see http://www.quirksmode.org/js/events_properties.html#button)
        uint flags = Event::Left;
        if(message->button == 1) flags = Event::Middle; 
        if(message->button == 2) flags = Event::Right;

        // When scale is 1, the position is differential
        if(message->scale == 1)
        {
            data->guiManager->updatePointerPosition(message->x, message->y);
            const Vector2f& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Down, id, pt[0], pt[1], flags, data->userId);
        }
        else
        {
            svc->postPointerEvent(Event::Down, id, message->x, message->y, flags, data->userId);
        }
    }

    else if(strcmp(message->event_type.c_str(), MSG_EVENT_KEYUP) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        // HACK: for some reason, Key UP on browser returns UPPERCASE 
        char key = tolower(message->key);
        //ofmsg("Key up %1%", %key);
        uint flags = 0;
        svc->postKeyEvent(Event::Up, key, flags, data->userId);
    }

    else if(strcmp(message->event_type.c_str(), MSG_EVENT_KEYDOWN) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        char key = message->key;
        uint flags = 0;
        //ofmsg("Key down %1%", %key);
        svc->postKeyEvent(Event::Down, key, flags, data->userId);
    }

    // First message received is a device spec message
    else if (strcmp(message->event_type.c_str(),MSG_EVENT_SPEC)==0)
    {
        // Save device specification
        // NOTE: message->value contains the interface ID
        data->guiManager->setDeviceSpecifications(message->width,message->height,message->orientation, message->value);

        // Send the Html elements to the browser based on device specification
        sendHtmlElements(message->firstTime,data,context,wsi);

        // Ask for the first available slot on this channel
        libwebsocket_callback_on_writable(context, wsi);
    }

    // Modify the camera size if FPS in client is too low
    else if (strcmp(message->event_type.c_str(),MSG_EVENT_FPS_ADJUST)==0)
    {
        PortholeCamera* pc = data->guiManager->getSessionCamera();
        pc->targetFps = (pc->targetFps + (int)(message->fps)) / 2;
        if(pc->targetFps < 1) pc->targetFps = 1;
        if(pc->targetFps < 40) pc->targetFps++;

        //ofmsg("Adjusting fps to %1%", % pc->targetFps);
        pc->fpsStat->addSample(pc->targetFps);

        PortholeService* svc = data->guiManager->getService();
        
        if(svc->isDynamicStreamQualityEnabled())
        {
            if(pc->targetFps < pc->lowFps && pc->size > 0.5f)
            {
                if(pc->targetFps <= pc->lowFps / 2) pc->size = 0.2f;
                else pc->size = 0.5f;
                // Everytime we decrease the quality, increase high fps requirement.
                pc->highFps+= 5;
                sendHtmlElements(false, data, context, wsi);
            }
            else if(pc->targetFps > pc->highFps && pc->size < 1.0f)
            {
                pc->size = 1.0f;
                sendHtmlElements(false, data, context, wsi);
            }
        }

#ifdef llenc_ENABLED
        if(pc->streamer != NULL) pc->streamer->setTargetFps(pc->targetFps);
#endif
    }

    // Javascript functions bind
    else if(strcmp(message->event_type.c_str(),MSG_EVENT_INPUT)==0){
        // Create event
        PortholeEvent ev(data->guiManager->getId());
        ev.sessionCamera = data->guiManager->getSessionCamera();
        ev.mouseButton = message->button;
        ev.value = message->value;

        // If this message is generated by a phCall function (that is, a
        // javascript porthole API call to invoke a command on the server)
        // Just read the jsFunction value and queue it in the script interpreter.
        // Otherwise, this input event has been generated by an html event and we
        // let the porthole function binder process it.
        // Similarly, if message is generated by a phJSCall, send it to all other
        // connected clients.
        // This mechanism is more complex than needed, but works this way because
        // originally only html events could be 'attached' to python or C++ calls.
        // If we wanted to call a server function from within a javascript function,
        // that was not possible, until phCall was introduced.
        if(ev.value == "phCall")
        {
            PythonInterpreter* pi = SystemManager::instance()->getScriptInterpreter();
            String call = StringUtils::replaceAll(message->jsFunction, "%client_id%", "\"" + ev.clientId + "\"");
            pi->queueCommand(call);
        }
        else if(ev.value == "phJSCall")
        {
            String call = StringUtils::replaceAll(message->jsFunction, "%client_id%", "\"" + ev.clientId + "\"");
            call = StringUtils::replaceAll(call, "\"", "\\\"");
            ServerThread::service->broadcastjs(call, ev.clientId);
        }
        else
        {
            ev.key = message->key;
            ev.htmlEvent = message->jsFunction;
            ev.args = message->args;
            // Call the function or python script
            PortholeGUI::getPortholeFunctionsBinder()->callFunction(message->jsFunction, ev);
        }
    }

}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::sendHtmlElements(bool firstTime, per_session_data* data,
        libwebsocket_context *context, libwebsocket *wsi)
{
        string deviceBasedHtml = data->guiManager->create(firstTime);

        string toSend = "{\"event_type\" : \"html_elements\", \"innerHTML\" : \"";
        toSend.append(omicron::StringUtils::replaceAll(deviceBasedHtml.c_str(),"\"","\\\""));
        toSend.append("\"}");

        // Send the html elements
        int n;
        unsigned char* buf;
        buf = new unsigned char[LWS_SEND_BUFFER_PRE_PADDING + toSend.length() + LWS_SEND_BUFFER_POST_PADDING];
        unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
        n = sprintf((char *)p, "%s",toSend.c_str());
        n = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);

        // Free buffer
        delete[] buf;

        return;
}