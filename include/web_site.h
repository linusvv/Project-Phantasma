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
        <div>Check bottom-right corner when connected to WiFi</div>
    </div>
    
    <div style="margin-top: 20px; padding: 20px; background: #fff; border-radius: 10px;">
        <h3>📶 WiFi Setup (For Voice)</h3>
        <input type="text" id="ssid" placeholder="SSID (Name)" style="padding:10px; width:70%; border:1px solid #ccc; border-radius:4px; margin:5px;">
        <input type="password" id="pass" placeholder="Password" style="padding:10px; width:70%; border:1px solid #ccc; border-radius:4px; margin:5px;">
        <br>
        <button onclick="connectWifi()" style="background:#8e44ad">Connect Router</button>
        <div id="wifi-status" style="margin-top:10px; color:#7f8c8d; font-size:14px;">Not Connected</div>
        <div style="font-size:12px; color:#95a5a6; margin-top:5px;">After connecting, use <b>http://ghostarm.local</b> on your phone.</div>
    </div>
  </div>

  <!-- VOICE OVERLAY (GLOBAL) -->
  <div id="voice-overlay" style="display:none; position:fixed; bottom:20px; right:20px; z-index:999; background:white; padding:15px; border-radius:15px; box-shadow:0 0 15px rgba(0,0,0,0.2); text-align:center;">
    <button id="btn-mic" class="mic-btn">🎤</button>
    <div id="voice-status" style="margin-top:5px; font-size:12px; color:#7f8c8d; white-space:nowrap;">Hold to Speak</div>
    <div onclick="showVoiceHelp()" style="margin-top:10px; color:#3498db; cursor:pointer; font-size:12px; text-decoration:underline;">? Commands</div>
  </div>

  <!-- HELP POPUP -->
  <div id="voice-help" class="modal" style="display:none; position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(0,0,0,0.5); z-index:1000; align-items:center; justify-content:center;">
    <div style="background:white; padding:20px; border-radius:10px; max-width:90%; width:350px; text-align:left;">
        <h3>🎤 Voice Commands</h3>
        <ul style="padding-left:20px; line-height:1.6; color:#2c3e50;">
            <li><b>"Open Hand" / "Drop"</b> - Open Gripper</li>
            <li><b>"Close Hand" / "Grab"</b> - Close Gripper</li>
            <li><b>"Mode Controller"</b> - Switch to ESP-NOW</li>
            <li><b>"Mode Web"</b> - Switch to Web Control</li>
            <li><b>"Mode Script"</b> - Switch to Script Mode</li>
            <li><b>"Start Recording"</b> - Record Motion (Mode 0)</li>
            <li><b>"Stop Recording"</b> - Stop Record (Mode 0)</li>
            <li><b>"Play Recording"</b> - Replay Motion (Mode 0)</li>
            <li><b>"Play/Run Demo Hello"</b> - Wave Hand</li>
            <li><b>"Play/Run Demo Dance"</b> - Dance</li>
        </ul>
        <button onclick="document.getElementById('voice-help').style.display='none'" style="width:100%; margin-top:10px;">Close</button>
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
