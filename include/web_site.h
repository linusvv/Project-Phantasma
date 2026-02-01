#ifndef WEB_SITE_H
#define WEB_SITE_H

#include "web_css.h"
#include "web_js.h"

const char PAGE_HTML_HEAD[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot Arm Controller</title>
)rawliteral";

const char PAGE_HTML_BODY[] PROGMEM = R"rawliteral(
</head>
<body>
  <h2>ROBOT ARM MONITOR</h2>

  <!-- MODE SELECTION MENU -->
  <div id="menu" class="container visible">
    <h3>Select Mode</h3>
    <button onclick="setMode(0)">🎮 Controller Mode</button>
    <button onclick="setMode(1)">🌐 Web Control Mode</button>
    <button onclick="setMode(2)">📜 Script Mode</button>
    
    <div style="margin-top: 30px; padding: 20px; background: #fff; border-radius: 10px;">
        <h3>🎤 Voice Control</h3>
        <button id="btn-mic" class="mic-btn">🎤</button>
        <div id="voice-status" style="margin-top:10px; color:#7f8c8d;">Hold button to speak</div>
    </div>
  </div>

  <!-- CONTROLLER MONITOR (MODE 0) -->
  <div id="mode0" class="container">
    <h3>Mode: ESP-NOW Controller</h3>
    
    <div class="slider-card">
      <h4>🔴 Motion Recorder</h4>
      <div id="rec-info" style="margin-bottom:10px; color:#7f8c8d;">Status: Ready</div>
      <div class="input-group">
        <button id="btn-rec" class="rec-btn" onclick="toggleRecord()">Start Recording</button>
        <button id="btn-play" class="secondary" onclick="togglePlayback()">▶ Playback</button>
      </div>
      <button onclick="downloadRecord()" style="width:90%">💾 Download Sequence</button>
    </div>

    <div class="data-box">
      <div><span class="data-label">X Axis</span><span id="dspX0">0.0</span> cm</div>
      <div><span class="data-label">Y Axis</span><span id="dspY0">0.0</span> cm</div>
      <div><span class="data-label">Z Axis</span><span id="dspZ0">0.0</span> cm</div>
      <div><span class="data-label">Pitch</span><span id="dspP0">0.0</span> &deg;</div>
    </div>
    <div id="monitor-sliders"></div>
    <button class="secondary" onclick="showMenu()">Back to Menu</button>
  </div>

  <!-- WEB CONTROL (MODE 1) -->
  <div id="mode1" class="container">
    <h3>Mode: Web Control</h3>
    <div id="ik-error" class="error">⚠️ TARGET UNREACHABLE</div>
    
    <div class="data-box">
      <div><span class="data-label">X Axis</span><span id="dspX1">0.0</span> cm</div>
      <div><span class="data-label">Y Axis</span><span id="dspY1">0.0</span> cm</div>
      <div><span class="data-label">Z Axis</span><span id="dspZ1">0.0</span> cm</div>
      <div><span class="data-label">Pitch</span><span id="dspP1">0.0</span> &deg;</div>
    </div>

    <div class="slider-card">
      <div class="label-row"><span>Inverse Kinematics Target</span></div>
      <div class="input-group">
        <div class="input-wrapper"><label>X</label><input type="number" id="ikX" step="0.1" value="15"></div>
        <div class="input-wrapper"><label>Y</label><input type="number" id="ikY" step="0.1" value="0"></div>
        <div class="input-wrapper"><label>Z</label><input type="number" id="ikZ" step="0.1" value="10"></div>
        <div class="input-wrapper"><label>Pitch</label><input type="number" id="ikP" step="1" value="0"></div>
      </div>
      <button onclick="sendIK()">Move to XYZ</button>
    </div>

    <div id="control-sliders"></div>
    <button class="secondary" onclick="showMenu()">Back to Menu</button>
  </div>

  <!-- SCRIPT MODE (MODE 2) -->
  <div id="mode2" class="container">
    <h3>Mode: Upload & Play Script</h3>
    
    <div class="slider-card">
      <h4>📂 Upload Sequence .CSV</h4>
      <input type="file" id="scriptFile" accept=".csv,.txt,text/csv,text/plain" style="display:none">
      <label for="scriptFile" class="file-label">📂 Choose File</label>
      <div id="file-chosen" style="margin: 10px 0; color:#7f8c8d; font-size: 0.9em;">No file chosen</div>
      <button onclick="uploadScript()">Upload & Play</button>
      <div id="upload-status" style="margin-top:10px; color:#2980b9;"></div>
    </div>

    <div class="slider-card">
      <h4>Pre-loaded Demos</h4>
      <div class="input-group" style="flex-wrap: wrap;">
        <button onclick="loadDemo('hello')" style="background:#16a085">👋 Hello</button>
        <button onclick="loadDemo('picknplace')" style="background:#8e44ad">📦 Pick & Place</button>
        <button onclick="loadDemo('dancing')" style="background:#e67e22">💃 Dancing</button>
      </div>
      <div id="demo-status" style="margin-top:10px; color:#2980b9;"></div>
    </div>

    <br>
    <div id="play-controls" style="display:none;">
        <button id="btn-script-play" class="rec-btn" onclick="togglePlayback()">⏹ Stop Playback</button>
    </div>

    <button class="secondary" onclick="showMenu()">Back to Menu</button>
  </div>
</body>
</html>
)rawliteral";

#endif
