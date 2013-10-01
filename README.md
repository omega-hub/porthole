omegalib offers support for HTML5-capable browsers interactions. Specifically, the interface is multi-touchable, in order to support most of the hand helded devices, such as iPads, iPhones, Android tablets, and Android smartphones.

Moreover, the interfaces are tailored on application needs and on devices specifications, without writing any Javascript or JQuery code, but only through an XML file and an, optional, CSS file. 

An overview of the system implementation is given in the following figure.

<p align="middle"><img src="http://uic-evl.github.io/omegalib/Porthole/porthole_overview.jpg" width="500"/></p>

Basically, the Porthole Service creates a Server in a separate thread, that handles all the incoming connections. But all the steps between the XML description and the actual management of the different devices, including camera streaming and manipulations, are transparent to the application developer.

## Introduction

If you want your application to expose an interface to all the HTML5-capable browsers, and, in particular, to most of the hand helded devices, you need to provide the following files:

- A *.xml* file, that specifies the elements and their disposition for the different types of devices
- A *.css* file, that may contain any CSS style you want to apply to the final interface; it is used to tweak the style of the elements, but it may be empty if you are satisfied with the basic style provided

Finally, you need to add the Porthole Service in your C++ or Python application.

In the following sections we go deeper into:
- Create the xml file
- Create the css file
- Start the Porthole Service

## Interfaces description: .xml file

The main idea of how to compose the XML file is given in the following figure.

<p align="middle"><img src="http://uic-evl.github.io/omegalib/Porthole//xml_schema.jpg" width="500"/></p><p align="middle">
The xml file composition.
</p>

the XML is divided into two main branches. The first one specifies the number and type of GUI elements, that represent the main “blocks” of the GUI. The second branch is responsible for mapping the main blocks to the different types of devices that may want to connect to the application.
In particular, the gui_elements part contains how many global interface blocks as needed, that are divided into two type (a third, a GoogleMaps canvas is in testing).

#### HTML element

This type of element can include any html piece of code, such as a group of control buttons, sliders, and so on. JQuery Mobile support is available, thus one could for example specify Buttons, Sliders, Radio Buttons, and so on, simply using JQuery Mobile classes.
These elements represent an interface building block.

Notice that JQuery Mobile style will apply to all your elements, thus, if you do NOT want any style on a particular html tag (and all its children), add the following attribute:

{{{data-role:"none"}}}

#### CAMERA element

This type of element represents a camera stream element, that is a rectangular space, with touch gestures enabled, that visualizes a camera inside the 3D world.
A camera could be of two types:

- Custom, that is a camera that can freely move inside the world, based on interface interactions
- Default, that is the default 3D camera inside the CAVE; thus, any interactions in the Virtual Environment is reflected in the interface canvas, and viceversa.

#### Javascript events handlers binding

In the HTML elements, events handlers could be binded to application functionalities using either:

- C++ functions binding: developer could bind an event handler to a C++ function in his application
- Python code injection: developer could insert into the event handler code a complete piece of code in Python language

This functionalities is achieved using a dynamic creation of Javascript functions that uses WebSockets to send which event has been triggered and which C++ function or Python code to execute. In the following XML Description example, Python code has been injected directly into a button Javascript events handlers in order to move the camera (downward in this case).

```python
onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[1] -= 0.025; camera.setPosition(position);"  
```


Finally, the gui disposition part can be composed by any number of elements, with two required attributes each: __minWidth__ and __minHeight__. This means that the interface type is mapped to the closest matching case.

Moreover, each element must define two type of interfaces, a __Portrait__ and a __Landscape__ interface (which could be the same though). These elements have a required attribute that is __layout__, which could be __vertical__ or __horizontal__: the former, disposes the elements from top to bottom, the latter, disposes the elements from left to right.

Inside each one of these two elements, any gui_element previously declared could be inserted.

## Interfaces style: .css file

The usage of the .css file is straightforward: it is attached to the final html file that is run on the device, in order to be able to apply custom fine-grained style to the interface. An example is given in the Hello Porthole section.

## Start the service

This part differes either if the application is written in C++ or in Python.

#### C++

If the application is written in C++, you must have a free port number, the xml file path, the css file path and a PortholeFunctionsBinder binder object.

The function binder is used if you want to map any Javascript event handler to a C++ function (remember that C++ language has no Reflection, so the map must be explicit). In the following example, 5 functions are added.

