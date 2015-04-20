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

#include <iostream>
#include <iomanip>
#include <fstream>

// H264 hardware encoder support
#ifdef llenc_ENABLED
#include "llenc/Encoder.h"
#endif

using namespace omega;
using namespace omicron;

// User Id start contains the first id used for porthole clients. We start with
// a pretty high value to avoid possible conflicts with user ids from other 
// services. When porthole service is initialized, it will set this value based
// on its own id.
unsigned int sUserIdStart = 1000;
// Global variables, yay!
unsigned int sUserIdCounter = 0;

///////////////////////////////////////////////////////////////////////////////
void ServerThread::sendFile(libwebsocket *wsi, const String& filename)
{
    String filePath = filename;
    // Default file request points to index.html
    if(filePath == "/") filePath = sWebserverDefaultPage;
    // Always add . to absolute paths starting with /
    if(filePath[0] == '/') filePath = "." + filePath;

    // If we can find a file using the standard omegalib search,serve that one.
    // Otherwise serve files from the webroot dir (the porthole dir where 
    // index.html is)
    String fullFilePath;
    if(!DataManager::findFile(filePath, fullFilePath))
    {
        fullFilePath = sWebserverDataRoot + "/" + filePath;
    }

    // Set mime type
    String mime = "text/plain";
    if(StringUtils::endsWith(fullFilePath, ".ico")) mime = "image/x-icon";
    else if(StringUtils::endsWith(fullFilePath, ".js")) mime = "application/javascript";
    else if(StringUtils::endsWith(fullFilePath, ".png")) mime = "image/png";
    else if(StringUtils::endsWith(fullFilePath, ".jpg")) mime = "image/png";
    else if(StringUtils::endsWith(fullFilePath, ".jpeg")) mime = "image/jpeg";
    else if(StringUtils::endsWith(fullFilePath, ".html")) mime = "text/html";
    else if(StringUtils::endsWith(fullFilePath, ".css")) mime = "text/css";

    if(libwebsockets_serve_http_file(wsi, fullFilePath.c_str(), mime.c_str()))
    {
        ofwarn("ServerThread::sendFile: failed sending %1%", %fullFilePath);
    }
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::sendFunctionBindings(libwebsocket *wsi)
{
    // Build Content. Socket var is used to hold the JS socket object
    string content = "";

    // Python scripts
    PortholeFunctionsBinder* functionsBinder = PortholeGUI::getPortholeFunctionsBinder();
    std::map<std::string, string>::const_iterator py_it;
    for(py_it = functionsBinder->pythonFunMap.begin(); py_it != functionsBinder->pythonFunMap.end(); py_it++)
    {
        content.append("var " + functionsBinder->pythonFunIdMap[py_it->first] + "_skip_next_event = false;");
        content.append(" function ");

        Vector<String> toks = StringUtils::split(py_it->second, "%");

        content.append(py_it->first);
        content.append("{ "
            "JSONToSend = {"
            "\"event_type\": \"input\","
            "\"button\": event.button,");
        //"\"char\": getChar(event),");

        // Odd tokens are argument names "like(%in%, %this%)"
        for(int i = 1; i < toks.size(); i += 2)
        {
            // Ignore special token %value% to keep compatibility with old interfaces.
            // Ignore special token %client_id%: this will be substituted server-side
            // with the name of the client sending the call
            if(toks[i] != "value" && toks[i] != "client_id")
            {
                content.append(ostr("\"%1%\": String(%1%),", %toks[i]));
            }
        }
        // Add special token %value% to keep compatibility with old interfaces.
        content.append("\"value\": event.target.value,"
            "\"function\": \"");

        content.append(py_it->first);
        content.append("\""
            "};"
            //"if(!" + functionsBinder->pythonFunIdMap[py_it->first] + "_skip_next_event) {"
            "socket.send(JSON.stringify(JSONToSend));" +
            //"}" +
            functionsBinder->pythonFunIdMap[py_it->first] + "_skip_next_event = false;"
            "};");
    }

    // Cpp functions
    typedef void(*memberFunction)(PortholeEvent&);
    std::map<std::string, memberFunction>::const_iterator cpp_it;
    for(cpp_it = functionsBinder->cppFuncMap.begin(); cpp_it != functionsBinder->cppFuncMap.end(); cpp_it++)
    {

        content.append(" function ");
        content.append(cpp_it->first);
        content.append("{ "
            "JSONToSend = {"
            "\"event_type\": \"input\","
            "\"button\": event.button,"
            "\"char\": getChar(event),"
            "\"value\": event.target.value,"
            "\"function\": \"");
        content.append(cpp_it->first);
        content.append("\""
            "};"
            "socket.send(JSON.stringify(JSONToSend));"
            "};");
    }

    // Build Message = Header + Content
    char content_length[16];
    sprintf(content_length, "%d", content.length());
    string msg = "HTTP/1.0 200 OK\x0d\x0a"
        "Server: libwebsockets\x0d\x0a"
        "Content-Type: application/javascript\x0d\x0a"
        "Content-Length: ";
    msg.append(content_length);
    msg.append("\x0d\x0a\x0d\x0a");
    msg.append(content);

    // Send the Javascript file
    libwebsocket_write(wsi, (unsigned char*)msg.c_str(), msg.length(), LWS_WRITE_HTTP);
}


///////////////////////////////////////////////////////////////////////////////
// this protocol server (always the first one) just knows how to do HTTP
int ServerThread::callback_http(libwebsocket_context *context, libwebsocket *wsi,
    libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
    char client_name[128];
    char client_ip[128];

    switch (reason) 
    {
    case LWS_CALLBACK_HTTP:
        /* Function Binder Javascript */
        if(!strcmp((char*)in, "/porthole_functions_binder.js")) sendFunctionBindings(wsi);
        else sendFile(wsi, (char*)in);
        break;

    // On connection, we could add any filtering function Now it's: accept any
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
        libwebsockets_get_peer_addresses((int)(long)user, client_name,
                 sizeof(client_name), client_ip, sizeof(client_ip));
        oflog(Debug, "Porthole: connect from %1% (%2%)", %client_name %client_ip);
        break;

    default:
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Struct of data to be passed across the entire session
struct per_session_data 
{
    PortholeGUI* guiManager;
    unsigned int userId;
    unsigned long long oldus;
    std::string test_flag;
};

///////////////////////////////////////////////////////////////////////////////
void sendHtmlElements(bool firstTime, struct per_session_data* data, struct libwebsocket_context *context,
        struct libwebsocket *wsi)
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
struct recv_message{
    string event_type;
    float x,y;
    float scale; // This value ranges about {0,2;6}: >1 is zoom in, <1 is zoom out
    float deltaRotation;
    int width,height;
    string orientation;
    string jsFunction;
    int cameraId;
    bool firstTime;
    float fps; // Target fps
    int button;
    char key;
    string value;

    Dictionary<String, String> args;
};

///////////////////////////////////////////////////////////////////////////////
// This is the function that handle the event received by the client,
// that has the per_session_data structure associated
void parse_json_message(json_value *value, per_session_data* data, recv_message* message)
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
            parse_json_message(it, data, message);
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
void handle_message(per_session_data* data, recv_message* message, 
        struct libwebsocket_context *context, struct libwebsocket *wsi)
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
int stream_jpeg(libwebsocket_context *context, libwebsocket *wsi, per_session_data* data)
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
#ifdef llenc_ENABLED
int stream_h264(libwebsocket_context *context, libwebsocket *wsi, per_session_data* data)
{
    PortholeCamera* pc = data->guiManager->getSessionCamera();
    llenc::Encoder* e = pc->streamer->getEncoder();

    if(e == NULL)
    {
        //libwebsocket_callback_on_writable(context, wsi);
        return 0;
    }

    if(!e->dataAvailable())
    {
        //libwebsocket_callback_on_writable(context, wsi);
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
    return 0;
}
#endif

///////////////////////////////////////////////////////////////////////////////
int ServerThread::callback_websocket(struct libwebsocket_context *context,
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
                res = stream_h264(context, wsi, data);
            } else
#endif
            {
                res = stream_jpeg(context, wsi, data);
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
            parse_json_message(root, data, &message);

            // Handle message received
            handle_message(data, &message, context, wsi);
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

/////////////////////////////////////////////////////////////////////////////////////////////////
/* list of supported protocols and callbacks */
struct libwebsocket_protocols protocols[] = 
{
    /* first protocol must always be HTTP handler */
    {
        "http-only",		/* name */
        ServerThread::callback_http,		/* callback */
        0			/* per_session_data_size */
    },

    /* websocket enabled protocol */
    {
        "porthole_websocket",
        ServerThread::callback_websocket,
        sizeof(struct per_session_data),
    },

    /* End of list */
    {
        NULL, NULL, 0	
    }
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
