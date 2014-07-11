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
#include "PortholeGUI.h"
#include "PortholeService.h"
#include <omicron/xml/tinyxml.h>
#include <iostream>

using namespace omega;
using namespace std;

PortholeFunctionsBinder* PortholeGUI::functionsBinder;
vector< Ref<PortholeInterfaceType> > PortholeGUI::interfaces;
std::map<string, omega::xml::TiXmlElement* > PortholeGUI::interfacesMap;
std::map<string, PortholeElement*> PortholeGUI::elementsMap;
std::map<int, PortholeCamera*> PortholeGUI::CamerasMap;

///////////////////////////////////////////////////////////////////////////////
inline float percentToFloat(String percent){
    return (float)(atoi(StringUtils::replaceAll(percent, "%", "").c_str()))/100;
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI::PortholeGUI(PortholeService* owner, const String& cliid):
    service(owner), clientId(cliid)
{
    this->sessionCamera = NULL;

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
        if(canvasSize[0] == 1 && canvasSize[1] == 1)
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
    pointerPosition = Vector2f::Zero();
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI::~PortholeGUI()
{
    if(sessionCamera != NULL)
    {
        //service->notifyCameraCreated(sessionCamera->camera);
        Engine::instance()->destroyCamera(this->sessionCamera->camera);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::updatePointerPosition(int dx, int dy)
{
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
void PortholeGUI::setDeviceSpecifications(int width, int height, const String& orientation, const String& interfaceId)
{
    // Set the device
    this->device = new PortholeDevice();
    device->deviceWidth = width;
    device->deviceHeight = height;
    device->deviceOrientation = orientation;
    
    for(int i=0; i<interfaces.size(); i++)
    {
        if (width > interfaces.at(i)->minWidth && 
            height > interfaces.at(i)->minHeight &&
            orientation.compare(interfaces.at(i)->orientation)==0 &&
            interfaceId == interfaces.at(i)->id)
            device->interfaceType = interfaces.at(i);
    }
}

///////////////////////////////////////////////////////////////////////////////
string PortholeGUI::create(bool firstTime)
{
    if(device->interfaceType == NULL)
    {
        return "Interface not available for this device";
    }

    string interfaceKey = device->interfaceType->id + device->interfaceType->orientation;
    omega::xml::TiXmlElement* root = interfacesMap[interfaceKey];

    if (root == NULL) return "Interface not available for this device";

    string result = "";

    if(device->interfaceType->layout == "vertical" ||
        device->interfaceType->layout == "horizontal")
    {
        result = "<table data-role=\"none\" border=\"0\" style=\" width : 100%; height : 100%; \" cellspacing=\"0\" cellpadding=\"0\">";
    
        if (device->interfaceType->layout == "horizontal")
        {
            result.append("<tr style=\" width : 100%; height : 100%; \">");
        }
    }

    // Parse the GUI elemets disposition
    for (omega::xml::TiXmlElement* pChild = root->FirstChildElement(); pChild != 0; pChild = pChild->NextSiblingElement())
    {
        // Get element name: should correspond to id
        String id;

        string width="default", height="default", x="0px", y="0px";

        // Parse attributes
        omega::xml::TiXmlAttribute* pAttrib = pChild->ToElement()->FirstAttribute();
        while(pAttrib)
        {

            string attribute = pAttrib->Name();
            StringUtils::toLowerCase(attribute);

            // Save attributes to local variables
            if (attribute == "id") id = pAttrib->Value();
            else if (attribute == "width") width = pAttrib->Value();
            else if (attribute == "height") height = pAttrib->Value();
            else if (attribute == "x")  x = pAttrib->Value();
            else if (attribute == "y") y = pAttrib->Value();

            // Next attribute
            pAttrib = pAttrib->Next();
        }

        PortholeElement* element = elementsMap[id];

        if (element == NULL)
        {
            std::cout << "!! >> XID Error: No element found. Check <element> ids inside <interface> elements."
                "Check if that element has been defined inside <elements>" << std::endl;
            abort();
        }

        /*
        *	GOOGLE MAPS
        */
        // For googlemaps element, create a div element that will contain a googlemaps view
        // TODO Size of map canvas
        if (element->type == "googlemap")
        {
            element->htmlValue = "<div  style=\" padding : 5px \"><input id=\"searchTextField\" type=\"text\" style=\"width : 100%; height : 20px;\"></div>";
            element->htmlValue.append("<div id=\"map-canvas\" class=\"map_container\" style=\"width:");
            element->htmlValue.append(boost::lexical_cast<string>(device->deviceWidth)+"px; height:"+
                              boost::lexical_cast<string>(device->deviceHeight)+"px \" ");
            element->htmlValue.append("></div>");
        }

        /*
        *	CAMERA STREAM
        */
        // Create a session camera
        else if (element->type == "camera_stream")
        {

            // Default camera case
            if (strcmp(element->cameraType.c_str(),"default")==0)
            {
                if (firstTime || this->sessionCamera == NULL)
                {
                    createCustomCamera(true, percentToFloat(width), percentToFloat(height));
                }
                else
                {
                    modCustomCamera(percentToFloat(width), percentToFloat(height));
                }
            }
            // Custom camera case
            else 
            {
                // Parse camera mask
                uint camMask = 0;
                Vector<String> args = StringUtils::split(element->cameraType, ", ");
                // The first word is always treated as the 'camera name' and ignored here. The second word (if present) is
                // used as the camera render pass mask. It is used to specify which render passes will draw content for this camera.
                if(args.size() == 2)
                {
                    camMask = boost::lexical_cast<uint>(args[1]);
                    // Use argument as bit number for the camera mask
                    camMask = 1 << camMask;
                }

                if (firstTime || this->sessionCamera == NULL)
                {
                    createCustomCamera(percentToFloat(width), percentToFloat(height), camMask);
                }
                else
                {
                    modCustomCamera(percentToFloat(width), percentToFloat(height));
                }
            }

            element->htmlValue = ostr("<canvas id=\"camera-canvas\" class=\"camera_container\" data-camera_id = \"%1%\" width=\"%2%\" height=\"%3%\"></canvas>", 
                %sessionCamera->id %sessionCamera->canvas->getWidth() %sessionCamera->canvas->getHeight());

        }
        else if (element->type == "script")
        {
            String escapedjs = StringUtils::replaceAll(element->htmlValue, "\"", "\\\"");
            calljs(escapedjs);
        }

        if(element->type != "script")
        {
            // Create the HTML result for this element. embedded into a table
            // tr (for vertical) or td (for horizontal) element TODO Layouts Grid/Relative
            // HORIZONTAL
            if ( device->interfaceType->layout.compare("horizontal")==0 ||
                device->interfaceType->layout.compare("hor") == 0 )
            {
                    result.append("<td style=\" width : "+ width +"; height : "+ height +"; \">"+ element->htmlValue +"</td>");
            }
            // VERTICAL
            else if ( device->interfaceType->layout.compare("vertical")==0 ||
                device->interfaceType->layout.compare("ver") == 0 )
            {
                    result.append("<tr style=\" width : "+ width +"; height : "+ height +"; \"><td style=\" width : "+ width +"; height : "+ height +"; \">"+ element->htmlValue +"</td></tr>");
            }
            // Else free
            else 
            {
                result.append(
                    ostr("<div style=\" width: %1%; height: %2%; left: %3%; top:%4%; position:absolute; \">%5%</div>",
                    %width %height %x %y %element->htmlValue));
            }
        }
    }

    if ( device->interfaceType->layout.compare("horizontal")==0 ||
            device->interfaceType->layout.compare("hor") == 0 ){
        result.append("</tr>");
    }

    result.append("</table>");

    return result;

}

///////////////////////////////////////////////////////////////////////////////
/* 
*	Camera creation function
*/
void PortholeGUI::createCustomCamera(float widthPercent, float heightPercent, uint cameraMask)
{

    // Get the global engine
    Engine* myEngine = Engine::instance();

    // Initialize camera size
    // Workaround. This avoid a canvas drawing bug
    // Round down width to a multiple of 4.
    int width = (int)( widthPercent * IMAGE_QUALITY * device->deviceWidth / 4 ) * 4;
    int height = (int)( heightPercent * IMAGE_QUALITY * device->deviceHeight / 4 ) * 4;
    // cout << "Width: " << width  << " - height: " << height << endl;

    PixelData* sessionCanvas = new PixelData(PixelData::FormatRgb,  width,  height);

    uint flags = Camera::DrawScene | Camera::DrawOverlay;

    Camera* sessionCamera = myEngine->createCamera(flags);
    sessionCamera->setMask(cameraMask);
    // Set the camera name using the client id and camera id
    String cameraName = ostr("%1%-%2%", %clientId %sessionCamera->getCameraId());
    sessionCamera->setName(cameraName);
    service->notifyCameraCreated(sessionCamera);

    // Notify camera creation


    DisplayTileConfig* dtc = sessionCamera->getCustomTileConfig();
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
    sessionCamera->setPosition(defaultCamera->getPosition());
    sessionCamera->setHeadOffset(defaultCamera->getHeadOffset());
    sessionCamera->getOutput(0)->setReadbackTarget(sessionCanvas);
    sessionCamera->getOutput(0)->setEnabled(true);

    // Save the new Camera and PixelData objects
    PortholeCamera* camera = new PortholeCamera();
    camera->id = sessionCamera->getCameraId();
    camera->camera =sessionCamera;
    camera->canvas = sessionCanvas;
    camera->canvasWidth = width;
    camera->canvasHeight = height;
    camera->size = IMAGE_QUALITY;

    // Save new camera
    PortholeGUI::CamerasMap[camera->id] = camera; // Global map
    this->sessionCamera = camera; // Session

}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::modCustomCamera(float widthPercent, float heightPercent){ 

    // Retrieve the camera to be modified
    PortholeCamera* portholeCamera = this->sessionCamera;

    Camera* sessionCamera = portholeCamera->camera;

    // Get the global engine
    Engine* myEngine = Engine::instance();

    // Initialize camera size
    // Workaround. This avoid a canvas drawing bug
    // Round down width to a multiple of 4.
    int width = (int)(widthPercent * portholeCamera->size * device->deviceWidth / 4) * 4;
    int height = (int)(heightPercent * portholeCamera->size * device->deviceHeight / 4) * 4;

    // cout << "Width: " << width  << " - height: " << height << endl;

    // Set new camera target
    portholeCamera->canvas = new PixelData(PixelData::FormatRgb, width, height);
    sessionCamera->getOutput(0)->setReadbackTarget(portholeCamera->canvas);
    sessionCamera->getOutput(0)->setEnabled(true);

    DisplayTileConfig* dtc = sessionCamera->getCustomTileConfig();
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

    portholeCamera->canvasWidth = width;
    portholeCamera->canvasHeight = height;
}

///////////////////////////////////////////////////////////////////////////////
String PortholeGUI::flushJavascriptQueueToJson()
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

///////////////////////////////////////////////////////////////////////////////
/*Recursive*/void PortholeGUI::searchNode(omega::xml::TiXmlElement* node){

    if ( !node ) return;

    const char* id = node->Attribute("id");

    // Parse attributes
    omega::xml::TiXmlAttribute* pAttrib = node->FirstAttribute();
    while(pAttrib){

        string attribute = pAttrib->Name();
        StringUtils::toLowerCase(attribute);

        // Save id attribute if not defined by user as cpp function
        if (HTML::isEvent(attribute) && !functionsBinder->isCppDefined(pAttrib->Value()))
        {
            // NOTE: If the value starts with %js, we consider the call a javascript
            // call, and we do not generate a python call site.
            String val = pAttrib->Value();
            if(StringUtils::startsWith(val, "%js"))
            {
                val = StringUtils::replaceAll(val, "%js", "");
                pAttrib->SetValue(val.c_str());
            }
            else
            {
                //cout << "Adding script " << pAttrib->Value() << endl; 
                String scriptId = boost::lexical_cast<string>(functionsBinder->scriptNumber);
                string key = "python_script" + scriptId + "(event)";

                if(id == NULL)
                {
                    functionsBinder->addPythonScript(pAttrib->Value(), key, "python_script" + scriptId); 
                }
                else
                {
                    // Insert the new script                    
                    functionsBinder->addPythonScript(pAttrib->Value(), key, id); 
                }
                pAttrib->SetValue(key.c_str());
            }
        }

        // Next attribute
        pAttrib = pAttrib->Next();
    }

    // Check recursively
    for (omega::xml::TiXmlElement* pChild = node->FirstChildElement(); pChild != 0; pChild = pChild->NextSiblingElement()){
        searchNode(pChild);
    }

}

///////////////////////////////////////////////////////////////////////////////
vector<string> PortholeGUI::findHtmlScripts(){

    vector<string> result;

    omega::xml::TiXmlNode* guiElements = xmlDoc->FirstChildElement()->FirstChildElement();

    searchNode(guiElements->ToElement());

    return result;
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::parseXmlFile(char* xmlPath)
{
    ::xmlDoc = new omega::xml::TiXmlDocument(xmlPath);
    bool loadOkay = xmlDoc->LoadFile();
    if (!loadOkay){
        ofwarn("Porthole: Failed to load XML file: %1%:%2% - %3%", 
            %xmlDoc->ErrorRow() %xmlDoc->ErrorCol() %xmlDoc->ErrorDesc());
        abort();
    }

    // Recursive search for Python Scripts inside events handlers
    findHtmlScripts();

    omega::xml::TiXmlNode* guiElements = xmlDoc->FirstChildElement()->FirstChildElement();

    // Parse the GUI elements
    for (omega::xml::TiXmlNode* pChild = guiElements->FirstChildElement(); pChild != 0; pChild = pChild->NextSiblingElement()){

        PortholeElement* element = new PortholeElement();

        // Parse attributes
        omega::xml::TiXmlAttribute* pAttrib = pChild->ToElement()->FirstAttribute();
        while(pAttrib){

            string attribute = pAttrib->Name();
            StringUtils::toLowerCase(attribute);

            // Save id attribute
            if (attribute.compare("id")==0){
                element->id = pAttrib->Value();
            }

            // Save type attribute
            else if (attribute.compare("type")==0){
                element->type = pAttrib->Value();
            }

            // Save camera type attribute
            else if (attribute.compare("camera")==0){
                element->cameraType = pAttrib->Value();
            }

            // Next attribute
            pAttrib = pAttrib->Next();
        }

        StringUtils::toLowerCase(element->type);
        StringUtils::toLowerCase(element->cameraType);

        // For HTML and script elements, just add all the content to the element
        if  (element->type == "html"){

            // Parse the GUI elements
            for (omega::xml::TiXmlNode* pHtmlChild = pChild->FirstChildElement(); pHtmlChild != 0; pHtmlChild = pHtmlChild->NextSiblingElement()){
                
                omega::xml::TiXmlPrinter* xmlPrinter = new omega::xml::TiXmlPrinter();
                
                pHtmlChild->Accept( xmlPrinter );
                //cout << "Added: " << id << " -> " << xmlPrinter->CStr() << endl;
                element->htmlValue.append(xmlPrinter->CStr());
                // delete new line
                element->htmlValue.erase(std::remove(element->htmlValue.begin(), element->htmlValue.end(), '\n'), element->htmlValue.end());

                delete xmlPrinter;
            }

        }
        else if(element->type == "script")
        {
            element->htmlValue = pChild->ToElement()->GetText();
        }

        if(element->id.length() > 0 && element->type.length() > 0){
            elementsMap[element->id] = element;
        }

    }

    omega::xml::TiXmlNode* guiDisposition = guiElements->NextSiblingElement();

    // Parse the GUI elemets disposition
    // For each specified interface size
    for (omega::xml::TiXmlElement* pInterfaceChild = guiDisposition->FirstChildElement(); pInterfaceChild != 0; pInterfaceChild = pInterfaceChild->NextSiblingElement()){

            // Get element name
            String interfaceId = "";

            int minWidth=0, minHeight=0;

            // Parse attributes
            omega::xml::TiXmlAttribute* pAttrib = pInterfaceChild->FirstAttribute();
            while(pAttrib){

                string attribute = pAttrib->Name();
                StringUtils::toLowerCase(attribute);

                // Save id attribute
                if (attribute.compare("id")==0){
                    interfaceId = pAttrib->Value();
                }

                // Save min width attribute
                if (attribute.compare("minwidth")==0){
                    minWidth = pAttrib->IntValue();
                }


                // Save min height attribute
                else if (attribute.compare("minheight")==0){
                    minHeight = pAttrib->IntValue();
                }

                // Next attribute
                pAttrib = pAttrib->Next();
            }

        // For each orientation
        for (omega::xml::TiXmlElement* pOrientationChild = pInterfaceChild->FirstChildElement(); pOrientationChild != 0; pOrientationChild = pOrientationChild->NextSiblingElement()){
                
            // Get element name
            string orientation = string(pOrientationChild->Value());
            StringUtils::toLowerCase(orientation);

            // Layout attribute
            std::string layout;

            // Parse attributes
            omega::xml::TiXmlAttribute* pAttrib = pOrientationChild->FirstAttribute();
            while(pAttrib){

                string attribute = pAttrib->Name();
                StringUtils::toLowerCase(attribute);

                // Save layout attribute
                if (attribute.compare("layout")==0){
                    layout = std::string(pAttrib->Value());
                }

                // Next attribute
                pAttrib = pAttrib->Next();
            }

            // Check orientation and save node in the map
            if (orientation.compare("portrait")==0 || orientation.compare("port")==0){
                PortholeInterfaceType* interfaceType = new PortholeInterfaceType();
                interfaceType->minWidth = minWidth;
                interfaceType->minHeight = minHeight;
                interfaceType->id = interfaceId;
                interfaceType->orientation = "portrait";
                interfaceType->layout = layout;
                interfaces.push_back(interfaceType);
                cout << ">> Added interface:" << interfaceId << " " << orientation << " " << minWidth << " " << minHeight << endl;
                interfacesMap[interfaceId + orientation] = pOrientationChild;
            }
            else if (orientation.compare("landscape")==0 || orientation.compare("land")==0){
                PortholeInterfaceType* interfaceType = new PortholeInterfaceType();
                interfaceType->minWidth = minHeight;
                interfaceType->minHeight = minWidth;
                interfaceType->id = interfaceId;
                interfaceType->orientation = "landscape";
                interfaceType->layout = layout;
                interfaces.push_back(interfaceType);
                cout << ">> Added interface:" << interfaceId << " " << orientation << " " << minHeight << " " << minWidth << endl;
                interfacesMap[interfaceId + orientation] = pOrientationChild;
            }
        }	
    }

}

///////////////////////////////////////////////////////////////////////////////
//void PortholeGUI::setStreamQuality(float quality)
//{
//    this->sessionCamera->size = quality;
//}
