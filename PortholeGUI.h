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
#ifndef __PORTHOLE_GUI_H__
#define __PORTHOLE_GUI_H__

#include <omega.h>

#define PORTHOLE_EVENT_TOKEN_CAMERA_ID "%id%"
#define PORTHOLE_EVENT_TOKEN_VALUE "%value%"
#define PORTHOLE_EVENT_TOKEN_KEY "%key%"
#define PORTHOLE_EVENT_TOKEN_MOUSE_BTN "%btn%"
#define PORTHOLE_EVENT_TOKEN_EVENT "%event%"

// define initial image quality: {0,1}
#define IMAGE_QUALITY 1

using namespace omega;
using namespace omicron;
using namespace std;

// HTML namespace will contain all html events, 
// so that parser could know which attribute is a Javascript event
namespace HTML {

	static const int eventsNumber = 19;

	enum Event{
		OnLoad,
		OnUnload,
		OnBlur,
		OnChange,
		OnFocus,
		OnReset,
		OnSelect,
		OnSubmit,
		OnAbort,
		OnKeyDown,
		OnKeyPress,
		OnKeyUp,
		OnClick,
		OnDblClick,
		OnMouseDown,
		OnMouseMove,
		OnMouseOut,
		OnMouseOver,
		OnMouseUp
	};

	// All HTML compatible events that could be found parsing the application xml
	static const string events[eventsNumber] = {
		"onload",  /* Script to be run when a document load */
		"onunload",  /*  Script to be run when a document unload */
		"onblur",  /* Script to be run when an element loses focus */
		"onchange",  /* Script to be run when an element changes */
		"onfocus",  /* Script to be run when an element gets focus */
		"onreset",  /*  Script to be run when a form is reset */
		"onselect",  /* Script to be run when a document load */
		"onsubmit",  /* Script to be run when a form is submitted */
		"onabort",  /* Script to be run when loading of an image is interrupted */
		"onkeydown",  /* Script to be run when a key is pressed */
		"onkeypress",  /* Script to be run when a key is pressed and released */
		"onkeyup",  /* Script to be run when a key is released */
		"onclick",  /* Script to be run on a mouse click */
		"ondblclick",  /* Script to be run on a mouse double-click */
		"onmousedown",  /* Script to be run when mouse button is pressed */
		"onmousemove",  /* Script to be run when mouse pointer moves */
		"onmouseout",  /* Script to be run when mouse pointer moves out of an element */
		"onmouseover",  /* Script to be run when mouse pointer moves over an element */
		"onmouseup",  /* Script to be run when mouse button is released */
	};

	static bool isEvent(string stringToSearch)
	{
		for(int i=0; i < eventsNumber; i++)
			if (stringToSearch.compare(events[i]) == 0)
				return true; // found
		return false; // not found
	}

};

// This will old a possible interface
typedef struct PortholeInterfaceType: ReferenceType
{
	int minWidth;
	int minHeight;
	string id;
	string orientation;
	string layout;
} PortholeInterfaceType;

// A device specifications object
typedef struct PortholeDevice: ReferenceType
{
	int deviceWidth;
	int deviceHeight;
	string deviceOrientation; // Portrait or Landscape
	PortholeInterfaceType* interfaceType;
} PortholeDevice;

// An element object
struct PortholeElement: ReferenceType
{
	string id;
	string type;
	string cameraType; // Defined if type is camera stream
	string htmlValue;
};

// A omega Camera wrapper for Porthole
struct PortholeCamera: ReferenceType
{
	int id;
	Camera* camera;
	PixelData* canvas;
	int canvasWidth, canvasHeight;
	float size; // 1.0 is default value = device size
	//unsigned int oldusStreamSent; // Timestamp of last stream sent via socket
};

// An obj binded with a Javascript event
struct PortholeEvent
{
	std::string htmlEvent;
	int mouseButton;
	char key;
	std::string value;
	PortholeCamera* sessionCamera;
    Dictionary<String, String> args;
};

