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
// this protocol server (always the first one) just knows how to do HTTP
int ServerThread::callbackHttp(libwebsocket_context *context, libwebsocket *wsi,
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
    
    oflog(Verbose, "[ServerThread::sendFile] serve %1%", %fullFilePath);

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
    PortholeFunctionsBinder* functionsBinder = PortholeClient::getPortholeFunctionsBinder();
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
