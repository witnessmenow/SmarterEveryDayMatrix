R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css" integrity="sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm" crossorigin="anonymous">
  </head>
  <body onload="setServerLocation()">
    <nav class="navbar navbar-dark bg-dark">
        <span class="navbar-brand h1">Smarter Every Day Matrix Display</span>
        <form class="form-inline">
            <div class="input-group">
              <div class="input-group-prepend">
                <span class="input-group-text" id="basic-addon1">Server:</span>
              </div>
              <input type="text" class="form-control" id="serverLocation">
              <button  type="button" class="btn btn-outline-success my-2 my-sm-0" onclick='setServerLocation()'>Set</button>
            </div>
          </form>
    </nav>
    <div class="container">
        <!-- Text Area-->
        <div class="row">
            <h3>Message:</h3>
        </div>
        <div class="row">
            <textarea class="form-control" id="messageBox" rows="3"></textarea>
        </div>
        <div class="row">
            <!--🪐🦕🦖🚀🌕🌙-->
            Add Emoji:
            <button type="button" style='margin: 0.5em; font-size: large;' class="btn btn-default" onclick='addEmojiToText("🪐")'>🪐</button>
            <button type="button" style='margin: 0.5em; font-size: large;' class="btn btn-default" onclick='addEmojiToText("🦕")'>🦕</button>
            <button type="button" style='margin: 0.5em; font-size: large;' class="btn btn-default" onclick='addEmojiToText("🦖")'>🦖</button>
            <button type="button" style='margin: 0.5em; font-size: large;' class="btn btn-default" onclick='addEmojiToText("🚀")'>🚀</button>
            <button type="button" style='margin: 0.5em; font-size: large;' class="btn btn-default" onclick='addEmojiToText("🌕")'>🌕</button>
            <button type="button" style='margin: 0.5em; font-size: large;' class="btn btn-default" onclick='addEmojiToText("🌙")'>🌙</button>
        </div>
        <div class="row">
            <div class="col-8">
                <button id="messageButton" type="button" class="btn btn-default" style="height: 100%; width: 100%" onclick='sendMessage()'>Scroll Message</button>
            </div>
            <div class="col-4">
                <button id="messageButton" type="button" class="btn btn-default" style="height: 100%; width: 100%" onclick='sendStaticMessage()'>Static</button>
            </div>
        </div>
    </div>
    <hr>
    <div class="container"> 
        <div class="row">
            <h3>Count Down:</h3>
        </div>
        <div class="row">
            <div class="col">
                <input type="number" class="form-control" id="countDownBox">
            </div>
            <div class="col">
                <button id="countDownButton" type="button" class="btn btn-default" style="height: 100%; width: 100%" onclick='sendCountDown()'>Start Count Down</button>
            </div>
        </div>
    </div>
    <hr>
    <div class="container"> 
        <div class="row">
            <h3>Goal:</h3>
        </div>
        <div class="row" style="margin-bottom: 1em;">
            <div class="col">
                <div class="input-group">
                    <div class="input-group-prepend">
                      <span class="input-group-text" id="currentLabel">Current:</span>
                    </div>
                    <input type="number" class="form-control" id="goalCurrent">
                  </div>
            </div>
            <div class="col">
                <div class="input-group">
                    <div class="input-group-prepend">
                      <span class="input-group-text" id="currentLabel">Out of:</span>
                    </div>
                    <input type="number" class="form-control" id="goalGoal">
                  </div>
            </div>
        </div>
        <div class="row" style="margin-bottom: 1em;">
            <div class="col">
                <div class="input-group">
                    <div class="input-group-prepend">
                      <span class="input-group-text" id="leftEmojiLabel">Left Emoji:</span>
                    </div>
                    <select style="font-size: large;" class="custom-select" id="leftEmoji">
                        <option selected value="0" >🦖</option>
                        <option value="1">🦕</option>
                        <option value="2">🚀</option>
                        <option value="3">🌕</option>
                        <option value="4">🌙</option>
                        <option value="5">🪐</option>
                        <option value="-1">  </option>
                      </select>
                  </div>
            </div>
            <div class="col">
                <div class="input-group">
                    <div class="input-group-prepend">
                        <span class="input-group-text" id="rightEmojiLabel">Right Emoji:</span>
                      </div>
                      <select style="font-size: large;" value="5" class="custom-select" id="rightEmoji">
                          <option value="0" >🦖</option>
                          <option value="1">🦕</option>
                          <option value="2">🚀</option>
                          <option value="3">🌕</option>
                          <option value="4">🌙</option>
                          <option selected value="5">🪐</option>
                          <option value="-1">  </option>
                        </select>
                    </div>
            </div>
        </div>
        <div class="row">
            <button id="goalButton" type="button" class="btn btn-default" style="height: 100%; width: 100%" onclick='sendGoal()'>Send Goal</button>
        </div>
    </div>
    <hr>
    </div>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.12.4/jquery.min.js"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>
    <script>

        var serverLocation = ''

        function setServerLocation() {
            var serverAddressBox = document.getElementById('serverLocation');
            serverLocation = serverAddressBox.value;
        }

        function addEmojiToText(emojiText){
            var messageBox = document.getElementById('messageBox');
            messageBox.value = messageBox.value + emojiText;
        }

      function sendMessage(){
        var messageBox = document.getElementById('messageBox');
        var message = messageBox.value;
        //$.post("message", message)
        var url = serverLocation + "message?msg=" + encodeURIComponent(message);
        $.ajax({"url": url})
        
      }

      function sendStaticMessage(){
        var messageBox = document.getElementById('messageBox');
        var message = messageBox.value;
        //$.post("message", message)
        var url = serverLocation + "static?msg=" + encodeURIComponent(message);
        $.ajax({"url": url})
        
      }

      function sendCountDown(){
        var countDownNumberBox = document.getElementById('countDownBox');
        var countDownValue = countDownNumberBox.value;
        //$.post("message", message)
        var url = serverLocation + "count?number=" + encodeURIComponent(countDownValue);
        $.ajax({"url": url})
        
      }

      function sendGoal(){
        var goalCurrent = document.getElementById('goalCurrent');
        var goalCurrentValue = goalCurrent.value;

        var goalGoal = document.getElementById('goalGoal');
        var goalGoalValue = goalGoal.value;

        var leftEmojiSelect = document.getElementById('leftEmoji');
        var leftEmojiIndex = leftEmojiSelect.value;

        var rightEmojiSelect = document.getElementById('rightEmoji');
        var rightEmojiIndex = rightEmojiSelect.value;

        var url = serverLocation 
                    + "goal?lower=" + encodeURIComponent(goalCurrentValue) 
                    + "&higher=" + encodeURIComponent(goalGoalValue) 
                    + "&firstEmojiIndex=" + encodeURIComponent(leftEmojiIndex) 
                    + "&secondEmojiIndex=" + encodeURIComponent(rightEmojiIndex);
        $.ajax({"url": url})
        
      }
    
    </script>
    <script> function makeAjaxCall(url){$.ajax({"url": url})}</script>
    <!--<script>
       document.addEventListener('keydown', function(event) {
          if(event.keyCode == 37) {
              //Left Arrow
              makeAjaxCall("left");            
          }
          else if(event.keyCode == 39) {
              //Right Arrow
              makeAjaxCall("right");   
          } else if(event.keyCode == 38) {
              //Up Arrow
              makeAjaxCall("forward");   
          } else if(event.keyCode == 40) {
              //Down Arrow
              makeAjaxCall("back");   
          }
      });

      document.addEventListener('keyup', function(event) {
          if(event.keyCode == 37 ||event.keyCode == 39 ) {
              //Left or Right Arrow
              makeAjaxCall("steerStop");            
          }
          else if(event.keyCode == 38 ||event.keyCode == 40 ) {
              //Up or Down Arrow
              makeAjaxCall("driveStop");            
          }
      });
    </script>-->
  </body>
</html>

)"