// Porthole functions binder
struct PortholeFunctionsBinder: ReferenceType
{
	typedef void(*memberFunction)(PortholeEvent&);

	void addFunction(std::string funcName, memberFunction func)
	{
		cppFuncMap[funcName] = func;
	}

	void addPythonScript(std::string script, string key){
		pythonFunMap[key] = script;
		scriptNumber++;
	}

	void callFunction(std::string funcName, PortholeEvent &ev)
	{
		std::map<std::string, memberFunction>::const_iterator cpp_it;
		cpp_it = cppFuncMap.find(funcName);
		if (cpp_it != cppFuncMap.end())	return (*cpp_it->second)(ev);

		std::map<std::string, string>::const_iterator py_it;
		py_it = pythonFunMap.find(funcName);
		if (py_it != pythonFunMap.end())
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

			if (ev.sessionCamera != NULL)
				pythonScript = omicron::StringUtils::replaceAll(pythonScript, PORTHOLE_EVENT_TOKEN_CAMERA_ID, boost::lexical_cast<std::string>(ev.sessionCamera->id));

			pi->queueCommand(pythonScript); 
		}
		return;
	}

	bool isCppDefined(string funcName)
	{
		std::map<std::string, memberFunction>::const_iterator it;
		it = cppFuncMap.find(funcName);
		if (it != cppFuncMap.end()) return true;
		return false;
	}

	std::map<std::string, memberFunction> cppFuncMap;
	std::map<std::string, string> pythonFunMap;
	int scriptNumber;
};

// Xml Document
static omega::xml::TiXmlDocument* xmlDoc;

class PortholeService;

///////////////////////////////////////////////////////////////////////////////
//! Implements the HTML GUI Manager for Porthole Service
class PortholeGUI: public ReferenceType
{
public:

	// Constructor
	PortholeGUI(PortholeService* owner, const String& clientId);

	// Destructor
	~PortholeGUI();

	// Create the device specifc html interface
	string create(bool firstTime);

	// Set device specifications
	void setDeviceSpecifications(int width, int height, string orientation);

	// Return an object that contains the device specifications
	PortholeDevice* getDevice() { return device; }

	bool isCameraReadyToStream() 
	{ return (sessionCamera != NULL && sessionCamera->camera->isEnabled()); } 

	// Get Porthole camera object for this client connected
	PortholeCamera* getSessionCamera() { return sessionCamera; } 
	PortholeService* getService()
	{ return service; }

	// Mod the camera with id cameraId 
	// size: the ratio of camera: 1.0 is full size
	void modCustomCamera(float size, float widthPercent, float heightPercent);

	// Parse HTML gui_element and look for Javascript events 
	static vector<string> findHtmlScripts();

	// Start application XML parsing
	static void parseXmlFile(char* xmlPath);

	// Functions binder getter and setter
	static PortholeFunctionsBinder* getPortholeFunctionsBinder() { return functionsBinder; }
	static void setPortholeFunctionsBinder(PortholeFunctionsBinder* binder) { functionsBinder = binder;  functionsBinder->scriptNumber=0;}

	//! Global map of cameras by id
	static std::map<int, PortholeCamera*> CamerasMap;

private:
	PortholeService* service;

	// The device for which an interface will be created
	PortholeDevice* device;

	// The camera of this session
	PortholeCamera* sessionCamera;

	String clientId;

	// Create a Porthole custom camera and a PixelData associated
	void createCustomCamera(float widthPercent, float heightPercent, uint cameraMask = 0); 

	static void searchNode(omega::xml::TiXmlElement* node);

	// Functions binder object
	static PortholeFunctionsBinder* functionsBinder;

	// All the possible interfaces
	static vector<PortholeInterfaceType*> interfaces; 

	// A map between a device type and its GUI elements
	static std::map<string, omega::xml::TiXmlElement* > interfacesMap;

	// A map between an element id and the element data
	static std::map<string, PortholeElement*> elementsMap;
};

#endif