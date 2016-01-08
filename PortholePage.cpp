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
#include "PortholePage.h"
#include "PortholeService.h"
#include "HTML.h"
#include <omicron/xml/tinyxml.h>
#include <iostream>

using namespace omega;
using namespace std;

PortholeFunctionsBinder* PortholePage::functionsBinder;

///////////////////////////////////////////////////////////////////////////////
PortholePage::PortholePage(PortholeService* owner, const String& cliid):
service(owner), clientId(cliid)
{
    ofmsg("Porthole GUI client connected: %1%", %clientId);    
}

///////////////////////////////////////////////////////////////////////////////
PortholePage::~PortholePage()
{
    ofmsg("Porthole GUI client disconnected: %1%", %clientId);
}

///////////////////////////////////////////////////////////////////////////////
String PortholePage::create(const String& filename)
{
    String result;

    omega::xml::TiXmlDocument* xmlDoc = new omega::xml::TiXmlDocument(filename.c_str());
    bool loadOkay = xmlDoc->LoadFile();
    if(!loadOkay)
    {
        oferror("Porthole: Failed to load XML file: %1%:%2% - %3%",
            %xmlDoc->ErrorRow() % xmlDoc->ErrorCol() % xmlDoc->ErrorDesc());
        return "";
    }

    parseNode(xmlDoc->FirstChildElement());
    return result;
}

///////////////////////////////////////////////////////////////////////////////
String PortholePage::flushJavascriptQueueToJson()
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
void PortholePage::parseNode(omega::xml::TiXmlElement* node)
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
    
    // If this xml node is an porthole element or interface node, parse it
    String nodeName = node->Value();
    // Traverse children
    for(omega::xml::TiXmlElement* pChild = node->FirstChildElement();
        pChild != 0;
        pChild = pChild->NextSiblingElement())
    {
        parseNode(pChild);
    }
}
