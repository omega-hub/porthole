/********************************************************************************************************************** 
 * THE OMEGA LIB PROJECT
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright 2010-2013							Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:										
 *  Daniele Donghi								d.donghi@gmail.com
 *  Alessandro Febretti							febret@gmail.com
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2010-2013, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
 * following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
 * disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
 * and the following disclaimer in the documentation and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------------------------------------------------
 *	portholeCustomDraw
 *		Shows how to do custom drawing for porthole clients. Runs a separate render pass that only renders to the 
 *		porthole camera.
 *********************************************************************************************************************/
#include <omega.h>
#include <cyclops.h>
#include <iostream>
#include <omegaToolkit.h>

using namespace omega;
using namespace cyclops;

#define PORTHOLE_CAMERA_MASK 0x01

class PortholeCustomDrawApplication;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PortholeRenderPass: public RenderPass
{
public:
	PortholeRenderPass(Renderer* client, PortholeCustomDrawApplication* app): RenderPass(client, "PortholeRenderPass"), myApplication(app) {}
	virtual void initialize();
	virtual void render(Renderer* client, const DrawContext& context);

private:
	PortholeCustomDrawApplication* myApplication;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class PortholeCustomDrawApplication: public EngineModule
{
public:
	PortholeCustomDrawApplication(): EngineModule("PortholeCustomDrawApplication")
	{ enableSharedData(); }

	virtual void initialize();
	virtual void initializeRenderer(Renderer* r);
	virtual void handleEvent(const Event& evt);

	void commitSharedData(SharedOStream& out);
	void updateSharedData(SharedIStream& in);

private:
	SceneManager* mySceneManager;

	float myYaw;
	float myPitch;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeRenderPass::initialize()
{
	RenderPass::initialize();

	PortholeService::createAndInitialize(4080, "porthole/portholeCustomDraw.xml", "porthole/porthello.css");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeRenderPass::render(Renderer* client, const DrawContext& context)
{
	if(context.task == DrawContext::OverlayDrawTask)
	{
		DrawInterface* di = client->getRenderer();

		di->beginDraw2D(context);
		di->drawRect(Vector2f(10, 10), Vector2f(100, 100), Color::Red);
		di->endDraw();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeCustomDrawApplication::initializeRenderer(Renderer* r) 
{ 
	RenderPass* rp = new PortholeRenderPass(r, this);
	rp->setCameraMask(PORTHOLE_CAMERA_MASK);
	r->addRenderPass(rp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeCustomDrawApplication::initialize()
{
	// Initialize a scene using cyclops
	// Create and initialize the cyclops scene manager.
	mySceneManager = SceneManager::createAndInitialize();

	SphereShape* sphere = new SphereShape(mySceneManager, 0.1f);
	sphere->setEffect("colored -d red");
	sphere->setPosition(0, 1.57, -3.24);

	PlaneShape* plane = new PlaneShape(mySceneManager, 4, 4);
	plane->setEffect("colored -d green");
	plane->pitch(-90 * Math::DegToRad);
	plane->setPosition(0, 0, -5);

	// Setup a light for the scene.
	Light* light = new Light(mySceneManager);
	light->setEnabled(true);
	light->setPosition(Vector3f(0, 50, 0));
	light->setColor(Color(1.0f, 1.0f, 0.7f));
	light->setAmbient(Color(0.1f, 0.1f, 0.1f));
	mySceneManager->setMainLight(light);

	ShadowSettings set;
	set.shadowsEnabled = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeCustomDrawApplication::handleEvent(const Event& evt)
{
	if(evt.getServiceType() == Service::Pointer)
	{
		// Normalize the mouse position using the total display resolution, 
		// then multiply to get 180 degree rotations
		DisplaySystem* ds = getEngine()->getDisplaySystem();
		Vector2i resolution = ds->getCanvasSize();
		myYaw = (evt.getPosition().x() / resolution[0]) * 180;
		myPitch = (evt.getPosition().y() / resolution[1]) * 180;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeCustomDrawApplication::commitSharedData(SharedOStream& out)
{
	out << myYaw << myPitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PortholeCustomDrawApplication::updateSharedData(SharedIStream& in)
{
 	in >> myYaw >> myPitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
	Application<PortholeCustomDrawApplication> app("portholeCustomDraw");
    return omain(app, argc, argv);
}
