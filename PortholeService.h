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
#ifndef __PORTHOLE_SERVICE_H__
#define __PORTHOLE_SERVICE_H__

#include <omega.h>
//#include "websockets/libwebsockets.h"
#include "PortholeGUI.h"

using namespace std;
using namespace omicron;

class ServerThread;

///////////////////////////////////////////////////////////////////////////////
//! Implements an interface to handled device applications
class PortholeService: public Service
{
public:
    // Default constructor and destructor
    PortholeService();
    ~PortholeService();

    //! Start the server listen to the specified port and serve the specified
    //! default page.
    void start(int port, const String& defaultPage);

    //! Loads an xml interface file. Must be called after start. Can be called
    //! multiple times to load a different interface or to re-load an existing one.
    void load(const String& interfaceFile);
    PortholeFunctionsBinder* getFunctionsBinder() { return myBinder; }

    void postPointerEvent(Event::Type type, int sourceId, float x, float y, uint flags, unsigned int userId);
    void postKeyEvent(Event::Type type, char key, uint flags, unsigned int userId);

    // Server instance. It will manage the incoming connections
    //thread server_thread;
    ServerThread* portholeServer;

    void setConnectedCommand(const String cmd)
    { myConnectedCommand = cmd; }
    void setDisconnectedCommand(const String cmd)
    { myDisconnectedCommand = cmd; }
    void setCameraCreatedCommand(const String cmd)
    { myCameraCreatedCommand = cmd; }
    void setCameraDestroyedCommand(const String cmd)
    { myCameraDestroyedCommand = cmd; }
    void setServerStartedCommand(const String cmd)
    { myServerStartedCommand = cmd; }

    //! Sets the bounds for pointer events coming from web clients. If this 
    //! is left to (0, 0), bounds will be set to the display canvas size.
    //! Values set through this method will only apply to newly connected clients.
    void setPointerBounds(const Vector2i& ptr)
    { myPointerBounds = ptr; }
    Vector2i getPointerBounds()
    { return myPointerBounds; }

    //! Sets the speed for pointer events coming from web clients. 
    //! Values set through this method will only apply to newly connected clients.
    void setPointerSpeed(float speed)
    { myPointerSpeed = speed; }
    float getPointerSpeed()
    { return myPointerSpeed; }


    // Notification functions called from the websockets thread
    void notifyConnected(const String& id);
    void notifyDisconnected(const String& id);
    void notifyCameraCreated(Camera* cam);
    void notifyCameraDestroyed(Camera* cam);
    //! Called when the web server has started and is ready to receive
    //! connections.
    void notifyServerStarted();

    PortholeGUI* createClient(const String& name);
    void destroyClient(PortholeGUI* gui);
    PortholeGUI* findClient(const String& name);

    //! Sends a javascript call to the specified client
    void sendjs(const String& js, const String& destination);
    //! Broadcasts a javascript call to all connected clients, excluding the
    //! one indicated in the optional origin parameter
    void broadcastjs(const String& js, const String& origin = "");

private:
    String myConnectedCommand;
    String myDisconnectedCommand;
    String myCameraCreatedCommand;
    String myCameraDestroyedCommand;
    String myServerStartedCommand;
    PortholeFunctionsBinder* myBinder;

    List< Ref<PortholeGUI> > myClients;

    // Bounds for pointer events coming in from web clients. 
    Vector2i myPointerBounds;
    float myPointerSpeed;
};

#endif
