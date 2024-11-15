// ------------------ General --------------------

/*
* <ESP-IP-ADDRESS>[/?name=name-value]
*
* // Fields
* name-value : everything
*/

// Defaults
var deviceName = "💗 Controleur de coeur 💗";


var queryString = decodeURIComponent(window.location.search); //parsing
queryString = queryString.substring(1); 
var queries = queryString.split("="); 
if(queries[0] == "name" && queries[1] != "")
  deviceName = queries[1];

document.getElementById("header").textContent = deviceName;


// ------------------ WebSocket ----------------------
var gateway = "ws://"+window.location.hostname+"/ws";
var websocket;

window.addEventListener('load', function(event){
  setTimeout(function(){
    initWebSocket();
  },1000);
});

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 1000);
}

function onMessage(event) {
  console.log(event.data);
  var message = event.data;
  if(message.includes("s")){
    var value = event.data.slice(1);
    document.getElementById("speed-text").textContent = value;
  }
  if(message.includes("m") && buttons.length > 0){    
    selectModeButton(document.getElementById(message));
  }
  if(message.includes("c")){
    trigger = false;
    var valueStr = event.data.slice(1);
    var valueInt = parseInt(valueStr);
    var valueHexStr = valueInt.toString(16);
    //colorPicker.color.hexString = "#"+valueHexStr;
    trigger = true;
  }
}


// -------------- COLOR PICKER --------------------
var trigger = true;

var colorPicker = new window.iro.ColorPicker("#picker", {
  width: 300,
});

colorPicker.on('color:change', onColorChange);

// color:change callbacks receive the current color
function onColorChange(color){
  if(trigger){
    let g = color.rgb.r;
    let r = color.rgb.g;
    let b = color.rgb.b;
    let colorDec = g*256 + r*65536 + b*1;
    websocket.send("c"+colorDec.toString());
  }
}


// ------------- SPEED -----------------
document.getElementById("su").addEventListener('click', function(){
  websocket.send("su");
});

document.getElementById("sd").addEventListener('click', function(){
  websocket.send("sd");
});


// ------------ MODES --------------------
var lastChangeMode = Date.now();
var buttons = [];

window.addEventListener('load', function(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("modes-container").innerHTML = this.responseText;
    }
  };
  xhttp.open('GET', '/modes', true);
  xhttp.send();  

  setTimeout(function(){
    var children = document.getElementById("modes-container").childNodes;
    var j = 0;
    for(var i = 0; i < children.length; i++){
      if(children[i].tagName == "BUTTON"){
        buttons[j] = children[i];
        j++;
      }
    }
    for(var i = 0; i < buttons.length; i++){
      buttons[i].addEventListener('click', function(event){
        if(Date.now() - lastChangeMode >= 1000){
          lastChangeMode = Date.now();
          websocket.send(event.currentTarget.id);
        }
      });
    }
  }, 1000);
});

function resetButtonsBorder(){
  for(var i = 0; i < buttons.length; i++){
    buttons[i].style.border = "1px solid #282828";
  }
}

function selectModeButton(button){
  resetButtonsBorder();
  button.style.border = "2px solid orange";
}
