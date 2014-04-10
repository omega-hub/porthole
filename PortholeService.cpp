/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2013		Electronic Visualization Laboratory, 
 *							University of Illinois at Chicago
 * Authors:										
 *  Daniele Donghi			d.donghi@gmail.com
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2013, Electronic Visualization Laboratory,  
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
// Implementation of the HTML5 based interfaces
#include "PortholeService.h"
#include "vjson/json.h"
#include "websockets/private-libwebsockets.h"
#include "websockets/extension-deflate-stream.h"

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace omega;
using namespace omicron;

///////////////////////////////////////////////////////////////////////////////
// Base64 encode/decode functions
inline string base64_encode(unsigned char const* , unsigned int len);
inline string base64_decode(string const& s);

///////////////////////////////////////////////////////////////////////////////
enum demo_protocols {
    /* always first */
    PROTOCOL_HTTP = 0,
    PROTOCOL_WEBSOCKET,
    /* always last */
    DEMO_PROTOCOL_COUNT
};

///////////////////////////////////////////////////////////////////////////////
/*
 * May be used for filtering allowing connections by the header
 * content
 */
void ServerThread::dump_handshake_info(struct lws_tokens *lwst)
{
    //int n;
    static const char *token_names[WSI_TOKEN_COUNT] = {
        /*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
        /*[WSI_TOKEN_HOST]		=*/ "Host",
        /*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
        /*[WSI_TOKEN_KEY1]		=*/ "key 1",
        /*[WSI_TOKEN_KEY2]		=*/ "key 2",
        /*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
        /*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
        /*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
        /*[WSI_TOKEN_DRAFT]		=*/ "Draft",
        /*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

        /* new for 04 */
        /*[WSI_TOKEN_KEY]		=*/ "Key",
        /*[WSI_TOKEN_VERSION]		=*/ "Version",
        /*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

        /* new for 05 */
        /*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

        /* client receives these */
        /*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
        /*[WSI_TOKEN_NONCE]		=*/ "Nonce",
        /*[WSI_TOKEN_HTTP]		=*/ "Http",
        /*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
    };

    //TESTING FUNCTION
    /*
    for (n = 0; n < WSI_TOKEN_COUNT; n++) {
        if (lwst[n].token == NULL)
            continue;
        fprintf(stderr, "    %s = %s\n", token_names[n], lwst[n].token);
    }
    */
}

///////////////////////////////////////////////////////////////////////////////
/* this protocol server (always the first one) just knows how to do HTTP */
int ServerThread::callback_http(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
                               void *in, size_t len){
    char client_name[128];
    char client_ip[128];

    switch (reason) {
    case LWS_CALLBACK_HTTP:
        fprintf(stderr, "serving HTTP URI %s\n", (char *)in);

        /* Html page icon */
        if (in && strcmp((char*)in, "/favicon.ico") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/favicon.ico").c_str(), "image/x-icon"))
                fprintf(stderr, "Failed to send favicon\n");
            break;
        }

        else if (in && strcmp((char*)in, "/ui.geo_autocomplete.js") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/ui.geo_autocomplete.js").c_str(), "application/javascript"))
                fprintf(stderr, "Failed to send ui.geo_autocomplete.js\n");
            break;
        }

        else if (in && strcmp((char*)in, "/farbtastic.js") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/farbtastic.js").c_str(), "application/javascript"))
                fprintf(stderr, "Failed to send farbtastic.js\n");
            break;
        }

        else if (in && strcmp((char*)in, "/hammer.js") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/hammer.js").c_str(), "application/javascript"))
                fprintf(stderr, "Failed to send hammer.js\n");
            break;
        }

        else if (in && strcmp((char*)in, "/wheel.png") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/wheel.png").c_str(), "image/png"))
                fprintf(stderr, "Failed to send wheel.png\n");
            break;
        }

        else if (in && strcmp((char*)in, "/marker.png") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/marker.png").c_str(), "image/png"))
                fprintf(stderr, "Failed to send marker.png\n");
            break;
        }

        else if (in && strcmp((char*)in, "/mask.png") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/mask.png").c_str(), "image/png"))
                fprintf(stderr, "Failed to send mask.png\n");
            break;
        }

        else if (in && strcmp((char*)in, "/farbtastic.css") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/farbtastic.css").c_str(), "text/css"))
                fprintf(stderr, "Failed to send farbtastic.css\n");
            break;
        }
        else if (in && strcmp((char*)in, "/recorder.js") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/recorder.js").c_str(), "application/javascript"))
                fprintf(stderr, "Failed to send recorder.js\n");
            break;
        }
        else if (in && strcmp((char*)in, "/recorderWorker.js") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                 (DATA_PATH+"/recorderWorker.js").c_str(), "application/javascript"))
                fprintf(stderr, "Failed to send recorderWorker.js\n");
            break;
        }
        /* Porthole CSS */
        else if (in && strcmp((char*)in, "/porthole.css") == 0) {
            if (libwebsockets_serve_http_file(wsi,
                css_path.c_str(), "text/css"))
                fprintf(stderr, "Failed to send porthole.css\n");
            break;
        }

        /* Function Binder Javascript */
        else if (in && strcmp((char*)in, "/porthole_functions_binder.js") == 0) {
            
            // Build Content. Socket var is used to hold the JS socket object
            string content = "";
            
            // Python scripts
            PortholeFunctionsBinder* functionsBinder = PortholeGUI::getPortholeFunctionsBinder();
            std::map<std::string, string>::const_iterator py_it;
            for(py_it = functionsBinder->pythonFunMap.begin(); py_it != functionsBinder->pythonFunMap.end(); py_it++){
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
            for(cpp_it = functionsBinder->cppFuncMap.begin(); cpp_it != functionsBinder->cppFuncMap.end(); cpp_it++ ){

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
            sprintf (content_length, "%d", content.length());
            string msg = "HTTP/1.0 200 OK\x0d\x0a"
                            "Server: libwebsockets\x0d\x0a"
                            "Content-Type: application/javascript\x0d\x0a"
                            "Content-Length: ";
            msg.append(content_length);
            msg.append("\x0d\x0a\x0d\x0a");
            msg.append(content);

            // Send the Javascript file
            libwebsocket_write(wsi, (unsigned char*)msg.c_str(), msg.length(), LWS_WRITE_HTTP);
            
            break;
        }

        /* HTML page... when it runs it'll start websockets */
        if (libwebsockets_serve_http_file(wsi,
                  (DATA_PATH+"/index.html").c_str(), "text/html"))
            fprintf(stderr, "Failed to send HTTP file\n");
        break;

    /* On connection, we could add any filtering function
    *  Now it's: accept any
    */
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:

        libwebsockets_get_peer_addresses((int)(long)user, client_name,
                 sizeof(client_name), client_ip, sizeof(client_ip));

        fprintf(stderr, "Received network connect from %s (%s)\n",
                            client_name, client_ip);

        /* if we returned non-zero from here, we kill the connection */
        break;

    default:
        break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/* websocket protocol */

///////////////////////////////////////////////////////////////////////////////
// Struct of data to be passed across the entire session
struct per_session_data {
    PortholeGUI* guiManager;
    unsigned long long oldus;
    std::string test_flag;
};

///////////////////////////////////////////////////////////////////////////////
void sendHtmlElements(bool firstTime, struct per_session_data* data, struct libwebsocket_context *context,
        struct libwebsocket *wsi){

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
// JSON simple print
#define IDENT(n) for (int i = 0; i < n; ++i) printf("    ")
inline void print(json_value *value, int ident = 0)
{
    IDENT(ident);
    if (value->name) printf("\"%s\" = ", value->name);
    switch(value->type)
    {
    case JSON_NULL:
        printf("null\n");
        break;
    case JSON_OBJECT:
    case JSON_ARRAY:
        printf(value->type == JSON_OBJECT ? "{\n" : "[\n");
        for (json_value *it = value->first_child; it; it = it->next_sibling)
        {
            print(it, ident + 1);
        }
        IDENT(ident);
        printf(value->type == JSON_OBJECT ? "}\n" : "]\n");
        break;
    case JSON_STRING:
        printf("\"%s\"\n", value->string_value);
        break;
    case JSON_INT:
        printf("%d\n", value->int_value);
        break;
    case JSON_FLOAT:
        printf("%f\n", value->float_value);
        break;
    case JSON_BOOL:
        printf(value->int_value ? "true\n" : "false\n");
        break;
    }
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
inline void parse_json_message(json_value *value, per_session_data* data, recv_message* message)
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
inline void handle_message(per_session_data* data, recv_message* message, 
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
            const Vector2i& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Move, id, pt[0], pt[1], 0);
        }
        else
        {
            svc->postPointerEvent(Event::Move, id, message->x, message->y, 0);
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
            const Vector2i& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Move, id, pt[0], pt[1], Event::Left);
        }
        else 
        {
            svc->postPointerEvent(Event::Click, id, message->x, message->y, Event::Left);
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
            const Vector2i& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Up, id, pt[0], pt[1], flags);
        }
        else
        {
            svc->postPointerEvent(Event::Up, id, message->x, message->y, flags);
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
            const Vector2i& pt = data->guiManager->getPointerPosition();
            svc->postPointerEvent(Event::Down, id, pt[0], pt[1], flags);
        }
        else
        {
            svc->postPointerEvent(Event::Down, id, message->x, message->y, flags);
        }
    }

    else if(strcmp(message->event_type.c_str(), MSG_EVENT_KEYUP) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        char key = message->key;
        uint flags = 0;
        svc->postKeyEvent(Event::Up, key, flags);
    }

    else if(strcmp(message->event_type.c_str(), MSG_EVENT_KEYDOWN) == 0)
    {
        PortholeService* svc = data->guiManager->getService();
        char key = message->key;
        uint flags = 0;
        svc->postKeyEvent(Event::Down, key, flags);
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
        pc->targetFps = (int)(message->fps);
        if(pc->targetFps < 1) pc->targetFps = 1;
        ofmsg("Adjusting fps to %1%", % pc->targetFps);
    //	data->guiManager->modCustomCamera(message->cameraSize);
    }

    // Javascript functions bind
    else if(strcmp(message->event_type.c_str(),MSG_EVENT_INPUT)==0){
        // Create event
        PortholeEvent ev(data->guiManager->getId());
        ev.sessionCamera = data->guiManager->getSessionCamera();
        ev.mouseButton = message->button;
        ev.value = message->value;
        ev.key = message->key;
        ev.htmlEvent = message->jsFunction;
        ev.args = message->args;
        // Call the function or python script
        PortholeGUI::getPortholeFunctionsBinder()->callFunction(message->jsFunction, ev);
    }

}

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
        data->oldus = 0;

        break;
    }

    /* On socket writable from server to client */
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        // Check if we have stream to send: if not, pass the token
        if (data->guiManager->isCameraReadyToStream() )
        {
            PortholeCamera* pc = data->guiManager->getSessionCamera();
            struct timeval tv;
            gettimeofday(&tv, NULL);
            unsigned long long millisecondsSinceEpoch =
                (unsigned long long)(tv.tv_sec) * 1000 +
                (unsigned long long)(tv.tv_usec) / 1000;

            if ((millisecondsSinceEpoch - data->oldus) < (1000 / pc->targetFps)) 
            {
                libwebsocket_callback_on_writable(context, wsi);
                return 0;
            }

            // Each time we send a frame, we also try to increase the frame rate, one
            // frame at a time (cap at 50fps max)
            if(pc->targetFps < 50) pc->targetFps++;

            // Get the corresponding camera to be modified
            PortholeCamera* sessionCamera = data->guiManager->getSessionCamera();
            Camera* camera = sessionCamera->camera;
            PixelData* canvas = sessionCamera->canvas;

            // IMAGE ENCODING
            // Get camera image as JPEG/PNG and base64 encode it, because only simple strings could be sent via websockets  
            // Multithreading stuff: Lock the camera output, to make sure the pixel data we are getting 
            // is not coming from an unfinished frame.
            //camera->getOutput(0)->lock();
            ByteArray* png = ImageUtils::encode(canvas, ImageUtils::FormatJpeg);
            //FILE* pf = fopen("./test.jpg", "wb");
            //fwrite((void*)png->getData(), 1, png->getSize(), pf);
            //fclose(pf);
            //camera->getOutput(0)->unlock();
            // END IMAGE ENCODING

            // BASE64 ENCODING
            std::string base64image = base64_encode(png->getData(),png->getSize());
            // END BASE64 ENCODING

            // String to be send: base64 image and camera id
            string toSend = "{\"event_type\":\"stream\",\"base64image\":\"";
            toSend.append(base64image.c_str());
            toSend.append("\",\"camera_id\":" + boost::lexical_cast<string>(sessionCamera->id) +
                "}");

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

            // Save new timestamp
            data->oldus = millisecondsSinceEpoch;
        }

        // See if we have javascript callbacks that we need to send to the
        // client
        else if(data->guiManager->isJavascriptQueued())
        {
            String toSend = data->guiManager->flushJavascriptQueueToJson();
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

        break;
    }
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        //dump_handshake_info((struct lws_tokens *)(long)user);
        /* you could return non-zero here and kill the connection */
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
        break;
    }

    default:
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
ServerThread::ServerThread(PortholeService* owner):
use_ssl(0), opts(0), n(0)
{
    service = owner;
    // Set DATA FOLDER PATH
    string fullPath;
    DataManager::findFile("porthole/index.html", fullPath);

    DATA_PATH = fullPath.substr(0,fullPath.find_last_of("/\\"));

    minterface="";

    //setPollPriority(Service::PollLast);

    if (!use_ssl)
    {
        cert_path = "";
        key_path = "";
    }
    else
    {
        cert_path = (DATA_PATH+"/server.pem").c_str();
        key_path = (DATA_PATH+"/server.key.pem").c_str();
    }

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
void ServerThread::setFunctionsBinder(PortholeFunctionsBinder* binder)
{
    PortholeGUI::setPortholeFunctionsBinder(binder);
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::setXMLfile(char* xmlPath)
{
    PortholeGUI::parseXmlFile(xmlPath);
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::setCSSPath(char* cssPath)
{
    css_path = std::string(cssPath);
}

///////////////////////////////////////////////////////////////////////////////
void ServerThread::threadProc()
{
    // Buffer used to send/receive data using websockets
    unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 1024 +
                          LWS_SEND_BUFFER_POST_PADDING];

    cout << ">> Porthole initialization" << endl;
    
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
    while (n >= 0 && !SystemManager::instance()->isExitRequested()) 
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
    }

    // Destroy context when main loop ends
    libwebsocket_context_destroy(context);
}

///////////////////////////////////////////////////////////////////////////////
PortholeService::PortholeService()
{
}

///////////////////////////////////////////////////////////////////////////////
PortholeService::~PortholeService()
{
    portholeServer->stop();
    delete portholeServer;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::start(int port, char* xmlPath, char* cssPath)
{
    myBinder = new PortholeFunctionsBinder();
    portholeServer = new ServerThread(this);
    portholeServer->setPort(port);
    portholeServer->setFunctionsBinder(myBinder);
    portholeServer->setXMLfile(xmlPath);
    portholeServer->setCSSPath(cssPath);
    portholeServer->start();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::setup(Setting& settings)
{
    cout << ">> !! Setup called" << endl;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::poll()
{
//	cout << ">> !! Poll called" << endl;
}

///////////////////////////////////////////////////////////////////////////////
//                                         BASE64 ENC/DEC                      
///////////////////////////////////////////////////////////////////////////////
static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

///////////////////////////////////////////////////////////////////////////////
inline bool is_base64(unsigned char c)
{
  return (isalnum(c) || (c == '+') || (c == '/'));
}

///////////////////////////////////////////////////////////////////////////////
inline std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

///////////////////////////////////////////////////////////////////////////////
inline std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyServerStarted()
{
    if(!myServerStartedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        i->queueCommand(myServerStartedCommand);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyConnected(const String& id)
{
    if(!myConnectedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myConnectedCommand, "%id%", id);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyDisconnected(const String& id)
{
    if(!myDisconnectedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myConnectedCommand, "%id%", id);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyCameraCreated(Camera* cam)
{
    if(!myCameraCreatedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String idstr = ostr("%1%", %cam->getCameraId());
        String cmd = StringUtils::replaceAll(myCameraCreatedCommand, "%id%", idstr);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::notifyCameraDestroyed(Camera* cam)
{
    if(!myCameraDestroyedCommand.empty())
    {
        PythonInterpreter* i = SystemManager::instance()->getScriptInterpreter();
        String idstr = ostr("%1%", %cam->getCameraId());
        String cmd = StringUtils::replaceAll(myCameraDestroyedCommand, "%id%", idstr);
        i->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::postPointerEvent(Event::Type type, int sourceId, int x, int y, uint flags)
{
    lockEvents();
    Event* evt = writeHead();
    evt->reset(type, Service::Pointer, sourceId, getServiceId());
    evt->setPosition(x, y);
    evt->setFlags(flags);
    unlockEvents();
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::postKeyEvent(Event::Type type, char key, uint flags)
{
    lockEvents();
    Event* evt = writeHead();
    evt->reset(type, Service::Keyboard, key, getServiceId());
    evt->setFlags(flags);
    unlockEvents();
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI* PortholeService::createClient(const String& name)
{
    PortholeGUI* cli = new PortholeGUI(this, name);
    myClients.push_back(cli);
    return cli;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::destroyClient(PortholeGUI* gui)
{
    oassert(gui != NULL);
    myClients.remove(gui);
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI* PortholeService::findClient(const String& name)
{
    foreach(PortholeGUI* cli, myClients)
    {
        if(cli->getId() == name) return cli;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::sendjs(const String& js, const String& destination)
{
    foreach(PortholeGUI* cli, myClients)
    {
        if(cli->getId() == destination)
        {
            cli->calljs(js);

        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeService::broadcastjs(const String& js, const String& origin)
{
    if(origin.empty())
    {
        foreach(PortholeGUI* cli, myClients)
        {
            cli->calljs(js);
        }
    }
    else
    {
        foreach(PortholeGUI* cli, myClients)
        {
            if(cli->getId() != origin) cli->calljs(js);
        }
    }
}
