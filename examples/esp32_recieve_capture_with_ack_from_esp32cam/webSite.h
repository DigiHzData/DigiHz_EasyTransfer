const char indexHtmlEn[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <title>ESP32-Cam image capture to ESP32</title>
    <meta charset=UTF-8>
    <!--meta http-equiv="X-UA-Compatible" content="IE=edge"-->
    <!--meta http-equiv="X-UA-Compatible" content="IE=EmulateIE10"-->
    <meta name="viewport" content="width=device-width, initial-scale=0.8">
    <style>
      body{
        min-width:280px;
        padding:10px;
        font-size:14px;
        background-color:lightgrey;
      }

      #overlayWrapper{
        height:100%;
        width:100%;
        position:fixed;
        z-index:1;
        top:0;
        left:0;
        background-color:rgb(0,0,0);
        background-color:rgba(0,0,0, 0.9);
        overflow-x:hidden;
      }

      #overlayContent{
        position:relative;
        top:25%;
        width:100%;
        text-align:center;
        font-size:36px;
        color:#818181;
      }

      #title{
        font-size:18px;
        text-align:center;
      }

      #errorWrapper{
        display:none;
        padding:5px;
        margin-top:10px;
        border:2px solid black;
        background-color:red;
      }

      #errorMessage{
        color:white;
      }

      #flashLedMainWrapper{
        margin-top:5px;
        padding:5px;
        border:1px solid white;
      }

      #flashLedHeader{
        text-align:center;
      }

      #flashLedWarning{
        text-align:center;
        color:red;
      }

      #flashLedWrapper{
        text-align:center;
      }

      #buttonsWrapper{
        margin-top:10px;
        margin-bottom:10px;
      }

      #captureImageButton{
        padding:10px;
      }

      #rebootEsp32CamButton{
        padding:10px;
        float:right;
      }

      #progressBarWrapper{
        width:100%;
        border:1px solid white;
        background-color:#ddd;
      }

      #progressBar{
        width:0%;
        height:30px;
        background-color:#04AA6D;
        text-align:center;
        line-height:30px;
        color:white;
      }

      #status{
        margin-top:10px;
      }

      #chunksMainWrapper{
        display:none;
        margin-top:10px;
      }

      #imageWrapper{
        display:none;
        margin-top:20px;
        margin-bottom:20px;
        margin-left:auto;
        margin-right:auto;
        width:50%;
      }

      #imageWrapper img{
        width:100%;
      }

      #image{
        border:1px solid white;
      }
    </style>

  </head>
  <body>
    <div id="overlayWrapper">
      <div id="overlayContent"></div>
    </div>

    <div id="title">ESP32-Cam image capture to ESP32</div>
    <div id="errorWrapper">
      <div id="errorMessage"></div>
    </div>

    <div id="flashLedMainWrapper">
      <div id="flashLedHeader">Flash led</div>
      <div id="flashLedWarning">Warning:Do not look straight into the led!</div>
      <div id="flashLedWrapper">
        <input type="radio" id="flashLedOff" name="flashLed" value="0">
        <label for="flashLedOff">Off</label>
        <input type="radio" id="flashLed64ms" name="flashLed" value="64">
        <label for="flashLed64ms">64ms</label>
        <input type="radio" id="flashLed128ms" name="flashLed" value="128">
        <label for="flashLed128ms">128ms</label>
        <input type="radio" id="flashLed255ms" name="flashLed" value="255">
        <label for="flashLed255ms">255ms</label>
      </div>
    </div>

    <div id="buttonsWrapper">
      <button id="captureImageButton" onclick="requestImage()" title="Capture an image from the ESP32-Cam">Capture image</button>
      <button id="rebootEsp32CamButton" onclick="rebootEsp32Cam()" title="reboot the ESP32-Cam">Reboot the ESP32-Cam</button>
    </div>

    <div id="progressBarWrapper">
      <div id="progressBar">0%</div>
    </div>

    <div id="status">Status:<span id="statusMessage"></span></div>

    <div id="chunksMainWrapper">
      <div id="numChunksWrapper">Total chunks:<span id="numChunks"></span></div>
      <div id="chunksTotalBytesWrapper">Total bytes:<span id="chunksTotalBytes"></span></div>
      <div id="chunkIdWrapper">Chunk:<span id="chunkId"></span></div>
      <div id="chunkActualBytesWrapper">Chunk bytes:<span id="chunkActualBytes"></span></div>
      <div id="imageWidthWrapper"><span id="imageWidth"></span></div>
      <div id="imageHeightWrapper"><span id="imageHeight"></span></div>
      <div id="timeToRecieveImageWrapper"><span id="timeToRecieveImage"></span></div>
    </div>

    <div id="imageWrapper">
      <img id="image" src="">
    </div>

    <script>
      var Socket;

      var numChunks = 0;
      var chunksTotalBytes = 0;
      var chunkId = 0;
      var chunkActualBytes = 0;

      var imageTemp = '';

      var captureImageButton = document.getElementById("captureImageButton");
      captureImageButton.disabled = true;
      var rebootEsp32CamButton = document.getElementById("rebootEsp32CamButton");
      rebootEsp32CamButton.disabled = true;

      var startTime;
      var endTime;

      var percentageProgressBar = 0;

      var errorWrapper = document.getElementById("errorWrapper");
      var chunksMainWrapper = document.getElementById("chunksMainWrapper");
      var imageWrapper = document.getElementById("imageWrapper");

      var flashLedRadioButtonDefault = document.getElementById("flashLedOff");
      flashLedRadioButtonDefault.checked = true;

      function prepareRequestImage(){
        errorWrapper.style.display = "none";
        chunksMainWrapper.style.display = "none";
        percentageProgressBar = 0;
        updateProgressBar(percentageProgressBar);
        imageTemp = '';
        imageWrapper.style.display = "none";
        numChunks = 0;
        chunksTotalBytes = 0;
        chunkId = 0;
        chunkActualBytes = 0;
      }

      function updateProgressBar(percentageProgressBar) {
        var elem = document.getElementById("progressBar");
        elem.style.width = percentageProgressBar + "%";
        elem.innerHTML = percentageProgressBar  + "%";
      }

      function requestImage(){
        prepareRequestImage();
        document.getElementById("imageWidth").innerHTML = "";
        document.getElementById("imageHeight").innerHTML = "";
        document.getElementById("timeToRecieveImage").innerHTML = "";
        captureImageButton.disabled = true;
        rebootEsp32CamButton.disabled = true;
        document.getElementById("statusMessage").innerHTML = "Requesting image...";
        var flashLedOnTime = document.querySelector('input[name="flashLed"]:checked').value;
        Socket.send("requestImage=" + flashLedOnTime);
      }

      function rebootEsp32Cam(){
        captureImageButton.disabled = true;
        rebootEsp32CamButton.disabled = true;
        document.getElementById("statusMessage").innerHTML = "Requesting to reboot ESP32-Cam...";
        Socket.send("rebootEsp32Cam");
      }

      function showImageCaptured(){
        img = document.getElementById('image');
        img.src = 'data:image/jpg;base64,' + window.btoa(imageTemp);
        img.onload = function() {
          document.getElementById("imageWidth").innerHTML = "Image width:" + img.naturalWidth + " pixels.";
          document.getElementById("imageHeight").innerHTML = "Image height:" + img.naturalHeight + " pixels.";
        }
        captureImageButton.disabled = false;
        rebootEsp32CamButton.disabled = false;
        document.getElementById("statusMessage").innerHTML = "Idle";
        document.getElementById("timeToRecieveImage").innerHTML = "Time to recieve image:" + (parseInt(endTime) - parseInt(startTime)) + " milli seconds.";
        imageWrapper.style.display = "block";
      }

      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.binaryType = "arraybuffer";
      Socket.onopen = function(event) {
        document.getElementById("statusMessage").innerHTML = "Websocket ready.";
        setTimeout(() => {
          captureImageButton.disabled = false;
          rebootEsp32CamButton.disabled = false;
          document.getElementById("statusMessage").innerHTML = "Idle";
        }, 2000);
      };
      Socket.onmessage = function(event){
        if (typeof event.data === "string" ) {
          var str = event.data;
          var res = str.split("=", 2);
          if (res[0] === "numChunks"){
            startTime = performance.now();
            numChunks = res[1];
            document.getElementById("numChunks").innerHTML = numChunks;
          } else if (res[0] === "chunksTotalBytes"){
            chunksTotalBytes = res[1];
            document.getElementById("chunksTotalBytes").innerHTML = chunksTotalBytes;
          } else if (res[0] === "chunkId"){
            chunkId = res[1];
            chunksMainWrapper.style.display = "block";
            document.getElementById("chunkId").innerHTML = chunkId;
            percentageProgressBar = parseInt((chunkId / numChunks) * 100);
            updateProgressBar(percentageProgressBar);
          } else if (res[0] === "chunkActualBytes"){
            chunkActualBytes = res[1];
            document.getElementById("chunkActualBytes").innerHTML = chunkActualBytes;
          } else if (res[0] === "chunksRecieved"){
            if (res[1] === "true"){
              endTime = performance.now();
              showImageCaptured();
            }
          } else if (res[0] === "acquiringImage"){
            if (res[1] === "true"){
              document.getElementById("statusMessage").innerHTML = "Acquiring image from ESP32-Cam...";
            } else {
              document.getElementById("statusMessage").innerHTML = "Acquiring image from ESP32-Cam failed!";
              setTimeout(() => {
                captureImageButton.disabled = false;
                rebootEsp32CamButton.disabled = false;
              }, 1000);
            }
          } else if (res[0] === "rebootingEsp32Cam"){
            if (res[1] === "true"){
              document.getElementById("statusMessage").innerHTML = "Rebooting ESP32-Cam...";
              setTimeout(() => {
                captureImageButton.disabled = false;
                rebootEsp32CamButton.disabled = false;
                document.getElementById("statusMessage").innerHTML = "Idle";
              }, 1000);
            } else {
              document.getElementById("statusMessage").innerHTML = "Acquiring reboot from ESP32-Cam failed!";
              setTimeout(() => {
                captureImageButton.disabled = false;
                rebootEsp32CamButton.disabled = false;
              }, 1000);
            }
          } else if (res[0] === "chunkRecieveFailure"){
            document.getElementById("errorMessage").innerHTML = "ERROR:TIMEOUT RECIEVING CHUNK!<br>ERROR:Did not catch the next chunk packet in time. (packet lost or skipped)<br>Skipped after chunkId:" + chunkId + "<br>Maybe you should increase the timeoutRecieveNextChunk value?";
            prepareRequestImage();
            errorWrapper.style.display = "block";
            captureImageButton.disabled = false;
            rebootEsp32CamButton.disabled = false;
            document.getElementById("statusMessage").innerHTML = "Idle";
          } else if (res[0] === "captureFrameAndCopyError"){
            if (res[1] === "true"){
              document.getElementById("statusMessage").innerHTML = "ERROR:Could not copy frame buffer on ESP32-Cam!";
              prepareRequestImage();
              setTimeout(() => {
                captureImageButton.disabled = false;
                rebootEsp32CamButton.disabled = false;
                document.getElementById("statusMessage").innerHTML = "Idle";
              }, 2000);
            }
          } else if (res[0] === "cameraInitializingError"){
            if (res[1] === "true"){
              document.getElementById("statusMessage").innerHTML = "ERROR:Camera Initializing failed on ESP32-Cam!";
              prepareRequestImage();
              setTimeout(() => {
                captureImageButton.disabled = false;
                rebootEsp32CamButton.disabled = false;
                document.getElementById("statusMessage").innerHTML = "Idle";
              }, 2000);
            }
          } else if (res[0] === "toManyConnections"){
            if (res[1] === "true"){
              document.getElementById("overlayContent").innerHTML = "To many connections!<br>Only 1 client allowed!";
            } else {
              document.getElementById("overlayContent").innerHTML = "Loading... Please wait.";
              setTimeout(() => {
                var fadeTarget = document.getElementById("overlayWrapper");
                var fadeEffect = setInterval(function () {
                  if (!fadeTarget.style.opacity) {
                      fadeTarget.style.opacity = 1;
                  }
                  if (fadeTarget.style.opacity > 0) {
                    fadeTarget.style.opacity -= 0.1;
                  } else {
                    clearInterval(fadeEffect);
                    fadeTarget.style.display = "none";
                  }
                }, 50);
              }, 1000);
            }
          }
        }
        if (event.data instanceof ArrayBuffer ){
          var bytes = new Uint8Array(event.data);
          var binary = '';
          var len = bytes.byteLength;
          for (var i = 0; i < len; i++) {
            binary += String.fromCharCode(bytes[i]);
          }
          imageTemp += binary;
        }
      };
      Socket.onerror = function(event) {
        document.getElementById("statusMessage").innerHTML = "Websocket failed, reload the web page.";
      };
    </script>
  </body>
  </html>
)=====";