```python
	// Functions Bind
	PortholeFunctionsBinder* binder = new PortholeFunctionsBinder();
	binder->addFunction("up(event)", &up);
	binder->addFunction("down(event)", &down);
	binder->addFunction("left(event)", &left);
	binder->addFunction("right(event)", &right);
	binder->addFunction("zoomSlider(event)", &zoomSlider);

	// Porthole initialize the porthole service
	PortholeService* service = new PortholeService();
	ServiceManager* svcManager = SystemManager::instance()->getServiceManager();
	svcManager->addService(service);

	string fullPath_xml;
	DataManager::findFile("porthole/porthello.xml", fullPath_xml);

	string fullPath_css;
	DataManager::findFile("porthole/porthello.css", fullPath_css);

	service->start(4080, (char*)fullPath_xml.c_str(), (char*)fullPath_css.c_str(), binder);

```


The functions definition is:

```python
void up(PortholeEvent &ev){
	Vector3f myPosition = ev.sessionCamera->camera->getPosition();
	myPosition[1] += 0.025f;
	ev.sessionCamera->camera->setPosition(myPosition);
}

void down(PortholeEvent &ev){
	Vector3f myPosition = ev.sessionCamera->camera->getPosition();
	myPosition[1] -= 0.025f;
	ev.sessionCamera->camera->setPosition(myPosition);
}

void left(PortholeEvent &ev){
	Vector3f myPosition = ev.sessionCamera->camera->getPosition();
	myPosition[0] -= 0.025f;
	ev.sessionCamera->camera->setPosition(myPosition);
}

void right(PortholeEvent &ev){
	Vector3f myPosition = ev.sessionCamera->camera->getPosition();
	myPosition[0] += 0.025f;
	ev.sessionCamera->camera->setPosition(myPosition);
}
void zoomSlider(PortholeEvent &ev){
	Vector3f myPosition = ev.sessionCamera->camera->getPosition();
	myPosition[2] = atoi(ev.value.c_str())/100;
	ev.sessionCamera->camera->setPosition(myPosition);
}
```


Now, if we have a button that states, for example:

```python
onmousedown="up(event)"  
```


each time a button is clicked in the browser, the C++ up(PortholeEvent &ev) function is called.

#### Python

If you want to add Porthole Service for your Python application, you just need to call the following Python function.

```python
PortholeService.createAndInitialize(port_number,path_to_xml,path_to_css)
```


The arguments are:
- port_number: a free TCP port
- path_to_xml: a string that contains the path to you .xml file
- path_to_css: a string that contains the path to your .css file

Now, if we have a button that states, for example:

```python
onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[2] -= 0.025; camera.setPosition(position);"  
```


When the botton is clicked, the Python code is passed to the Python interpreter and executed. In this case, the camera will move along the Z axis.

The Javascript variables exposed through tokens are:
- %id%, the camera id associated with the user session
- %value%, the value of the field that the event is targeting
- %key%, the keyboard key pressed in case of keyboard events
- %btn%, the mouse button number in case of mouse events
- %event%, a string containing the name of the event triggered

Basically, those tokens are replaced with Javascript event variables before passing the Python code to the interpreter.

## Hello Porthole!

In this section, a complete example is given. 
In particular, this example is taken from Porthello, a project that you should have checked out with omegalib.

#### Porthello.xml

In this example, various HTML interface blocks and a CAMERA block are created inside the gui_elements portion.
Then, in the gui_disposition, 4 types of interfaces has been created:
- small
- medium
- large
- xlarge

For each one of them, both a portrait and a landscape layout has been created.

The Javascript events handlers has been implemented using Python code injection. 

