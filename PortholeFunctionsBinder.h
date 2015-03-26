/******************************************************************************
* THE OMEGA LIB PROJECT
*-----------------------------------------------------------------------------
* Copyright 2010-2014		Electronic Visualization Laboratory,
*							University of Illinois at Chicago
* Authors:
*  Daniele Donghi			d.donghi@gmail.com
*  Alessandro Febretti		febret@gmail.com
*-----------------------------------------------------------------------------
* Copyright (c) 2010-2014, Electronic Visualization Laboratory,
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

#define PORTHOLE_EVENT_TOKEN_CAMERA_ID "%id%"
#define PORTHOLE_EVENT_TOKEN_VALUE "%value%"
#define PORTHOLE_EVENT_TOKEN_KEY "%key%"
#define PORTHOLE_EVENT_TOKEN_MOUSE_BTN "%btn%"
#define PORTHOLE_EVENT_TOKEN_EVENT "%event%"

#ifdef llenc_ENABLED
#include "llenc/CameraStreamer.h"
#endif

using namespace omicron;
using namespace omega;

///////////////////////////////////////////////////////////////////////////////
// This will old a possible interface
struct PortholeInterfaceType : public ReferenceType
{
    int minWidth;
    int minHeight;
    string id;
    string orientation;
    string layout;
};

///////////////////////////////////////////////////////////////////////////////
// A device specifications object
struct PortholeDevice : public ReferenceType
{
    int deviceWidth;
    int deviceHeight;
    string deviceOrientation; // Portrait or Landscape
    Ref<PortholeInterfaceType> interfaceType;
};

///////////////////////////////////////////////////////////////////////////////
// An element object
struct PortholeElement : ReferenceType
{
    string id;
    string type;
    string cameraType; // Defined if type is camera stream
    string htmlValue;
};

///////////////////////////////////////////////////////////////////////////////
// A omega Camera wrapper for Porthole
struct PortholeCamera : ReferenceType
{
    int id;
    float targetFps; // The desired output fps for this camera.
    int highFps; // When the frame rate passes this fps, increase stream quality.
    int lowFps; // When the frame rate is lower than this fps, decrease stream quality.
    Camera* camera;
    PixelData* canvas;
    int canvasWidth, canvasHeight;
    float size; // 1.0 is default value = device size
    //unsigned int oldusStreamSent; // Timestamp of last stream sent via socket

    // H264 hardware encoder support
#ifdef llenc_ENABLED
    Ref<llenc::CameraStreamer> streamer;
#endif
    PortholeCamera() :
        targetFps(60),
        highFps(15),
        lowFps(5)
    {}
};

///////////////////////////////////////////////////////////////////////////////
// An obj bound with a Javascript event
struct PortholeEvent
{
    PortholeEvent(const String& clid) :
        clientId(clid)
    {}

    std::string htmlEvent;
    int mouseButton;
    char key;
    const String& clientId;
    std::string value;
    PortholeCamera* sessionCamera;
    Dictionary<String, String> args;
};

///////////////////////////////////////////////////////////////////////////////
class PortholeFunctionsBinder : ReferenceType
{
public:
    typedef void(*memberFunction)(PortholeEvent&);

    void addFunction(std::string funcName, memberFunction func)
    {
        cppFuncMap[funcName] = func;
    }

    void addPythonScript(std::string script, string key, string elemid)
    {
        pythonFunMap[key] = script;
        pythonFunIdMap[key] = elemid;
        scriptNumber++;
    }

    void callFunction(std::string funcName, PortholeEvent &ev)
    {
        std::map<std::string, memberFunction>::const_iterator cpp_it;
        cpp_it = cppFuncMap.find(funcName);
        if(cpp_it != cppFuncMap.end())	return (*cpp_it->second)(ev);

        std::map<std::string, string>::const_iterator py_it;
        py_it = pythonFunMap.find(funcName);
        if(py_it != pythonFunMap.end())
        {
            PythonInterpreter* pi = SystemManager::instance()->getScriptInterpreter();

            // Substitute special %value% token
            String pythonScript = omicron::StringUtils::replaceAll(py_it->second, PORTHOLE_EVENT_TOKEN_VALUE, ev.value);

            // Substitute other argument tokens.
            typedef KeyValue<String, String> ArgItem;
            foreach(ArgItem i, ev.args)
            {
                String key = ostr("%%%1%%%", %i.getKey());
                pythonScript = omicron::StringUtils::replaceAll(pythonScript, key, i.getValue());
            }

            pythonScript = omicron::StringUtils::replaceAll(pythonScript, PORTHOLE_EVENT_TOKEN_KEY, boost::lexical_cast<std::string>(ev.key));
            pythonScript = omicron::StringUtils::replaceAll(pythonScript, PORTHOLE_EVENT_TOKEN_MOUSE_BTN, boost::lexical_cast<std::string>(ev.mouseButton));
            pythonScript = omicron::StringUtils::replaceAll(pythonScript, PORTHOLE_EVENT_TOKEN_EVENT, boost::lexical_cast<std::string>(ev.htmlEvent));
            pythonScript = omicron::StringUtils::replaceAll(pythonScript, "%client_id%", "\"" + ev.clientId + "\"");

            if(ev.sessionCamera != NULL)
                pythonScript = omicron::StringUtils::replaceAll(pythonScript, PORTHOLE_EVENT_TOKEN_CAMERA_ID, boost::lexical_cast<std::string>(ev.sessionCamera->id));

            pi->queueCommand(pythonScript);
        }
        return;
    }

    bool isCppDefined(string funcName)
    {
        std::map<std::string, memberFunction>::const_iterator it;
        it = cppFuncMap.find(funcName);
        if(it != cppFuncMap.end()) return true;
        return false;
    }

    void clear()
    {
        scriptNumber = 0;
        cppFuncMap.clear();
        pythonFunMap.clear();
        pythonFunIdMap.clear();
    }

    std::map<std::string, memberFunction> cppFuncMap;
    std::map<std::string, string> pythonFunMap;
    std::map<std::string, string> pythonFunIdMap;
    int scriptNumber;
};
