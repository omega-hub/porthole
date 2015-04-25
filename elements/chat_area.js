function messageReceived(text) {
    var p = document.createElement('p');
    p.innerHTML = text;
    chatArea = document.getElementById('chat_box');
    chatArea.appendChild(p);
    //chatArea.scrollTop = chatArea.scrollHeight;
}    

function submitOnEnter(inputElement, event) {
    if(event.keyCode == 13) {
        phCall("postMessage('{0}', %client_id%)", inputElement.value);
        inputElement.value = '';
    }
}