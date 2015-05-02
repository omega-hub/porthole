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
#include "PortholeGUI.h"
#include "PortholeService.h"
#include "HTML.h"
#include <omicron/xml/tinyxml.h>
#include <iostream>

// H264 hardware encoder support
#ifdef llenc_ENABLED
#include "llenc/Encoder.h"
#endif

using namespace omega;
using namespace std;

PortholeFunctionsBinder* PortholeGUI::functionsBinder;
vector< Ref<PortholeInterfaceType> > PortholeGUI::interfaces;
std::map<string, omega::xml::TiXmlElement* > PortholeGUI::interfacesMap;
std::map<string, Ref<PortholeElement> > PortholeGUI::elementsMap;
std::map<int, PortholeCamera*> PortholeGUI::CamerasMap;

// define initial image quality: {0,1}
#define IMAGE_QUALITY 1

///////////////////////////////////////////////////////////////////////////////
inline float percentToFloat(String percent){
    return (float)(atoi(StringUtils::replaceAll(percent, "%", "").c_str()))/100;
}

///////////////////////////////////////////////////////////////////////////////
PortholeGUI::PortholeGUI(PortholeService* owner, const String& cliid):
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
PortholeGUI::~PortholeGUI()
{
    ofmsg("Porthole GUI client disconnected: %1%", %clientId);
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::updatePointerPosition(int dx, int dy)
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
            //orientation.compare(interfaces.at(i)->orientation)==0 &&
            interfaceId == interfaces.at(i)->id)
            device->interfaceType = interfaces.at(i);
    }
    
    oflog(Verbose, "[PortholeGUI::setDeviceSpecifications] %1%x%2% %3%  interface=%4%",
        %width %height %orientation %device->interfaceType->id);
}

