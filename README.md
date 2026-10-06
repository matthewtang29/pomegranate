# Custom Pomodoro Timer

A standalone Pomodoro study timer built on an Arduino Uno R3, programmed in C++, with a 16x2 LCD, four push buttons, and a buzzer. It can be controlled with its buttons or from a computer over USB through **Pomegranate**, a companion web app that also plans tasks and tracks study habits. The timer is housed in a custom 3D-printed case designed in Autodesk Fusion, with a magnet-secured two-piece design for easy access to the electronics.

## Features

**Timer (hardware)**
- **Auto mode:** classic 25 min focus / 5 min break, with a prompt to continue after each cycle
- **Custom mode:** choose focus length, break length, and number of cycles
- **Pause, resume, or exit** any running timer
- **Mute button**, saved in EEPROM so it's remembered after a restart
- Checkpoint and summary screens tracking completed sessions and total focus time

**Pomegranate companion app (web / installable)**
- **Timer tab:** set up and control sessions over USB, with a live countdown synced to the device
- **Built-in timer:** no device plugged in? Sessions run in the app itself (also on phones), with the same tracking, scores, and notifications
- **Timer alerts:** system notifications when a focus block, break, or session ends (with the physical or built-in timer), plus an option to keep the screen awake during sessions
- **Tasks tab:** a Todoist-style task list with natural-language quick add (`Lab report fri p1 d4 #ECE140`), priorities, difficulty ratings (1–5), subtasks, projects (rename, merge or delete them under Manage), and Today / Upcoming / All / Done views
- **Link tasks to sessions:** pick what you're working on and focus time is credited to that task
- **Calendar tab:** month view of tasks due, each day's productivity score, and your day rating (switch between showing both, productivity, or day rating)
- **Journal tab:** write about your day and rate it out of 10 (1–3 red, 4–5 orange, 6–8 yellow, 9–10 green), with a 14-day overview; ratings color the calendar and are kept separate from the productivity score
- **Analytics tab:** daily productivity score, desk / focus / break time compared with yesterday and last week, and up to 30 days of history
- **Installable app** that works offline and reconnects to the timer automatically
- **Themes:** a playful robot-lab look matching my website (chunky type, outlined cards, robot mascots), in system / light / dark, plus 8 accent colors or any custom color
- **Cloud sync:** sign in with Google to sync tasks, journal, study history and settings between computer and phone (Firebase)

## Using the app

Open the GitHub Pages link in Chrome or Edge, plug in the timer, and press **Connect**. The app uses the Web Serial API to talk to the Arduino over its USB cable, with no drivers needed.

**Install it as an app:** on the GitHub Pages site, click the install icon at the right end of the address bar (or **⬇ Install as an app** under the timer). It then opens in its own window with a Start menu / taskbar icon, works offline, and reconnects to the timer on launch.

All data (tasks, journal, study history, settings) is stored in the browser first, so the app works offline. Sign in from ⚙ Settings to sync it across devices, or use **Back up** / **Restore** on the Analytics tab to move it manually.

## Productivity score

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

## Cloud sync
Sync uses Firebase Authentication (Google sign-in) and Cloud Firestore on the free Spark plan. Tasks and journal entries are stored one per document and the newest change wins; deletions sync as markers. Study history is stored per device and added together for display, so two devices studying on the same day never overwrite each other.

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

## How it works

The firmware is a non-blocking state machine: timing uses `millis()` instead of `delay()`, so the buttons and USB commands are both handled at any moment, and the LCD and app always stay in sync. The app records time from the timer's once-a-second status messages.

### Serial protocol (115200 baud, one line per message)
| Computer → timer | Meaning |
|---|---|
| `START f b c` | Custom session: `f` min focus, `b` min break, `c` cycles |
| `AUTO` | Auto mode (25/5, asks to continue) |
| `PAUSE` / `RESUME` / `EXIT` | Control the running timer |
| `YES` / `NO` | Answer the continue prompt |
| `MUTE` / `UNMUTE` | Turn the buzzer off / on |
| `STATUS` | Request the current state |

The timer replies with `STATE ...` lines (every second and on every change), `EVENT ...` lines when a timer ends, and `ERR ...` if a command can't be done.

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
icons/                          App icons and robot mascots (icons/bots/)
fonts/                          Bungee and Nunito (SIL Open Font License), stored locally so the app works offline
```

## Hardware
- Arduino Uno R3
- 16x2 character LCD (LiquidCrystal library)
- 4 push buttons (pins 6, 8, 9, 10)
- Piezo buzzer (pin 7)
- 3D-printed enclosure (Autodesk Fusion)

## Controls
| Button | Menus | While a timer is running |
|---|---|---|
| Pin 9 | Next / increase | — |
| Pin 8 | Previous / decrease | — |
| Pin 10 | Select | Pause (then Resume or Exit) |
| Pin 6 | Mute / unmute | Mute / unmute |
