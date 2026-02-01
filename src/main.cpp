#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include <vector>
#include "web_site.h"
#include "demos.h"

// ================= KONFIGURATION =================
const char *ssid = "RobotArmMonitor";
const char *password = "ghostarm_secret";
const char *hostName = "ghostarm"; // URL: http://ghostarm.local

// --- RECORDING DATA ---
struct RecordedStep
{
    uint8_t base;
    uint8_t shoulder;
    uint8_t elbow;
    uint8_t wrist;
    uint8_t gripper;
};
std::vector<RecordedStep> recordingBuffer;
bool isRecording = false;
bool isPlaying = false;
size_t playStep = 0;
unsigned long lastPlayTime = 0;
bool ikReachable = true;
String uploadLineBuffer = "";

// --- ROBOT DIMENSIONS (cm) ---
const float L1 = 7.55;  // Base Height
const float L2 = 9.01;  // Shoulder Length
const float L3 = 9.005; // Elbow Length
const float L4 = 3.25;  // Gripper Length

// --- SERVO LIMITS & KINEMATICS DATA ---
struct ServoConfig
{
    uint8_t pin;
    int minUs;
    int maxUs;
    int startUs;
    float angle0;   // Angle when slider is at 0%
    float angle100; // Angle when slider is at 100%
    String name;
};

// Index: 0=Base, 1=Shoulder, 2=Elbow, 3=Wrist, 4=Gripper
ServoConfig servos[] = {
    // Base: 0%=Left(85), 100%=Right(-65)
    {11, 500, 2500, 1500, 85.0, -65.8, "Base"},

    // Shoulder: SWAPPED ANGLES to fix "Wrong Way Round"
    // If 0% on slider makes the arm go BACK/UP, then 0% = 180 degrees.
    // If 100% on slider makes the arm go FLAT/FORWARD, then 100% = 0 degrees.
    {12, 500, 2200, 600, 180.0, 0.0, "Shoulder"},

    // Elbow: 0%=Straight(172), 100%=Bent(24)
    {13, 500, 2500, 2400, 172.0, 24.34, "Elbow"},

    // Wrist: 0%=Down(94), 100%=Up(230)
    {14, 500, 2500, 1500, 94.5, 230.3, "Wrist"},

    // Gripper
    {15, 600, 1500, 600, 0, 0, "Gripper"}};

// --- MODES ---
enum ControlMode
{
    MODE_CONTROLLER = 0,
    MODE_WEB = 1,
    MODE_SCRIPT = 2
};
int currentMode = MODE_CONTROLLER;

struct ScriptState
{
    bool active;
    int scriptId;
    int step;
    unsigned long lastStepTime;
};
ScriptState scriptRunner = {false, 0, 0, 0};

// --- DATA STRUCTURES ---
// INCOMING ESP-NOW Message matching the Controller
typedef struct struct_message
{
    uint8_t base;     // 0-100
    uint8_t shoulder; // 0-100
    uint8_t elbow;    // 0-100
    uint8_t wrist;    // 0-100
    bool grabber;     // 0 or 1
} struct_message;

struct_message incomingData;

// internal state (0-100)
// Initialize with "startUs" equivalents roughly
int currentPos[] = {50, 0, 100, 65, 0};

// Return type for Kinematics
struct Coord
{
    float x;
    float y;
    float z;
    float pitch;
};

// --- HARDWARE OBJECTS ---
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
WebServer server(80);

// --- HELPER FUNCTIONS ---