///////////////////////////////////////////////////////////////////////////////
string PortholeGUI::create(bool firstTime)
{
    if(device->interfaceType == NULL)
    {
        return "Interface not available for this device";
    }

    string interfaceKey = device->interfaceType->id + device->deviceOrientation;
    
    ofmsg("[PortholeGUI] Getting interface <%1%>", %interfaceKey); 
    
    omega::xml::TiXmlElement* root = interfacesMap[interfaceKey];

    if (root == NULL) return "Interface not available for this device";

    string result = "";

    // If interface layout is vertical or horizontal, create a table to contain
    // all the html.
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

            String canvasId = "camera-canvas";
#ifdef llenc_ENABLED
            if(service->isHardwareEncoderEnabled()) canvasId = "camera-h264-stream";
#endif
            // tabindex='1' is used to make the canvas focusable, so we can get key events from it
            element->htmlValue = ostr(
                "<canvas id=\"%1%\" class=\"camera_container\" data-camera_id = \"%2%\" width=\"%3%\" height=\"%4%\" tabindex='1'></canvas>",
                %canvasId %sessionCamera->id %sessionCamera->canvasWidth % sessionCamera->canvasHeight);
        }
        // Embedded script element
        else if (element->type == "script")
        {
            String jsCode = element->htmlValue;
            String escapedjs = StringUtils::replaceAll(jsCode, "\"", "\\\"");
            calljs(escapedjs);
        }
        // HTML and / or script file element
        if(element->type != "script")
        {
            // If element type is a js file name, we load the file as part
            // of this element.
            if(StringUtils::endsWith(element->type, ".js"))
            {
                String jsCode = ostr(
                    "var _js = document.createElement('script'); "
                    "_js.type = 'text/javascript'; _js.src = '%1%'; "
                    "document.body.appendChild(_js);", %element->type);
                String escapedjs = StringUtils::replaceAll(jsCode, "\"", "\\\"");
                calljs(escapedjs);            
            }
            
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
    //cout << "Width: " << width  << " - height: " << height << endl;

    uint flags = Camera::DrawScene | Camera::DrawOverlay;
    Camera* camera = myEngine->createCamera(flags);
    camera->setMask(cameraMask);
    // Set the camera name using the client id and camera id
    String cameraName = ostr("%1%-%2%", %clientId %camera->getCameraId());
    camera->setName(cameraName);
    service->notifyCameraCreated(camera);
    
    oflog(Verbose, "[PortholeGUI::createCustomCamera]: %1% <%2%x%3%>", %cameraName %width %height);

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
    PortholeGUI::CamerasMap[pc->id] = pc; // Global map
    this->sessionCamera = pc; // Session

    // If low latency hardware encoding is available, use that. Otherwise, 
    // fall back to the old JPEG encoding & streaming.
#ifdef llenc_ENABLED
    if(service->isHardwareEncoderEnabled())
    {
        if(pc->camera->getListener() != NULL)
        {
            pc->streamer = (llenc::CameraStreamer*)pc->camera->getListener();
        }
        else
        {
            pc->streamer = new llenc::CameraStreamer();
            pc->camera->addListener(pc->streamer);
        }
    } 
    else
#endif
    {
        PixelData* sessionCanvas = new PixelData(PixelData::FormatRgb,  width,  height);
        pc->camera->getOutput(0)->setReadbackTarget(sessionCanvas);
        pc->camera->getOutput(0)->setEnabled(true);

        pc->canvas = sessionCanvas;
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::modCustomCamera(float widthPercent, float heightPercent)
{ 
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

    oflog(Verbose, "[PortholeGUI::modCustomCamera]: %1% (%2%x%3%)", 
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
        
#ifdef llenc_ENABLED
    if(service->isHardwareEncoderEnabled())
    {
        /*if(pc->streamer != NULL)
        {
            
        }*/
    }
    else
#endif
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
void PortholeGUI::parseElementDefinition(omega::xml::TiXmlElement* elem)
{
    PortholeElement* element = new PortholeElement();

    // Parse attributes
    omega::xml::TiXmlAttribute* pAttrib = elem->ToElement()->FirstAttribute();
    while(pAttrib)
    {
        string attribute = pAttrib->Name();
        StringUtils::toLowerCase(attribute);

        // Save attributes
        if(attribute == "id") element->id = pAttrib->Value();
        else if(attribute == "type") element->type = pAttrib->Value();
        else if (attribute == "camera") element->cameraType = pAttrib->Value();

        // Next attribute
        pAttrib = pAttrib->Next();
    }

    //StringUtils::toLowerCase(element->type);
    StringUtils::toLowerCase(element->cameraType);

    // For HTML and script elements, just add all the content to the element
    if(element->type == "html" || 
        element->type == "HTML" ||
        element->type == "Html" ||
            StringUtils::endsWith(element->type, ".js"))
    {
        // Parse the GUI elements
        for (omega::xml::TiXmlNode* pHtmlChild = elem->FirstChildElement(); pHtmlChild != 0; pHtmlChild = pHtmlChild->NextSiblingElement()){
            
            omega::xml::TiXmlPrinter* xmlPrinter = new omega::xml::TiXmlPrinter();
            
            pHtmlChild->Accept( xmlPrinter );
            element->htmlValue.append(xmlPrinter->CStr());
            // delete new line
            element->htmlValue.erase(std::remove(element->htmlValue.begin(), element->htmlValue.end(), '\n'), element->htmlValue.end());

            delete xmlPrinter;
        }

    }
    else if(element->type == "script")
    {
        element->htmlValue = elem->ToElement()->GetText();
    }
    if(element->id.length() > 0 && element->type.length() > 0)
    {
        elementsMap[element->id] = element;
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::parseInterfaceDefinition(omega::xml::TiXmlElement* elem)
{
    // Get element name
    String interfaceId = "";

    int minWidth=0, minHeight=0;

    // Parse attributes
    omega::xml::TiXmlAttribute* pAttrib = elem->FirstAttribute();
    while(pAttrib)
    {

        string attribute = pAttrib->Name();
        StringUtils::toLowerCase(attribute);

        // Save attributes
        if(attribute == "id") interfaceId = pAttrib->Value();
        else if(attribute == "minwidth") minWidth = pAttrib->IntValue();
        else if(attribute == "minheight") minHeight = pAttrib->IntValue();

        // Next attribute
        pAttrib = pAttrib->Next();
    }

    // For each orientation
    for (omega::xml::TiXmlElement* pOrientationChild = elem->FirstChildElement(); 
        pOrientationChild != 0; 
        pOrientationChild = pOrientationChild->NextSiblingElement())
    {
            
        // Get element name
        string orientation = string(pOrientationChild->Value());
        StringUtils::toLowerCase(orientation);

        // Layout attribute
        std::string layout;

        // Parse attributes
        omega::xml::TiXmlAttribute* pAttrib = pOrientationChild->FirstAttribute();
        while(pAttrib)
        {
            string attribute = pAttrib->Name();
            StringUtils::toLowerCase(attribute);

            // Save layout attribute
            if (attribute == "layout") layout = pAttrib->Value();

            // Next attribute
            pAttrib = pAttrib->Next();
        }

        PortholeInterfaceType* interfaceType = new PortholeInterfaceType();
        interfaceType->minWidth = minWidth;
        interfaceType->minHeight = minHeight;
        interfaceType->id = interfaceId;
        interfaceType->layout = layout;
        interfaces.push_back(interfaceType);
        
        // Check orientation and save node in the map
        // If the interface orientation is not specified, save the interface for 
        // both portrait and landscape mode.
        if (orientation == "portrait" || orientation =="port")
        {
            //interfaceType->orientation = "portrait";
            interfacesMap[interfaceId + orientation] = pOrientationChild;
        }
        else if(orientation == "landscape" || orientation == "land")
        {
            //interfaceType->orientation = "landscape";
            interfacesMap[interfaceId + orientation] = pOrientationChild;
        }
        else
        {
            interfacesMap[interfaceId + "landscape"] = pOrientationChild;
            interfacesMap[interfaceId + "portrait"] = pOrientationChild;
        }
    }	
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::parseInclude(omega::xml::TiXmlElement* elem)
{
    const char* fileName = elem->Attribute("file");
    if(fileName == NULL)
    {
        owarn("Porthole: include element missing file attribute");
        return;
    }
    String interfaceFilePath;
    if(DataManager::findFile(fileName, interfaceFilePath))
    {
        parseXmlFile(interfaceFilePath.c_str());
    }
    else
    {
        ofwarn("PortholeService::load: could not find interface file %1%", %fileName);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::parseNode(omega::xml::TiXmlElement* node)
{
    if ( !node ) return;
    
    const char* id = node->Attribute("id");

    // Parse attributes
    omega::xml::TiXmlAttribute* pAttrib = node->FirstAttribute();
    while(pAttrib)
    {
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
    
    // Traverse children
    for (omega::xml::TiXmlElement* pChild = node->FirstChildElement(); 
        pChild != 0; 
        pChild = pChild->NextSiblingElement())
    {
        parseNode(pChild);
    }

    // If this xml node is an porthole element or interface node, parse it
    String nodeName = node->Value();
    if(nodeName == "element") parseElementDefinition(node);
    else if(nodeName == "interface") parseInterfaceDefinition(node);
    else if(nodeName == "include") parseInclude(node);
}

///////////////////////////////////////////////////////////////////////////////
void PortholeGUI::parseXmlFile(const char* xmlPath)
{
    // Clean current xml interface data
    interfaces.clear();
    interfacesMap.clear();
    elementsMap.clear();


    ::xmlDoc = new omega::xml::TiXmlDocument(xmlPath);
    bool loadOkay = xmlDoc->LoadFile();
    if (!loadOkay)
    {
        oferror("Porthole: Failed to load XML file: %1%:%2% - %3%", 
            %xmlDoc->ErrorRow() %xmlDoc->ErrorCol() %xmlDoc->ErrorDesc());
        return;
    }

    parseNode(xmlDoc->FirstChildElement());
}
