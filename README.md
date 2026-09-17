# ESP32 Conveyor Sorting Simulation

A browser-controlled simulation of an automatic color-sorting conveyor, implemented in `ConveyorSimulation.ino`. The ESP32 hosts a web dashboard, processes simulated object detections, and records sorting events in the Serial Monitor and dashboard log.

**Project status:** simulation only. Motor, sensor, and servo behavior is represented by variables and log messages. The sketch does not configure GPIO pins or control physical sorting hardware.

## Features

- Four-state sorting sequence: Standby, Detecting, Sorting, and Reset.
- Manual red and green object simulation.
- Simulated IR detection with a randomly selected red or green object.
- Red, green, and total object counters.
- Embedded HTML, CSS, JavaScript, and SVG dashboard.
- Browser animation showing a conveyor, sensors, a servo arm, and sorting bins.
- JSON status endpoint polled by the dashboard every second.
- Recent event log, with approximately 15 lines retained, newest first.

## Requirements

- An ESP32 board and USB connection for running the sketch on hardware.
- Arduino IDE with ESP32 board support installed.
- `WiFi.h` and `WebServer.h`, provided by the ESP32 Arduino environment.
- Wi-Fi access and a browser on a device that can reach the ESP32.

No external sensors, motor driver, or servos are needed for this simulation. No pin mapping is defined in the source.

## Setup

1. Place `ConveyorSimulation.ino` inside a folder named `ConveyorSimulation` and open it in Arduino IDE.
2. Select the matching ESP32 board and serial port.
3. Replace the Wi-Fi settings near the beginning of the sketch:

   ```cpp
   const char* WIFI_SSID     = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
   ```

   Use your own credentials; the values above are placeholders.
4. Upload the sketch to the board.
5. Open Serial Monitor at **115200 baud**.
6. Wait for Wi-Fi connection and the printed dashboard IP address.
7. Open `http://<ESP32_IP>/` from a device on the same reachable network.

The web server listens on **port 80**. During startup, the sketch waits until Wi-Fi connects; it does not implement a connection timeout or fallback access point.

The source comments also mention Wokwi as a simulation option. This uploaded file does not include a Wokwi project configuration or network-access setup.

## Using the Dashboard

The existing dashboard labels are in Vietnamese. Their functions are:

| Dashboard button | Function |
| --- | --- |
| `Gia lap vat den (IR)` | Randomly selects RED or GREEN in the browser and starts a sorting cycle. |
| `Gia lap mau Do` | Starts a RED sorting cycle. |
| `Gia lap mau Xanh` | Starts a GREEN sorting cycle. |

Each accepted simulation request processes one object. The dashboard shows conveyor status, servo status, counters, recent logs, and an illustrative animation. The browser ignores further button presses while its animation is active, for approximately **2.6 seconds**.

Reloading the page preserves counters while the ESP32 remains running. Restarting the ESP32 resets counters and other in-memory state. There is no counter-reset button or persistent storage.

## Sorting Logic

### State sequence

| State | Behavior | Delay in the sketch |
| --- | --- | --- |
| `STATE_STANDBY` | Waits for a simulation request. | None |
| `STATE_DETECTING` | Sets the conveyor flag to running and logs object detection. | 700 ms |
| `STATE_SORTING` | Records the color, assigns a simulated servo angle, and increments counters. | 700 ms |
| `STATE_RESET` | Sets the servo status to idle and the conveyor flag to stopped. | 500 ms |
| Return to `STATE_STANDBY` | Ready for the next request. | None |

A backend sorting cycle takes approximately **1.9 seconds**, excluding request and processing overhead.

### Color handling

| Input color | Simulated servo angle | Counter changes |
| --- | --- | --- |
| `RED` | 180° | Red +1; total +1 |
| `GREEN` | 0° | Green +1; total +1 |
| Any other supplied value | 90° | Total +1 only |

The request handler converts input to uppercase before processing. The dashboard sends only RED or GREEN. Other values can be supplied directly through the API, but the dashboard has no dedicated unknown-color control.

**Reset behavior:** the code changes `servoStatus` to idle but does not restore `servoAngle` to a home angle. The API therefore retains the angle assigned to the most recently processed object.

## HTTP API

The dashboard uses the following GET requests:

| Endpoint | Purpose | Response |
| --- | --- | --- |
| `/` | Load the dashboard. | HTML |
| `/status` | Read current system data. | JSON |
| `/simulate?color=RED` | Process a red object. | JSON after the cycle finishes |
| `/simulate?color=GREEN` | Process a green object. | JSON after the cycle finishes |
| `/simulate?color=UNKNOWN` | Exercise the unknown-color branch. | JSON after the cycle finishes |
| `/simulate` | Return status without processing an object because `color` is absent. | JSON |

