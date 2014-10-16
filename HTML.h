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
        for(int i = 0; i < eventsNumber; i++)
            if(stringToSearch.compare(events[i]) == 0)
                return true; // found
        return false; // not found
    }

};