// Converts microseconds to PWM ticks (0-4096)
int usToTicks(int microseconds)
{
    float pulse_length = 1000000.0 / 50.0; // 50Hz
    return (int)(microseconds / pulse_length * 4096.0);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int angleToPercent(int servoIndex, float angle)
{
    ServoConfig cfg = servos[servoIndex];
    // Reverse logic of mapFloat: (angle - out_min) / (out_max - out_min) could work but mapFloat is generic.
    // percent = (angle - angle0) * 100 / (angle100 - angle0)
    // Note: angle0 corresponds to 0%, angle100 to 100%
    float p = (angle - cfg.angle0) * 100.0 / (cfg.angle100 - cfg.angle0);
    return (int)p;
}

// Moves a specific servo by index using percentage (0-100)
void moveServo(int servoIndex, int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    // Save global state for kinematics
    currentPos[servoIndex] = percent;

    ServoConfig cfg = servos[servoIndex];

    // Map percent to microseconds
    // Note: Using map() for integer mapping like in Code 1
    // Code 2 used float mapping, but for PWM output integer is fine.
    int pulse = map(percent, 0, 100, cfg.minUs, cfg.maxUs);

    // Hard-Limit Safety
    if (pulse < cfg.minUs)
        pulse = cfg.minUs;
    if (pulse > cfg.maxUs)
        pulse = cfg.maxUs;

    pwm.setPWM(cfg.pin, 0, usToTicks(pulse));
}

// --- FORWARD KINEMATICS ---
Coord calculateFK()
{
    // 1. Convert Percent to Physical Angles
    float theta1 = mapFloat(currentPos[0], 0, 100, servos[0].angle0, servos[0].angle100);
    float theta2 = mapFloat(currentPos[1], 0, 100, servos[1].angle0, servos[1].angle100);
    float gamma = mapFloat(currentPos[2], 0, 100, servos[2].angle0, servos[2].angle100);
    float wristServo = mapFloat(currentPos[3], 0, 100, servos[3].angle0, servos[3].angle100);

    // 2. Degrees to Radians
    float t1_rad = radians(theta1);
    float t2_rad = radians(theta2);
    float gamma_rad = radians(gamma);

    // 3. Global Angles
    // Shoulder Angle: 0 = Horizontal Forward, 90 = Up.

    // Elbow Global (relative to horizon)
    float elbow_global_rad = t2_rad - (PI - gamma_rad);

    // Wrist Global (Pitch)
    float wrist_deviation_rad = radians(wristServo - 180.0);
    float pitch_rad = elbow_global_rad + wrist_deviation_rad;

    // 4. Coordinates
    float R = L2 * cos(t2_rad) + L3 * cos(elbow_global_rad) + L4 * cos(pitch_rad);
    float Z = L1 + L2 * sin(t2_rad) + L3 * sin(elbow_global_rad) + L4 * sin(pitch_rad);

    float X = R * cos(t1_rad);
    float Y = R * sin(t1_rad);

    return {X, Y, Z, (float)degrees(pitch_rad)};
}

void calculateIK(float x, float y, float z, float pitch_deg)
{
    ikReachable = true;

    // 1. Base (Theta 1)
    // atan2(y, x).
    float theta1 = degrees(atan2(y, x));

    // 2. Wrist Center
    float R = sqrt(x * x + y * y);
    // Singularity protection (near origin)
    if (R < 0.1)
        R = 0.1;

    float Z_arm = z - L1; // Height relative to shoulder

    // Wrist joint position
    float pitch_rad = radians(pitch_deg);
    float wr = R - L4 * cos(pitch_rad);
    float wz = Z_arm - L4 * sin(pitch_rad);

    // 3. Triangle L2-L3 to Reach (wr, wz)
    float D_sq = wr * wr + wz * wz;
    float D = sqrt(D_sq);

    if (D > (L2 + L3) || D < abs(L2 - L3))
    {
        Serial.println("IK Target Unreachable");
        ikReachable = false;
        return;
    }

    // Law of Cosines for Shoulder (Alpha)
    float c_alpha = (D_sq + L2 * L2 - L3 * L3) / (2 * D * L2);
    // Clamp
    if (c_alpha > 1.0)
        c_alpha = 1.0;
    if (c_alpha < -1.0)
        c_alpha = -1.0;
    float alpha_rad = acos(c_alpha);

    float phi_rad = atan2(wz, wr);
    float theta2_rad = phi_rad + alpha_rad; // Elbow UP solution
    float theta2 = degrees(theta2_rad);

    // Law of Cosines for Elbow (Gamma - included angle)
    float c_gamma = (L2 * L2 + L3 * L3 - D_sq) / (2 * L2 * L3);
    if (c_gamma > 1.0)
        c_gamma = 1.0;
    if (c_gamma < -1.0)
        c_gamma = -1.0;
    float gamma_rad = acos(c_gamma);
    float gamma = degrees(gamma_rad);

    // Wrist Servo
    // FK Logic was: pitch = theta2 - (180 - gamma) + (servo - 180)
    // pitch_rad = theta2_rad - (PI - gamma_rad) + (wrist_servo_rad - PI)
    // wrist_servo_rad = pitch_rad - theta2_rad + PI - gamma_rad + PI
    // Wait -> wrist_servo_rad = pitch_rad - (theta2_rad - PI + gamma_rad) + PI
    // Let's reuse the derived formula: wrist_servo_rad = pitch_rad - elbow_global_rad + PI
    // where elbow_global_rad = theta2_rad - (PI - gamma_rad)
    float elbow_global_rad = theta2_rad - (PI - gamma_rad);
    float wrist_servo_rad = pitch_rad - elbow_global_rad + PI;
    float wrist_servo = degrees(wrist_servo_rad);

    moveServo(0, angleToPercent(0, theta1));
    moveServo(1, angleToPercent(1, theta2));
    moveServo(2, angleToPercent(2, gamma));
    moveServo(3, angleToPercent(3, wrist_servo));
}

// --- ESP-NOW CALLBACK ---
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len)
{
    if (len != sizeof(incomingData))
        return;

    memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

    if (currentMode != MODE_CONTROLLER)
        return;

    // Ignore controller input if playing back recording
    if (isPlaying)
        return;

    // Update logic matching Code 1
    moveServo(0, incomingData.base);
    moveServo(1, incomingData.shoulder);
    moveServo(2, incomingData.elbow);
    moveServo(3, incomingData.wrist);

    // Gripper logic: bool to 0/100
    int gripperPercent = incomingData.grabber ? 100 : 0;
    moveServo(4, gripperPercent);

    // Recording Logic
    if (isRecording && recordingBuffer.size() < 2000)
    {
        recordingBuffer.push_back({(uint8_t)incomingData.base,
                                   (uint8_t)incomingData.shoulder,
                                   (uint8_t)incomingData.elbow,
                                   (uint8_t)incomingData.wrist,
                                   (uint8_t)gripperPercent});
    }
}