```python
<gui>
  
  <gui_elements>
    
    <gui_element id="move_ctrl" type="HTML">
      <div class="div-aligned">
        <fieldset  data-type="horizontal" class="">
          <input type="button" data-role="button" onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[1] += 0.025; camera.setPosition(position);" data-shadow="false" data-inline="true" data-corner="false" data-iconpos="notext" data-icon="arrow-u" data-continuous="true" class="ui-btn ui-btn-icon-notext"/>
          <input type="button" data-role="button" onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[1] -= 0.025; camera.setPosition(position);" data-shadow="false" data-inline="true" data-corner="false" data-iconpos="notext" data-icon="arrow-d" data-continuous="true" class="ui-btn ui-btn-icon-notext"/>
          <input type="button" data-role="button" onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[0] -= 0.025; camera.setPosition(position);" data-shadow="false" data-inline="true" data-corner="false" data-iconpos="notext" data-icon="arrow-l" data-continuous="true" class="ui-btn ui-btn-icon-notext"/>
          <input type="button" data-role="button" onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[0] += 0.025; camera.setPosition(position);" data-shadow="false" data-inline="true" data-corner="false" data-iconpos="notext" data-icon="arrow-r" data-continuous="true" class="ui-btn ui-btn-icon-notext"/>
        </fieldset>
      </div>
    </gui_element>
    
    <gui_element id="camera" type="camera_stream" camera="custom"/>

    <gui_element id="zoom_slider" type="HTML">
      <div>
          <input type="range" name="slider" id="slider" value="0" min="-800" max="800" data-highlight="true" onchange="camera=getCameraById(%id%); position=camera.getPosition(); position[2] = float(%value%)/100; camera.setPosition(position);"/>
      </div>
    </gui_element>

    <gui_element id="zoom_ctrl" type="HTML">
      <div class="div-aligned">
        <fieldset  data-type="horizontal" class="">
          <input type="button" data-role="button" onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[2] -= 0.025; camera.setPosition(position);" data-shadow="false" data-inline="true" data-corner="false" data-iconpos="notext" data-icon="plus" data-continuous="true" class="ui-btn ui-btn-icon-notext"/>
          <input type="button" data-role="button" onmousedown="camera=getCameraById(%id%); position=camera.getPosition(); position[2] += 0.025; camera.setPosition(position);" data-shadow="false" data-inline="true" data-corner="false" data-iconpos="notext" data-icon="minus" data-continuous="true" class="ui-btn ui-btn-icon-notext"/>
        </fieldset>
      </div>
    </gui_element>

    <gui_element id="color_ctrl" type="HTML">
      <div class="div-aligned">
        <input type="hidden" id="color_label" name="color_label" value="#000000" data-role="none" data-inline="true" onchange="print('%value%');"/>
        <div id="color_picker" data-role="none" data-inline="true" ></div>
      </div>
    </gui_element>
    
  </gui_elements>
  
  <gui_disposition>

    <small minWidth="0" minHeight="0">
      <portrait layout="vertical">
        <move_ctrl width="100%" height="10%" />
        <camera width="100%" height="90%" />
      </portrait>
      <landscape layout="vertical">
        <camera width="100%" height="100%" />
      </landscape>
    </small>
    
    <medium minWidth="320" minHeight="470">
      <portrait layout="vertical">
        <move_ctrl width="100%" height="10%" />
        <camera width="100%" height="90%" />
      </portrait>
      <landscape layout="vertical">
        <camera width="100%" height="100%" />
      </landscape>
    </medium>
    
    <large minWidth="480" minHeight="640">
      <portrait layout="vertical">
        <move_ctrl width="100%" height="5%" />
        <zoom_ctrl width="100%" height="5%" />
        <camera width="100%" height="90%" />
      </portrait>
      <landscape layout="horizontal">
        <camera width="70%" height="100%" />
        <move_ctrl width="10%" height="100%" />
        <color_ctrl width="20%" height="100%" />
      </landscape>   
    </large>
    
    <xlarge minWidth="720" minHeight="960">
      <portrait layout="vertical">
        <move_ctrl width="100%" height="5%" />
        <zoom_ctrl width="100%" height="5%" />
        <camera width="100%" height="90%" />
      </portrait>
      <landscape layout="horizontal">
        <camera width="70%" height="100%" />
        <move_ctrl width="10%" height="100%" />
        <color_ctrl width="20%" height="100%" />
      </landscape>
    </xlarge>
    
  </gui_disposition>
  
</gui>
```


You can play with number, names and sizes of the elements in each interface and see the difference in the browser, just by resizing your browser windows accordingly with the interface size and type (portrait vs landscape: i.e. (width > height)? ) 

#### Porthello.css

In this example, the .css file is quite simple. We just want to have a style for inline and centered contents, hide the input field of the color picker and set the width of our zooming slider.

```python
.div-aligned {
    margin: 0 auto;
    text-align: center;
    position:relative;
}
.div-aligned fieldset {
    display: inline;
}

#color_picker 
{
    margin: 0 auto;
    text-align: center;
    position:relative;
}

input.ui-slider-input {
    display : none !important;
}

div.ui-slider{
    width:95%;
}
```


Here, you can define your style for any content of the final html file.

#### Result

The result of the defined interface in Porthello xlarge landscape and in Brain2 xlarge portrait is given.

<p align="middle"><img src="http://uic-evl.github.io/omegalib/Porthole/porthello_porthole.jpg" width="500"/></p><p align="middle">
The porthello interface.
</p>

<p align="middle"><img src="http://uic-evl.github.io/omegalib/Porthole/brain2_porthole.jpg" width="300"/></p><p align="middle">
The Brain2 interface.
</p>

## Conclusion

As you can see from the previous example, now it's quite straightforward to have your HTML5 dynamic interface, simply using an xml file and, optionally, a css file.
You don't have to bother on handling connections nor implementing the message passing protocol between client and server: just define your beautiful dynamic interface!
