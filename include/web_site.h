#ifndef WEB_SITE_H
#define WEB_SITE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot Arm Controller</title>
  <style>
    body { font-family: 'Segoe UI', Arial; text-align: center; background: #eef2f3; margin: 0; padding-bottom: 50px; }
    h2 { color: #2c3e50; margin-top: 20px; }
    button { padding: 15px 25px; font-size: 16px; margin: 10px; border: none; border-radius: 5px; cursor: pointer; background: #2980b9; color: white; transition: 0.3s; }
    button:hover { background: #3498db; }
    button.secondary { background: #95a5a6; }
    button.active { background: #27ae60; }
    
    .container { max-width: 600px; margin: 0 auto; display: none; }
    .visible { display: block; }
    
    .data-box {
      background: #fff; padding: 15px; margin: 15px auto;
      width: 90%; border-radius: 10px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.1);
      display: grid; grid-template-columns: 1fr 1fr; gap: 10px;
    }
    .data-item { font-size: 18px; font-weight: bold; color: #2980b9; }
    .data-label { font-size: 12px; color: #7f8c8d; display: block; }

    .slider-card {
      background: white; margin: 10px auto; padding: 15px;
      width: 90%; border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.05); text-align: left;
    }
    input[type=range] { width: 100%; height: 20px; accent-color: #2980b9; cursor: pointer; }
    .label-row { display: flex; justify-content: space-between; font-weight: 600; color: #34495e; margin-bottom: 5px; }

    .input-group { display: flex; gap: 10px; justify-content: center; margin: 10px 0; }
    .input-wrapper { display: flex; flex-direction: column; width: 60px; }
    .input-wrapper input { padding: 5px; text-align: center; border: 1px solid #ccc; border-radius: 4px; }
    
    .error { color: #c0392b; font-weight: bold; margin: 10px auto; display: none; border: 2px solid #c0392b; padding: 10px; border-radius: 5px; background: #fadbd8; width: 80%; }
    .rec-btn { background: #e74c3c; }
    .rec-active { animation: pulse 1.5s infinite; background: #c0392b !important; border: 2px solid white; }
    @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.05); } 100% { transform: scale(1); } }
  </style>
</head>
<body>
  <h2>ROBOT ARM MONITOR</h2>

  <!-- MODE SELECTION MENU -->
  <div id="menu" class="container visible">
    <h3>Select Mode</h3>
    <button onclick="setMode(0)">🎮 Controller Mode</button>
    <button onclick="setMode(1)">🌐 Web Control Mode</button>
    <button onclick="setMode(2)">📜 Script Mode</button>
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
      <input type="file" id="scriptFile" accept=".csv" style="margin: 10px 0;">
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

<script>
  const servoNames = ["Base", "Shoulder", "Elbow", "Wrist", "Gripper"];
  let currentMode = -1;
  let intervalId = null;
  let isDragging = false;
  let lastUpdate = 0;

  function init() {
    createSliders('monitor-sliders', true);  // Disabled sliders for monitor
    createSliders('control-sliders', false); // Enabled sliders for control
    
    // Global mouseup to catch drags that end outside the slider
    window.addEventListener('mouseup', () => { isDragging = false; });
    window.addEventListener('touchend', () => { isDragging = false; });
  }

  function createSliders(containerId, disabled) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';
    servoNames.forEach((name, i) => {
      let div = document.createElement('div');
      div.className = 'slider-card';
      // Add events to track dragging state
      const events = disabled ? '' : `
        oninput="updateServo(${i}, this.value)" 
        onmousedown="isDragging = true" 
        ontouchstart="isDragging = true"
        onchange="isDragging = false"
      `;
      
      div.innerHTML = `
        <div class="label-row"><span>${name}</span><span id="${containerId}-val${i}">50%</span></div>
        <input type="range" min="0" max="100" value="50" id="${containerId}-sld${i}" ${disabled ? 'disabled' : ''} ${events}>
      `;
      container.appendChild(div);
    });
  }

  function showMenu() {
    switchSection('menu');
    stopPolling();
  }

  function setMode(mode) {
    currentMode = mode;
    fetch('/set_mode?mode=' + mode)
      .then(r => {
        if(mode === 0) {
            switchSection('mode0');
            startPolling();
        } else if (mode === 1) {
            switchSection('mode1');
            startPolling(); // Poll to update UI with current pos (sync initially)
        } else if (mode === 2) {
            switchSection('mode2');
            startPolling(); // Also poll in script mode to show playback status
        }
      });
  }

  function switchSection(id) {
    document.querySelectorAll('.container').forEach(el => el.classList.remove('visible'));
    document.getElementById(id).classList.add('visible');
  }

  function updateServo(index, value) {
    // Update label locally immediately
    document.getElementById(`control-sliders-val${index}`).innerText = value + "%";
    // Send to server
    fetch(`/set_servo?index=${index}&value=${value}`);
  }

  function sendIK() {
    const x = document.getElementById('ikX').value;
    const y = document.getElementById('ikY').value;
    const z = document.getElementById('ikZ').value;
    const p = document.getElementById('ikP').value;
    fetch(`/set_xyz?x=${x}&y=${y}&z=${z}&p=${p}`);
  }

  function runScript(id) {
    fetch(`/run_script?id=${id}`);
  }

  function uploadScript() {
      const input = document.getElementById('scriptFile');
      if(input.files.length === 0) {
          alert("Please select a file first");
          return;
      }
      
      const file = input.files[0];
      const formData = new FormData();
      formData.append("file", file);

      document.getElementById('upload-status').innerText = "Uploading...";
      
      fetch('/upload_script', {
          method: 'POST',
          body: formData
      })
      .then(response => {
           if(response.ok) {
               document.getElementById('upload-status').innerText = "Upload Complete! Playing...";
               // Force switch to Mode 2 play state if needed
               startPolling(); 
           } else {
               document.getElementById('upload-status').innerText = "Upload Failed";
           }
      })
      .catch(err => {
          document.getElementById('upload-status').innerText = "Error: " + err;
      });
  }

  function loadDemo(name) {
    document.getElementById('demo-status').innerText = "Loading " + name + "...";
    fetch('/load_demo?name=' + name)
      .then(r => r.json())
      .then(data => {
         if(data.status === 'ok') {
            document.getElementById('demo-status').innerText = "Playing using " + data.steps + " steps";
            startPolling();
         } else {
            document.getElementById('demo-status').innerText = "Error loading demo";
         }
      })
      .catch(e => {
         document.getElementById('demo-status').innerText = "Connection Error";
      });
  }

  function toggleRecord() {
      const btn = document.getElementById('btn-rec');
      const action = btn.classList.contains('rec-active') ? 'stop' : 'start';
      fetch('/record?action=' + action);
  }

  function togglePlayback() {
      const btn = document.getElementById('btn-play');
      const action = btn.innerText.includes('Stop') ? 'stop' : 'play';
      fetch('/record?action=' + action);
  }

  function downloadRecord() {
      window.location.href = '/download';
  }

  function startPolling() {
    if(intervalId) clearInterval(intervalId);
    intervalId = setInterval(fetchData, 250);
  }

  function stopPolling() {
    if(intervalId) clearInterval(intervalId);
    intervalId = null;
  }

  function fetchData() {
    // Stop updating if user is interacting to prevent jumps
    if (isDragging) return;

    fetch('/state')
      .then(response => response.json())
      .then(data => {
        // Update Monitor (Mode 0)
        if(currentMode === 0) {
            document.getElementById('dspX0').innerText = data.x.toFixed(1);
            document.getElementById('dspY0').innerText = data.y.toFixed(1);
            document.getElementById('dspZ0').innerText = data.z.toFixed(1);
            document.getElementById('dspP0').innerText = data.p.toFixed(1);
            
            // Update Recording State (Controller Mode)
            const btnRec = document.getElementById('btn-rec');
            const btnPlay = document.getElementById('btn-play');
            const info = document.getElementById('rec-info');
            
            if(btnRec && btnPlay && info) {
                // ... (previous logic for Mode 0) ...
                 if(data.recording) {
                    btnRec.innerText = "STOP RECORDING";
                    btnRec.classList.add('rec-active');
                    btnPlay.disabled = true;
                    info.innerText = "Recording... " + (data.recSize || 0) + " points";
                } else if (data.playing) {
                    btnRec.innerText = "Start Recording";
                    btnRec.disabled = true;
                    btnRec.classList.remove('rec-active');
                    
                    btnPlay.innerText = "⏹ Stop Playback";
                    btnPlay.classList.add('active'); // Turn Green
                    info.innerText = "Playing... " + (data.recSize || 0) + " points";
                } else {
                    btnRec.innerText = "Start Recording";
                    btnRec.classList.remove('rec-active');
                    btnRec.disabled = false;

                    btnPlay.innerText = "▶ Playback";
                    btnPlay.classList.remove('active');
                    btnPlay.disabled = (!data.recSize || data.recSize === 0);
                    info.innerText = data.recSize ? ("Stored: " + data.recSize + " points") : "Status: Ready";
                }
            }

            // Update monitor sliders (read-only view)
            if (data.servos) {
                data.servos.forEach((val, i) => {
                    const sld = document.getElementById(`monitor-sliders-sld${i}`);
                    // Since monitor sliders are disabled, we can always update them
                    if(sld) {
                        sld.value = val;
                        document.getElementById(`monitor-sliders-val${i}`).innerText = val + "%";
                    }
                });
            }
        }
        
        if(currentMode === 2) {
             const btnScriptPlay = document.getElementById('btn-script-play');
             const panel = document.getElementById('play-controls');
             
             if(data.playing) {
                 panel.style.display = 'block';
                 btnScriptPlay.innerText = "⏹ Stop Playback";
             } else {
                 // In Script Mode, showing the upload box is default. 
                 // If not playing, hide the stop button panel if desired, 
                 // or show it if we have data?
                 // For now, only show STOP if playing. 
                 panel.style.display = 'none';
             }
        }
        
        // Update Web Controls (Mode 1)
        if(currentMode === 1) {
            document.getElementById('dspX1').innerText = data.x.toFixed(1);
            document.getElementById('dspY1').innerText = data.y.toFixed(1);
            document.getElementById('dspZ1').innerText = data.z.toFixed(1);
            document.getElementById('dspP1').innerText = data.p.toFixed(1);

            // IK Error
            const errDiv = document.getElementById('ik-error');
            if (data.reachable === false) {
                 errDiv.style.display = 'block';
            } else {
                 errDiv.style.display = 'none';
            }

            // Update control sliders only if we are sure user isn't touching them
            if (data.servos) {
                data.servos.forEach((val, i) => {
                    const sld = document.getElementById(`control-sliders-sld${i}`);
                    if(sld && document.activeElement !== sld) {
                         sld.value = val;
                         document.getElementById(`control-sliders-val${i}`).innerText = val + "%";
                    }
                });
            }
        }
      })
      .catch(err => console.error("Poll error", err));
  }

  window.onload = init;
</script>
</body>
</html>
)rawliteral";

#endif
