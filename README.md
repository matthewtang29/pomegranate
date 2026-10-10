# Pomodoro Timer

A Pomodoro study timer in two halves:

- **The timer (hardware):** a standalone device built on an Arduino Uno R3, with a 16x2 LCD, four push buttons and a buzzer, housed in a custom 3D-printed case designed in Autodesk Fusion.
- **Pomegranate (software):** an installable web app that controls the timer over USB and doubles as a study hub, with a task list, calendar, daily journal and productivity analytics, synced between devices.

Each half works on its own. The timer runs from its buttons with no computer attached, and Pomegranate has a built-in timer for when the device isn't plugged in. Together, the LCD and the app stay in sync second by second.

---

## Contents
1. [Hardware: the timer](#hardware-the-timer)
2. [Software: Pomegranate](#software-pomegranate)
3. [How they talk: serial protocol](#how-they-talk-serial-protocol)
4. [Project structure](#project-structure)
5. [Testing](#testing)
6. [What's next](#whats-next)

---

## Hardware: the timer

### Features
- **Auto mode:** classic 25 min focus / 5 min break, with a prompt to continue after each cycle
- **Custom mode:** choose focus length (25–60 min), break length (5–30 min) and number of cycles (1–6) on the device, or longer from the app
- **Pause, resume or exit** any running timer
- **Mute button**, saved in EEPROM so it's remembered after a restart
- Checkpoint and summary screens tracking completed sessions and total focus time
- Audio alerts for button presses, phase changes and session completion

### Parts
| Part | Function |
|---|---|
| Arduino Uno R3 | Runs the timer logic, buttons, display and USB communication |
| 16x2 character LCD | Shows menus, the countdown and status (FOCUSING, PAUSED, …) |
| 10 kΩ potentiometer | Adjusts LCD contrast |
| 220 Ω resistor | Limits current to the LCD backlight |
| 4 push buttons | Navigate menus, select / pause, mute |
| Piezo buzzer | Button clicks and phase-change alerts |
| Magnets | Hold the two halves of the case together |
| PLA filament | 3D-printed case |

### Wiring
| Component | Arduino pin |
|---|---|
| LCD RS / E | 12 / 11 |
| LCD D4, D5, D6, D7 | 5, 4, 3, 2 |
| LCD V0 (contrast) | Potentiometer wiper |
| Buzzer | 7 |
| Right button | 8 |
| Left button | 9 |
| Select button | 10 |
| Mute button | 6 |

Each button connects its pin to GND. The firmware enables the Arduino's internal pull-up resistors (`INPUT_PULLUP`), so no external resistors are needed: a pin reads `HIGH` normally and `LOW` when pressed.

### Controls
| Button | In menus | While a timer is running |
|---|---|---|
| Pin 9 | Next / increase | — |
| Pin 8 | Previous / decrease | — |
| Pin 10 | Select | Pause (then Resume or Exit) |
| Pin 6 | Mute / unmute | Mute / unmute |

### Firmware
The firmware (`firmware/Pomodoro_Timer_Code/`) is a **non-blocking state machine**. The timer is always in one state (`MAIN_MENU`, `FOCUS`, `REST`, `PAUSED`, `SUMMARY`, …). Each pass of `loop()` reads USB commands and buttons, checks the clock with `millis()`, moves to the next state if a phase has ended, and redraws the screen:

```cpp
void loop(){
  readSerial();
  int btn = readButtons();
  if (btn >= 0) handleButton(btn);
  update();
  draw();
}
```

Since nothing uses `delay()`, the buttons and USB commands are handled at any moment. Pausing stores the milliseconds left, and resuming sets a new end time from it. Buttons are debounced with a 30 ms window.

**Uploading:** open `firmware/Pomodoro_Timer_Code/Pomodoro_Timer_Code.ino` in the Arduino IDE, select **Arduino Uno** and the right port, then upload. Close Pomegranate (or press Disconnect) first, since only one program can use the port at a time.

### Case
Designed in Autodesk Fusion and 3D printed in two pieces: a base that holds the electronics, with cutouts for the LCD and USB cable, and a decorative lid. The halves are held together with magnets pressed into pockets around the rim, so the case opens without tools.

---

## Software: Pomegranate

### Features
- **Timer tab:** set up and control sessions over USB, with a live countdown synced to the device
- **Built-in timer:** no device plugged in? Sessions run in the app itself (also on phones), with the same tracking, scores and notifications
- **Timer alerts:** system notifications when a focus block, break or session ends, plus an option to keep the screen awake during sessions
- **Tasks tab:** a Todoist-style task list with natural-language quick add (`Lab report fri p1 d4 #ECE140`), priorities, difficulty ratings (1–5), projects, and Today / Upcoming / All / Done views
- **Link tasks to sessions:** pick what you're working on and focus time is credited to that task
- **Calendar tab:** month view of tasks due, each day's productivity score and your day rating
- **Journal tab:** write about your day and rate it out of 10 (1–3 red, 4–5 orange, 6–8 yellow, 9–10 green), with a 14-day overview; ratings colour the calendar and are kept separate from the productivity score
- **Analytics tab:** daily productivity score, desk / focus / break time compared with yesterday and last week, and up to 30 days of history
- **Installable app** that works offline and reconnects to the timer automatically
- **Themes:** system / light / dark, plus 8 accent colours or any custom colour
- **Cloud sync:** sign in with Google to sync tasks, journal, study history and settings between computer and phone
- **Export and backup:** CSV export, plus Back up / Restore of all data to a file

### Using the app
Open the GitHub Pages link in **Chrome or Edge**, plug in the timer, and press **Connect**. The app uses the **Web Serial API** to talk to the Arduino over its USB cable, with no drivers needed. (Other browsers can still use the built-in timer and everything else.)

**Install it as an app:** click the install icon at the right end of the address bar (or **⬇ Install as an app** under the timer). It then opens in its own window with a Start menu / taskbar icon, works offline, and reconnects to the timer on launch. On iPhone, use Share → Add to Home Screen.

All data is stored in the browser first, so the app works offline. Sign in from ⚙ Settings to sync across devices, or use **Back up** / **Restore** on the Analytics tab to move it manually.

### Productivity score
**Session score (0–100)**, for each timer session:
| Part | Weight | Measures |
|---|---|---|
| Completion | 45% | Finished all cycles or checked off a task during the session; otherwise the share of planned focus done |
| Efficiency | 35% | Focus share of desk time, relative to what the focus/break plan allows (long pauses lower it) |
| Intensity | 20% | Focus-time-weighted difficulty of the linked task(s) ÷ 5 |

**Daily score (0–100):**
| Part | Weight | Measures |
|---|---|---|
| Focus time | 40% | Focus minutes vs. a daily goal (adjustable) |
| Tasks done | 30% | Difficulty-weighted share of tasks due that day that got done |
| Session quality | 30% | Focus-weighted average of that day's session scores |

Parts with nothing to measure are left out and the rest are rescaled.

### Cloud sync
Sync uses **Firebase Authentication** (Google sign-in) and **Cloud Firestore** on the free Spark plan. Tasks and journal entries are stored one per document and the newest change wins; deletions sync as markers. Study history is stored per device and added together for display, so two devices studying on the same day never overwrite each other.

Firestore security rules (only the signed-in user can read or write their own data):
```
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /users/{uid}/{document=**} {
      allow read, write: if request.auth != null && request.auth.uid == uid;
    }
  }
}
```

### Built with
HTML, CSS and JavaScript (no frameworks) · Web Serial API · PWA (manifest + service worker) · Firebase Auth + Firestore · Notifications and Screen Wake Lock APIs · GitHub Pages

---

## How they talk: serial protocol

The timer and app exchange one line of text per message over USB at **115200 baud**. The built-in timer in the app speaks the same protocol, so the rest of the app works identically with or without the device.

| App → timer | Meaning |
|---|---|
| `START f b c` | Custom session: `f` min focus, `b` min break, `c` cycles |
| `AUTO` | Auto mode (25/5, asks to continue) |
| `PAUSE` / `RESUME` / `EXIT` | Control the running timer |
| `YES` / `NO` | Answer the continue prompt |
| `MUTE` / `UNMUTE` | Turn the buzzer off / on |
| `STATUS` | Request the current state |

| Timer → app | Meaning |
|---|---|
| `STATE state=… left=… cycle=… cycles=… …` | Current state and time left, sent every second and on every change |
| `EVENT FOCUS_DONE` / `REST_DONE` / `SESSION_ENDED` / `EXITED` | A phase or session ended |
| `ERR BUSY` / `RANGE` / `STATE` / `UNKNOWN` | A command couldn't be done |

The app's display is driven by the timer's `STATE` lines, which is what keeps the LCD and app in sync. The app also records desk, focus and break time from them.

---

## Project structure
```
firmware/Pomodoro_Timer_Code/   Arduino sketch (open in the Arduino IDE)
index.html, style.css           App layout and styles
app.js                          USB connection and timer controls
localtimer.js                   Built-in timer (same protocol as the Arduino)
notify.js                       Timer-finished notifications and keep-awake
analytics.js                    Study time tracking and Analytics tab
score.js                        Session and daily productivity scores
tasks.js                        Tasks tab and quick-add parser
calendar.js                     Calendar tab
journal.js                      Journal tab (day ratings)
sync.js                         Google sign-in and cloud sync
theme.js                        Theme settings
main.js                         Tabs, install-as-app, start-up
manifest.webmanifest, sw.js     Installable app (PWA) setup
icons/                          App icons
```

---

## Testing
- **Firmware:** compiled with the AVR toolchain (42% flash, 38% RAM) and run in a simulator built from the same code, fed USB commands to check every state change.
- **App:** automated browser tests in Playwright, about 150 checks across timer control, tasks, scoring, journal, the built-in timer, themes, sync and notifications. A fake serial port connects the app to the firmware simulator, and a shared fake Firebase backend tests sync between two simulated devices.

---

## What's next
The next version turns the timer into a small, wireless desk robot about the size of an action figure:
- **ESP32-S3 with a round 1.28" touchscreen:** the screen becomes the robot's face and countdown ring, touch replaces the buttons, and a built-in motion sensor enables gestures like shake to pause
- **Bluetooth Low Energy:** connects to Pomegranate wirelessly, using the same text protocol as the USB version
- **Battery power:** a LiPo charged over USB-C, plus a vibration motor for silent alerts
- **Custom PCB:** a small board designed in Altium for the buzzer, motor driver, power switch and buttons, replacing loose wiring
- **A more compact 3D-printed body** designed around the new screen and board
