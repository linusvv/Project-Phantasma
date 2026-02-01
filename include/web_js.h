#ifndef WEB_JS_H
#define WEB_JS_H

const char PAGE_JS[] PROGMEM = R"rawliteral(
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
    
    // --- Voice Button Press-and-Hold Listeners ---
    // Note: On mobile, 'click' might fire after 'touchend', so we preventDefault.
    // Also, 'contextmenu' happens on long press.
    
    // Check if Speech API exists
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    const btnMic = document.getElementById('btn-mic');

    if (!SpeechRecognition) {
        if(btnMic) {
            btnMic.style.display = 'none';
            console.log("Speech API not supported");
        }
    } else {
        if(btnMic) {
             // Disable Context Menu (Long Press menu)
             btnMic.oncontextmenu = function(event) {
                 event.preventDefault();
                 event.stopPropagation();
                 return false;
             };

             // Mouse Events (Desktop)
             btnMic.addEventListener('mousedown', (e) => { 
                 if(e.button !== 0 || ignoreMouse) return; // Ignore if touch active
                 startVoice(); 
             });
             btnMic.addEventListener('mouseup', (e) => { 
                 if(ignoreMouse) return;
                 stopVoice(); 
             });
             btnMic.addEventListener('mouseleave', (e) => { 
                 if(ignoreMouse) return;
                 stopVoice(); 
             });

             // Touch Events (Mobile)
             btnMic.addEventListener('touchstart', (e) => { 
                 e.preventDefault(); 
                 ignoreMouse = true; // Block mouse events
                 startVoice(); 
             }, { passive: false });
             
             // Prevent scroll/cancel on move
             btnMic.addEventListener('touchmove', (e) => { 
                 e.preventDefault(); 
             }, { passive: false });
             
             btnMic.addEventListener('touchend', (e) => { 
                 e.preventDefault(); 
                 stopVoice(); 
                 // Reset flag after delay to allow mouse again if needed
                 setTimeout(() => { ignoreMouse = false; }, 500);
             }, { passive: false });
             
             btnMic.addEventListener('touchcancel', (e) => { 
                 e.preventDefault(); 
                 stopVoice(); 
                 setTimeout(() => { ignoreMouse = false; }, 500);
             }, { passive: false });
        }
    }
    
    // File Input Listener
    const scriptInput = document.getElementById('scriptFile');
    if(scriptInput) {
        scriptInput.addEventListener('change', function(e) {
            if(this.files && this.files.length > 1) {
                document.getElementById('file-chosen').innerText = this.files.length + " files selected";
            } else if(this.files && this.files.length === 1) {
                document.getElementById('file-chosen').innerText = "Selected: " + this.files[0].name;
            } else {
                document.getElementById('file-chosen').innerText = "No file chosen";
            }
        });
    }
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

  function connectWifi() {
      const ssid = document.getElementById('ssid').value;
      const pass = document.getElementById('pass').value;
      const status = document.getElementById('wifi-status');
      
      if(!ssid) { alert("Enter SSID"); return; }
      
      status.innerText = "Connecting to " + ssid + "...";
      
      fetch('/connect_wifi?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass))
      .then(r => r.text())
      .then(msg => {
          status.innerText = msg;
          if(msg.includes("Connected")) {
              const ip = msg.replace("Connected! IP: ", "");
              alert("Success! Connect your phone to home WiFi '" + ssid + "'.\n\n" +
                    "Try URL: http://ghostarm.local\n" + 
                    "Fallback IP: http://" + ip);
          }
      })
      .catch(e => {
          status.innerText = "Error requesting connection (Check Monitor)";
      });
  }

  function sendIK() {
    const x = document.getElementById('ikX').value;
    const y = document.getElementById('ikY').value;
    const z = document.getElementById('ikZ').value;
    const p = document.getElementById('ikP').value;
    fetch(`/set_xyz?x=${x}&y=${y}&z=${z}&p=${p}`);
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
      // Use logic based on actual server state, not button text
      const action = isServerPlaying ? 'stop' : 'play';
      fetch('/record?action=' + action);
  }

  function downloadRecord() {
      window.location.href = '/download';
  }

  function startPolling() {
    if(intervalId) clearInterval(intervalId);
    intervalId = setInterval(fetchData, 250);
  }

  // Global Variable to track server playback state
  let isServerPlaying = false;

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
        isServerPlaying = data.playing || false; // Update global state
        
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
                 panel.style.display = 'none';
             }
        }
        
        // WiFi / Voice Visibility
        // Show only if connected to WiFi (implies partial internet capability)
        const voiceOverlay = document.getElementById('voice-overlay');
        if(voiceOverlay) {
             if(data.wifi_connected === true) {
                 voiceOverlay.style.display = 'block';
             } else {
                 voiceOverlay.style.display = 'none';
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
  
  // --- VOICE CONTROL ---
  let recognition;
  let isListening = false;
  let ignoreMouse = false; // Flag to prevent double-firing on mobile
  let isButtonHeld = false; // Flag to track physical button state
  let manualStop = false; // Flag if we manually stopped it
  let tempTranscript = ""; // Buffer for interim results

  function startVoice() {
    isButtonHeld = true;
    manualStop = false;
    tempTranscript = ""; // Reset buffer
    
    // Immediate feedback
    try { if(navigator.vibrate) navigator.vibrate(50); } catch(e) {}
    
    if(isListening) return;

    // Setup Speech Recognition
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) return;

    try {
        recognition = new SpeechRecognition();
        recognition.continuous = true; 
        recognition.lang = 'en-US';
        recognition.interimResults = true; // IMPORTANT: Get partial results
        recognition.maxAlternatives = 1;

        recognition.onstart = function() {
            isListening = true;
            document.getElementById('btn-mic').classList.add('mic-active');
            document.getElementById('voice-status').innerText = "Listening... (Speak Now)";
            if(navigator.vibrate) navigator.vibrate(50); 
        };

        recognition.onend = function() {
            isListening = false;
            
            // 1. Check if we have a dangling interim result that wasn't finalized
            if(manualStop && tempTranscript.length > 0) {
                 // We stopped manually, but never got a 'final' packet. Use the temp one.
                 processCommand(tempTranscript);
                 tempTranscript = ""; // Clear so we don't double process
            }

            // 2. CRITICAL FIX: If button is still held, and we didn't manually stop it, RESTART.
            if(isButtonHeld && !manualStop) {
                console.log("Auto-restarting speech engine (Held)");
                document.getElementById('voice-status').innerText = "Listening... (Keep talking)";
                try {
                     recognition.start();
                } catch(e) {
                     // If start fails (too fast), retry with delay
                     setTimeout(startVoice, 100);
                }
                return; 
            }
            
            // 3. Normal stop cleanup
            document.getElementById('btn-mic').classList.remove('mic-active');
            // If we just processed a command (above), status is updated in processCommand usually?
            // If tempTranscript was empty and no final result, we might have heard nothing.
            
            setTimeout(() => {
                 if(!isListening) document.getElementById('voice-status').innerText = "Hold button to speak";
            }, 1500);
        };

        recognition.onresult = function(event) {
            let finalCmd = "";
            let interimCmd = "";

            for (let i = event.resultIndex; i < event.results.length; ++i) {
                if (event.results[i].isFinal) {
                    finalCmd += event.results[i][0].transcript;
                } else {
                    interimCmd += event.results[i][0].transcript;
                }
            }
            
            if (finalCmd.length > 0) {
                finalCmd = finalCmd.trim().toLowerCase();
                document.getElementById('voice-status').innerText = 'Final: "' + finalCmd + '"';
                processCommand(finalCmd);
                tempTranscript = ""; // Clear temp, we have final
            } else if (interimCmd.length > 0) {
                tempTranscript = interimCmd.trim().toLowerCase();
                document.getElementById('voice-status').innerText = 'Hearing: "' + tempTranscript + '"';
            }
        };
        
        recognition.nomatch = function(event) {
            document.getElementById('voice-status').innerText = "Sorted: No Match";
        };
        
        recognition.onerror = function(event) {
            console.error("Speech Error:", event.error);
            if(event.error === 'no-speech') {
                 if(isButtonHeld) return; 
                 document.getElementById('voice-status').innerText = "No speech detected";
            } else if(event.error === 'not-allowed') {
                 document.getElementById('voice-status').innerText = "Mic blocked/HTTPS needed";
                 manualStop = true; 
            } else {
                 document.getElementById('voice-status').innerText = "Err: " + event.error;
            }
            stopVoice(true); // Cleanup
        };

        recognition.start();
        
    } catch(e) {
        console.error("Start Error", e);
        document.getElementById('voice-status').innerText = "Err: " + e.message;
        isListening = false;
    }
  }
  
  function stopVoice(preserveStatus) {
    isButtonHeld = false;
    manualStop = true; // Tell onend NOT to restart
    if(recognition && isListening) {
        try {
            if(preserveStatus !== true) document.getElementById('voice-status').innerText = "Releasing...";
            // Give a tiny delay for final packets to arrive before killing it
            // This is safer than stop() immediately which sometimes discards buffers on mobile
            setTimeout(() => {
                if(recognition) recognition.stop();
            }, 100);
        } catch(e) { console.log(e); }
    }
  }

  function showVoiceHelp() {
      document.getElementById('voice-help').style.display = 'flex';
  }

  function processCommand(cmd) {
    console.log("Processing voice command:", cmd);
    
    // Command Normalization
    cmd = cmd.toLowerCase();

    // 0. Mode Switching
    if (cmd.includes('mode controller') || cmd.includes('controller mode')) { setMode(0); return; }
    if (cmd.includes('mode web') || cmd.includes('web mode')) { setMode(1); return; }
    if (cmd.includes('mode script') || cmd.includes('script mode')) { setMode(2); return; }

    // 1. Demos (Force switch to Script Mode for demos?) No, loadDemo works anytime usually but best in Mode 2
    if (cmd.includes('hello') || cmd.includes('wave')) loadDemo('hello');
    else if (cmd.includes('dance') || cmd.includes('party')) loadDemo('dancing');
    else if (cmd.includes('pick') || cmd.includes('place')) loadDemo('picknplace');

    // 2. Gripper (Any Mode - Force Override)
    // Note: If in Controller Mode, controller might fight back.
    else if (cmd.includes('open') || cmd.includes('release') || cmd.includes('drop')) updateServo(4, 0);
    else if (cmd.includes('close') || cmd.includes('grab') || cmd.includes('catch')) updateServo(4, 100);

    // 3. Recording (Controller Mode 0)
    else if (cmd.includes('start record')) {
        // If not in mode 0, switch?
        if(currentMode !== 0) { 
            setMode(0); 
            setTimeout(() => fetch('/record?action=start'), 500);
        } else {
             fetch('/record?action=start');
        }
    }
    else if ((cmd.includes('stop') && cmd.includes('record')) || cmd.includes('finish')) fetch('/record?action=stop');
    
    // 4. Playback / Stop
    else if (cmd.includes('play record') || cmd.includes('replay')) {
         if(currentMode !== 0) setMode(0);
         setTimeout(() => fetch('/record?action=play'), 500); // Force 'play' explicitly
    }
    else if (cmd.includes('play')) togglePlayback(); 
    else if (cmd.includes('stop')) {
        // Universal Stop
        fetch('/record?action=stop'); 
    }
  }

  window.onload = init;
</script>
)rawliteral";

#endif