// --- WEB SERVER HANDLERS ---
void handleRoot()
{
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(PAGE_HTML_HEAD);
    server.sendContent_P(PAGE_CSS);
    server.sendContent_P(PAGE_HTML_BODY);
    server.sendContent_P(PAGE_JS);
}

void handleState()
{
    // Calculate current XYZ
    Coord pos = calculateFK();

    StaticJsonDocument<512> doc;
    doc["x"] = pos.x;
    doc["y"] = pos.y;
    doc["z"] = pos.z;
    doc["p"] = pos.pitch;
    doc["reachable"] = ikReachable;
    doc["recording"] = isRecording;
    doc["playing"] = isPlaying;
    doc["recSize"] = recordingBuffer.size();
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);

    JsonArray servoArr = doc.createNestedArray("servos");
    for (int i = 0; i < 5; i++)
    {
        servoArr.add(currentPos[i]);
    }

    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void handleRecord()
{
    if (server.hasArg("action"))
    {
        String action = server.arg("action");
        if (action == "start")
        {
            isRecording = true;
            isPlaying = false;
            recordingBuffer.clear();
        }
        else if (action == "stop")
        {
            isRecording = false;
            isPlaying = false;
        }
        else if (action == "play")
        {
            if (!recordingBuffer.empty())
            {
                isRecording = false;
                isPlaying = true;
                playStep = 0;
                lastPlayTime = millis();
            }
        }
        else if (action == "clear")
        {
            recordingBuffer.clear();
            isPlaying = false;
        }
    }
    server.send(200, "text/plain", "OK");
}

void handleDownload()
{
    if (recordingBuffer.empty())
    {
        server.send(404, "text/plain", "No recording available");
        return;
    }

    String output = "Base,Shoulder,Elbow,Wrist,Gripper\n";
    for (const auto &step : recordingBuffer)
    {
        output += String(step.base) + "," + String(step.shoulder) + "," +
                  String(step.elbow) + "," + String(step.wrist) + "," +
                  String(step.gripper) + "\n";
    }

    server.sendHeader("Content-Disposition", "attachment; filename=recording.csv");
    server.send(200, "text/csv", output);
}

void processLine(String line)
{
    if (line.startsWith("Base"))
        return; // Header

    int b, s, e, w, g;
    if (sscanf(line.c_str(), "%d,%d,%d,%d,%d", &b, &s, &e, &w, &g) == 5)
    {
        recordingBuffer.push_back({(uint8_t)b, (uint8_t)s, (uint8_t)e, (uint8_t)w, (uint8_t)g});
    }
}

