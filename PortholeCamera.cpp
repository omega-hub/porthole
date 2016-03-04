/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2016		Electronic Visualization Laboratory, 
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
#include "PortholeCamera.h"
#include "PortholeService.h"
#include "base64.h"

using namespace omega;
using namespace std;

extern PortholeService* sServiceInstance;

///////////////////////////////////////////////////////////////////////////////
PortholeCamera::PortholeCamera()
{}

///////////////////////////////////////////////////////////////////////////////
PortholeCamera::~PortholeCamera()
{
    //if(myStreamer != NULL) myCamera->removeListener(streamer);
}


///////////////////////////////////////////////////////////////////////////////
void PortholeCamera::initialize(Camera* cam, const String& elemId, uint width, uint height)
{
    myElementId = elemId;
    myStreamer = new CameraStreamer(sServiceInstance->getStreamEncoderType());
    myStreamer->setResolution(Vector2i(width, height));
    cam->addListener(myStreamer);
}

///////////////////////////////////////////////////////////////////////////////
void PortholeCamera::resize(uint width, uint height)
{
    myStreamer->setResolution(Vector2i(width, height));
}

///////////////////////////////////////////////////////////////////////////////
int PortholeCamera::sendStream(PortholeClient* c)
{
    int n = 0;
    IEncoder* e = myStreamer->lockEncoder();
    if(e != NULL && e->dataAvailable())
    {
        const void* bitstream;
        uint32_t size;
        if(e->lockBitstream(&bitstream, &size))
        {
            if(myStreamer->getEncoderName() == "png" || myStreamer->getEncoderName() == "jpeg")
            {
                std::string base64image = base64_encode((unsigned char*)bitstream, size);

                // String to be send: base64 image and camera id
                string toSend = "{\"event_type\":\"stream\",\"base64image\":\"";
                toSend.append(base64image.c_str());
                toSend.append("\",\"element_id\":\"" + boost::lexical_cast<string>(myElementId) +
                    "\"}");
                n = c->send(toSend);

            }
            else
            {
                n = c->send(bitstream, (size_t)size);
            }
            e->unlockBitstream();
        }
        else
        {
            omsg("Bitstream lock failed");
            oexit(0);
        }
    }

    myStreamer->unlockEncoder();
    return n;
}