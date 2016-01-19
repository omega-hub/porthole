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
#ifndef __SERVER_THREAD__
#define __SERVER_THREAD__

#include <omega.h>
#include "websockets/libwebsockets.h"
#include "vjson/json.h"
#include "PortholeClient.h"

using namespace std;
using namespace omicron;

#define ZOOM_STEP 0.025


class PortholeService;

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
// Struct of data to be passed across the entire session
struct per_session_data 
{
    PortholeClient* client;
    unsigned int userId;
    unsigned long long oldus;
    std::string test_flag;
};

///////////////////////////////////////////////////////////////////////////////
//! Implements, in a separate thread, the HTTP server for Porthole Service
class ServerThread: public Thread
{
public:
    // Websocket
    static int callbackWebsocket(libwebsocket_context *context, libwebsocket *wsi, 
        libwebsocket_callback_reasons reason, void *user, void *in, size_t len);
    static int streamJpeg(libwebsocket_context *context, libwebsocket *wsi, per_session_data* data);
    static int streamH264(libwebsocket_context *context, libwebsocket *wsi, per_session_data* data);

    // Json
    static void parseJsonMessage(json_value *value, per_session_data* data, recv_message* message);
    static void handleJsonMessage(per_session_data* data, recv_message* message, 
        libwebsocket_context *context, libwebsocket *wsi);
    static void sendHtmlElements(bool firstTime, per_session_data* data,
        libwebsocket_context *context, libwebsocket *wsi);
        
    // Http
    static int callbackHttp(libwebsocket_context *context, libwebsocket *wsi,
        libwebsocket_callback_reasons reason, void *user, void *in, size_t len);
    static void sendFile(libwebsocket *wsi, const String& filename);
    static void sendFunctionBindings(libwebsocket *wsi);
    
public:
    static PortholeService* service;

    // Constructor
    ServerThread(PortholeService* owner, const String& defaultPage);

    // Destructor
    ~ServerThread();

    // Set port
    void setPort(int portNumber); 

    // Thread process
    virtual void threadProc();

private:
    // Server params
    int port;
    struct libwebsocket_context *context; 
    int opts;
    int n;
    unsigned int oldus;
    char* minterface;

    // SSL vars - NOT TESTED
    int use_ssl;
    const char* cert_path;
    const char* key_path;
    
    static unsigned int sUserIdStart;
    static unsigned int sUserIdCounter;
    //! Path to resources, such as html files
    static String sWebserverDataRoot; 
    static String sWebserverDefaultPage;
    
};
#endif