void handleLoadDemo()
{
    if (!server.hasArg("name"))
    {
        server.send(400, "text/plain", "Missing name");
        return;
    }
    String name = server.arg("name");
    const char *demoPtr = nullptr;

    if (name == "hello")
        demoPtr = demo_hello;
    else if (name == "picknplace")
        demoPtr = demo_picknplace;
    else if (name == "dancing")
        demoPtr = demo_dancing;

    if (!demoPtr)
    {
        server.send(404, "text/plain", "Demo not found");
        return;
    }

    recordingBuffer.clear();

    // Read from PROGMEM
    String lineBuffer = "";
    int i = 0;
    while (true)
    {
        char c = pgm_read_byte(demoPtr + i);
        if (c == 0)
            break; // End of string

        if (c == '\n' || c == '\r')
        {
            if (lineBuffer.length() > 0)
            {
                processLine(lineBuffer);
                lineBuffer = "";
            }
        }
        else
        {
            lineBuffer += c;
        }
        i++;
    }
    // Process last line
    if (lineBuffer.length() > 0)
        processLine(lineBuffer);

    // Start playing
    isRecording = false;
    isPlaying = true;
    playStep = 0;
    lastPlayTime = millis();

    server.send(200, "application/json", "{\"status\":\"ok\", \"steps\":" + String(recordingBuffer.size()) + "}");
}

void onScriptUpload()
{
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        recordingBuffer.clear();
        uploadLineBuffer = "";
        Serial.printf("Upload Start: %s\n", upload.filename.c_str());
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        for (size_t i = 0; i < upload.currentSize; i++)
        {
            char c = (char)upload.buf[i];
            if (c == '\n' || c == '\r')
            {
                if (uploadLineBuffer.length() > 0)
                {
                    processLine(uploadLineBuffer);
                    uploadLineBuffer = "";
                }
            }
            else
            {
                uploadLineBuffer += c;
            }
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        // Process last line if any
        if (uploadLineBuffer.length() > 0)
            processLine(uploadLineBuffer);

        Serial.printf("Upload End. Steps: %u\n", recordingBuffer.size());
        isRecording = false;
        isPlaying = true;
        playStep = 0;
        lastPlayTime = millis();
    }
}

void handleSetMode()
{
    if (server.hasArg("mode"))
    {
        currentMode = server.arg("mode").toInt();
        if (currentMode != MODE_SCRIPT)
            scriptRunner.active = false;

        // Stop recording if leaving controller mode
        if (currentMode != MODE_CONTROLLER)
        {
            isRecording = false;
        }

        server.send(200, "text/plain", "Mode Set");
    }
    else
    {
        server.send(400, "text/plain", "Missing args");
    }
}

void handleSetServo()
{
    // Mode check removed to allow voice control override
    // if (currentMode != MODE_WEB) ...

    if (server.hasArg("index") && server.hasArg("value"))
    {
        int idx = server.arg("index").toInt();
        int val = server.arg("value").toInt();
        if (idx >= 0 && idx < 5)
            moveServo(idx, val);
        server.send(200, "text/plain", "OK");
    }
    else
    {
        server.send(400, "text/plain", "Missing args");
    }
}

void handleSetXYZ()
{
    if (currentMode != MODE_WEB)
    {
        server.send(403, "text/plain", "Not in Web Mode");
        return;
    }
    if (server.hasArg("x") && server.hasArg("y") && server.hasArg("z") && server.hasArg("p"))
    {
        float x = server.arg("x").toFloat();
        float y = server.arg("y").toFloat();
        float z = server.arg("z").toFloat();
        float p = server.arg("p").toFloat();
        calculateIK(x, y, z, p);
        server.send(200, "text/plain", "Moved");
    }
    else
    {
        server.send(400, "text/plain", "Missing args");
    }
}

void handleConnectWifi()
{
    if (server.hasArg("ssid") && server.hasArg("pass"))
    {
        String s = server.arg("ssid");
        String p = server.arg("pass");

        Serial.print("Connecting to: ");
        Serial.println(s);
        WiFi.begin(s.c_str(), p.c_str());

        // Block briefly to try connection (max 5 secs)
        int tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 10)
        {
            delay(500);
            tries++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            String ip = WiFi.localIP().toString();
            server.send(200, "text/plain", "Connected! IP: " + ip);

            // Re-broadcast MDNS (Clean Restart)
            MDNS.end();
            if (MDNS.begin(hostName))
            {
                MDNS.addService("http", "tcp", 80);
            }
        }
        else
        {
            server.send(200, "text/plain", "Failed to connect. Check creds.");
        }
    }
    else
    {
        server.send(400, "text/plain", "Missing args");
    }
}

