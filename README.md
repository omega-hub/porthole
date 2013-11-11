omegalib offers support for HTML5-capable browsers interactions. Specifically, the interface is multi-touchable, in order to support most of the hand helded devices, such as iPads, iPhones, Android tablets, and Android smartphones.

Moreover, the interfaces are tailored on application needs and on devices specifications, without writing any Javascript or JQuery code, but only through an XML file and an, optional, CSS file. 

An overview of the system implementation is given in the following figure.

<p align="middle"><img src="http://uic-evl.github.io/omegalib/Porthole/porthole_overview.jpg" width="500"/></p>

Basically, the Porthole Service creates a Server in a separate thread, that handles all the incoming connections. But all the steps between the XML description and the actual management of the different devices, including camera streaming and manipulations, are transparent to the application developer.

## Porthole API