### Status fields

| Field | Type | Meaning |
| --- | --- | --- |
| `state` | String | `Standby`, `Detecting`, `Sorting`, or `Reset`. |
| `conveyor` | String | `DANG CHAY` (running) or `DUNG` (stopped). |
| `servo` | String | Servo status text; `NGHỈ` means idle. |
| `servoAngle` | Integer | Most recently assigned simulated angle. |
| `lastColor` | String | Most recent color input; initially `--`. |
| `countRed` | Integer | Number of red objects processed. |
| `countGreen` | Integer | Number of green objects processed. |
| `countTotal` | Integer | Number of all objects processed, including unknown colors. |
| `log` | String | Recent log entries separated by newline characters. |

`servoAngle` and `lastColor` are exposed by the API but are not displayed as separate dashboard values.

## Source Structure

| Function | Responsibility |
| --- | --- |
| `addLog()` | Prints to Serial and maintains the recent log buffer. |
| `handleObjectDetected()` | Executes the blocking sorting sequence and updates counters. |
| `stateName()` | Converts the state enum into readable text. |
| `buildDashboardHTML()` | Builds the embedded dashboard page. |
| `buildStatusJSON()` | Builds the JSON status response. |
| `handleRoot()` | Serves the dashboard. |
| `handleSimulate()` | Reads the color argument, runs a cycle, and returns status. |
| `handleStatus()` | Returns status without starting a cycle. |
| `setup()` | Starts Serial, connects Wi-Fi, registers routes, and starts the server. |
| `loop()` | Calls `server.handleClient()`. |

The embedded JavaScript handles button events, status polling, UI updates, and the SVG animation.

## Timing and Current Limitations

- **Blocking request handling:** `handleObjectDetected()` uses `delay()`. While the approximately 1.9-second cycle runs, the main loop cannot service other HTTP requests. One-second polling does not guarantee that the dashboard displays intermediate backend states; responses normally show the completed Standby state.
- **Independent animation:** the browser runs a roughly 2.6-second animation using local timers. It is illustrative and is not synchronized with backend state transitions. It starts immediately, even if the simulation request later fails.
- **Single simulated servo:** this version models one servo angle. It has no separate blue-sorting branch, two-servo control, ultrasonic distance measurement, or forward/reverse/coasting motor modes.
- **Local button guard:** `isPlaying` prevents repeated clicks only in the current browser page. There is no server-side busy rejection or explicit object queue.
- **Input handling:** arbitrary color values use the unknown-color branch. `lastColor` is inserted into JSON without escaping, so unusual inputs containing quotes or control characters can produce invalid JSON.
- **Access control:** the HTTP routes have no authentication and are intended for a local demonstration environment.

## Suggested Manual Checks

These are expected outcomes derived from the source, not results of a hardware test.

| Action | Expected outcome |
| --- | --- |
| Restart the ESP32 and open the dashboard. | Standby state, stopped conveyor, idle servo, and zero counters. |
| Simulate one red object. | Red = 1, green = 0, total = 1; final stored angle = 180°. |
| Then simulate one green object. | Red = 1, green = 1, total = 2; final stored angle = 0°. |
| Use the IR simulation button once. | Either red or green increases by one; total increases by one. |
| Request `/simulate?color=UNKNOWN`. | Only total increases; final stored angle = 90°. |
| Refresh the browser. | ESP32 counters remain unchanged. |
| Restart the ESP32. | Counters reset to zero. |

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| Serial Monitor repeatedly prints dots. | Check Wi-Fi credentials and network availability; startup waits indefinitely for connection. |
| Dashboard does not open. | Use the IP printed by the current boot, include `http://`, and ensure the client can reach the ESP32. |
| Dashboard mostly displays Standby. | This is expected with the current blocking request handler; use the log to inspect the state sequence. |
| Animation plays but counters do not change. | Check ESP32 connectivity and the browser console; animation runs independently of request success. |
| Servo or motor does not move. | Physical output control is not implemented in this simulation. |

## Extending the Project

Possible future changes include replacing simulated detections with sensor readings, adding GPIO and actuator control, and replacing blocking delays with a `millis()`-based state machine. Server-side request coordination and input validation would also make the application more robust.

Hardware pin assignments, sensor calibration, motor direction control, and physical servo home positions must be defined when implementing a hardware version. They are not part of the supplied sketch.