void handleRunScript()
{
    if (server.hasArg("id"))
    {
        currentMode = MODE_SCRIPT;
        scriptRunner.active = true;
        scriptRunner.scriptId = server.arg("id").toInt();
        scriptRunner.step = 0;
        scriptRunner.lastStepTime = 0;
        server.send(200, "text/plain", "Script Started");
    }
    else
    {
        server.send(400, "text/plain", "Missing id");
    }
}

void setup()
{
    Serial.begin(115200);
    Wire.begin(21, 22);

    // 1. PWM Init
    pwm.begin();
    pwm.setPWMFreq(50);

    for (int i = 0; i < 5; i++)
    {
        pwm.setPWM(servos[i].pin, 0, usToTicks(servos[i].startUs));
    }

    // 2. WiFi Setup (Combine AP and Station)
    WiFi.mode(WIFI_AP_STA);
    // CRITICAL: Disable WiFi Power Save to prevent mDNS/HTTP dropping out after a while
    WiFi.setSleep(false);

    // Setup SoftAP
    WiFi.softAP(ssid, password);
    Serial.print("Access Point Started. IP: ");
    Serial.println(WiFi.softAPIP());

    // START MDNS
    if (MDNS.begin(hostName))
    {
        Serial.println("MDNS responder started");
        Serial.printf("Access at http://%s.local\n", hostName);
        MDNS.addService("http", "tcp", 80);
    }

    // 3. ESP-NOW Init
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register Receiver
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    // 4. Web Server
    server.on("/", handleRoot);
    server.on("/state", handleState);
    server.on("/set_mode", handleSetMode);
    server.on("/set_servo", handleSetServo);
    server.on("/set_xyz", handleSetXYZ);
    server.on("/run_script", handleRunScript);
    server.on("/record", handleRecord);
    server.on("/connect_wifi", handleConnectWifi); // Added
    server.on("/download", handleDownload);
    server.on("/upload_script", HTTP_POST, []()
              { server.send(200, "text/plain", ""); }, onScriptUpload);
    server.on("/load_demo", handleLoadDemo);
    server.begin();

    Serial.println("Server & Robot Ready");
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
}

void loop()
{
    server.handleClient();

    // Playback Logic
    if (isPlaying && !recordingBuffer.empty())
    {
        unsigned long now = millis();
        if (now - lastPlayTime > 20)
        { // ~50Hz Playback
            if (playStep < recordingBuffer.size())
            {
                const auto &step = recordingBuffer[playStep];
                moveServo(0, step.base);
                moveServo(1, step.shoulder);
                moveServo(2, step.elbow);
                moveServo(3, step.wrist);
                moveServo(4, step.gripper);
                playStep++;
                lastPlayTime = now;
            }
            else
            {
                isPlaying = false; // Done
            }
        }
    }

    if (currentMode == MODE_SCRIPT && scriptRunner.active)
    {
        unsigned long now = millis();
        // Script 1: Wave
        if (scriptRunner.scriptId == 1)
        {
            if (scriptRunner.step == 0 && (now - scriptRunner.lastStepTime > 100))
            {
                moveServo(2, 50); // Elbow Up
                moveServo(3, 50); // Wrist mid
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 1 && (now - scriptRunner.lastStepTime > 1000))
            {
                moveServo(3, 80); // Wrist Up
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 2 && (now - scriptRunner.lastStepTime > 500))
            {
                moveServo(3, 20); // Wrist Down
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 3 && (now - scriptRunner.lastStepTime > 500))
            {
                moveServo(3, 80); // Wrist Up
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 4 && (now - scriptRunner.lastStepTime > 500))
            {
                moveServo(3, 50);            // Center
                scriptRunner.active = false; // Done
            }
        }
        // Script 2: Pick Place Demo
        else if (scriptRunner.scriptId == 2)
        {
            if (scriptRunner.step == 0)
            {
                moveServo(4, 0);          // Open
                calculateIK(15, 0, 5, 0); // Go down
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 1 && (now - scriptRunner.lastStepTime > 2000))
            {
                moveServo(4, 100); // Close
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 2 && (now - scriptRunner.lastStepTime > 1000))
            {
                calculateIK(15, 0, 15, 0); // Up
                scriptRunner.step++;
                scriptRunner.lastStepTime = now;
            }
            else if (scriptRunner.step == 3 && (now - scriptRunner.lastStepTime > 2000))
            {
                scriptRunner.active = false;
            }
        }
        else if (scriptRunner.scriptId == 3)
        {
            scriptRunner.active = false;
        }
    }

    // Allow a tiny delay for network stability
    delay(5);
}
