#include <LiquidCrystal.h>

//---------------------------------------- HARDWARE SETUP ----------------------------------------
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int buzzerpin    = 7;
const int rightButton  = 8;   // previous / decrease
const int leftButton   = 9;   // next / increase
const int selectButton = 10;  // select, and pause while a timer is running

//music tone setup for "SO" (G), "LA" (A), "TI" (B), "DO" (C)
int abc[] = {3920, 4400, 4940, 5230};

//---------------------------------------- CUSTOM MODE RANGES ------------------------------------
// Change these to change what the CUSTOM menu offers.
const int TIMER_MIN  = 25, TIMER_MAX  = 60, TIMER_STEP = 5;   // focus length (minutes)
const int BREAK_MIN  = 5,  BREAK_MAX  = 30, BREAK_STEP = 5;   // break length (minutes)
const int CYCLES_MIN = 1,  CYCLES_MAX = 6;                    // number of focus+break cycles

// Current custom settings (the menu remembers your last choice)
int focusMins = 25;
int breakMins = 5;
int cycles    = 1;

//---------------------------------------- SESSION STATE -----------------------------------------
int  count = 0;            // focus sessions completed
int  totalFocus = 0;       // total minutes focused
bool exitRequested = false;

char line[17];             // 16 characters + end-of-string

//---------------------------------------- SETUP -------------------------------------------------
void setup(){
  lcd.begin(16,2);
  pinMode(rightButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(buzzerpin, OUTPUT);

  lcd.setCursor(3,0);
  lcd.print("WELCOME TO");
  lcd.setCursor(4,1);
  lcd.print("POMODORO");
  for(int i = 0; i < 4; i++){ //playing welcoming melody
    mtone(buzzerpin, abc[i], 500);
    delay(50);
  }
}

//---------------------------------------- MAIN MENU ---------------------------------------------
void loop(){
  int choice = chooseTwo("---- OPTION ----", "AUTO", "CUSTOM", 0);
  if (choice == 0) runAuto();
  else             customMenu();
}

//---------------------------------------- BUTTON HELPERS ----------------------------------------
bool pressed(int pin){
  return !digitalRead(pin);
}

void waitRelease(int pin){
  delay(30);                    // debounce
  while(!digitalRead(pin));
  delay(30);
}

// Waits for any button, plays its tone, and returns which pin was pressed.
int waitButton(){
  while(true){
    if(pressed(leftButton)){   mtone(buzzerpin, abc[0], 50); waitRelease(leftButton);   return leftButton; }
    if(pressed(rightButton)){  mtone(buzzerpin, abc[0], 50); waitRelease(rightButton);  return rightButton; }
    if(pressed(selectButton)){ mtone(buzzerpin, abc[3], 50); waitRelease(selectButton); return selectButton; }
  }
}

//---------------------------------------- MENU SCREENS ------------------------------------------
// Two-option menu (AUTO/CUSTOM, YES/NO, RESUME/EXIT). Returns 0 for the first option, 1 for the second.
int chooseTwo(const char* title, const char* a, const char* b, int sel){
  while(true){
    lcd.clear();
    lcd.print(title);
    lcd.setCursor(0,1);
    snprintf(line, 17, " %c%-6s %c%-6s", sel == 0 ? '~' : ' ', a, sel == 1 ? '~' : ' ', b);
    lcd.print(line);

    int btn = waitButton();
    if      (btn == leftButton)  sel = 1;
    else if (btn == rightButton) sel = 0;
    else return sel;
  }
}

// Number picker used by the CUSTOM menu. Going below the minimum shows BACK.
// Returns the chosen value, or -1 if BACK was selected.
int pickValue(const char* title, int minV, int maxV, int step, int current,
              const char* unitOne, const char* unitMany){
  int backV = minV - step;
  int v = current;
  char text[17];

  while(true){
    if (v == backV) snprintf(text, 17, "< BACK >");
    else            snprintf(text, 17, "< %d %s >", v, v == 1 ? unitOne : unitMany);

    lcd.clear();
    lcd.print(title);
    lcd.setCursor((16 - strlen(text)) / 2, 1);
    lcd.print(text);

    int btn = waitButton();
    if      (btn == leftButton  && v < maxV)  v += step;
    else if (btn == rightButton && v > backV) v -= step;
    else if (btn == selectButton) return (v == backV) ? -1 : v;
  }
}

// CUSTOM -> TIMER -> BREAK -> CYCLES, then runs the session.
// Selecting BACK on any screen goes to the previous one.
void customMenu(){
  int stage = 0;
  while(true){
    if (stage == 0){
      int v = pickValue("----- TIMER ----", TIMER_MIN, TIMER_MAX, TIMER_STEP, focusMins, "MIN", "MIN");
      if (v < 0) return;                 // back to the main menu
      focusMins = v;
      stage = 1;
    }
    else if (stage == 1){
      int v = pickValue("----- BREAK ----", BREAK_MIN, BREAK_MAX, BREAK_STEP, breakMins, "MIN", "MIN");
      if (v < 0) { stage = 0; continue; }
      breakMins = v;
      stage = 2;
    }
    else {
      int v = pickValue("---- CYCLES ----", CYCLES_MIN, CYCLES_MAX, 1, cycles, "CYCLE", "CYCLES");
      if (v < 0) { stage = 1; continue; }
      cycles = v;
      runCustom(focusMins, breakMins, cycles);
      return;
    }
  }
}

//---------------------------------------- SESSIONS ----------------------------------------------
// AUTO: 25 min focus + 5 min break, then asks whether to continue.
void runAuto(){
  count = 0;
  totalFocus = 0;
  exitRequested = false;

  while(true){
    getReady();
    if (!runTimer("--- FOCUSING ---", 25)) break;
    count++;
    totalFocus += 25;
    focusDone();

    if (!runTimer("----- REST -----", 5)) break;
    checkpoint();

    if (chooseTwo("-- CONTINUE ? --", "YES", "NO", 0) == 1) break;
  }
  showSummary();
}

// CUSTOM: runs the chosen number of focus + break cycles.
// The break after the final cycle is skipped.
void runCustom(int focus, int brk, int n){
  count = 0;
  totalFocus = 0;
  exitRequested = false;

  for (int i = 1; i <= n; i++){
    getReady();
    if (!runTimer("--- FOCUSING ---", focus)) break;
    count++;
    totalFocus += focus;
    if (i == n) break;                   // last cycle: no break

    focusDone();
    if (!runTimer("----- REST -----", brk)) break;
    checkpoint();
  }

  if (!exitRequested){
    lcd.clear();
    lcd.print("SESSION");
    lcd.setCursor(0,1);
    lcd.print("ENDED");
    for (int i = 0; i < 3; i++){
      digitalWrite(buzzerpin, HIGH);
      delay(100);
      digitalWrite(buzzerpin, LOW);
      delay(100);
    }
    delay(2000);
  }
  showSummary();
}

//---------------------------------------- TIMER -------------------------------------------------
// Counts down 'mins' minutes. Returns true if it finished, false if the user chose EXIT.
bool runTimer(const char* title, int mins){
  long remaining = (long)mins * 60;

  lcd.clear();
  lcd.print(title);

  while (remaining > 0){
    showTime(remaining, false);
    if (!timerWait(300, title, remaining)) return false;
    showTime(remaining, true);
    if (!timerWait(700, title, remaining)) return false;
    remaining--;
  }
  showTime(0, true);
  delay(1000);
  return true;
}

// Prints MM :: SS on the bottom row (colon blinks).
void showTime(long secs, bool colon){
  int m = secs / 60;
  int s = secs % 60;
  if (colon) snprintf(line, 17, "    %02d :: %02d  ", m, s);
  else       snprintf(line, 17, "    %02d    %02d  ", m, s);
  lcd.setCursor(0,1);
  lcd.print(line);
}

// Like delay(ms), but pressing select opens the pause menu.
// Time spent paused isn't counted. Returns false if the user chose EXIT.
bool timerWait(unsigned long ms, const char* title, long remaining){
  unsigned long start = millis();
  while (millis() - start < ms){
    if (pressed(selectButton)){
      unsigned long elapsed = millis() - start;
      if (!pauseMenu(remaining)){
        exitRequested = true;
        return false;
      }
      lcd.clear();                       // back to the timer screen
      lcd.print(title);
      showTime(remaining, true);
      start = millis() - elapsed;        // finish the rest of this wait
    }
  }
  return true;
}

// Shows PAUSED with the time left, and lets the user RESUME or EXIT.
// Returns true to resume, false to exit.
bool pauseMenu(long remaining){
  mtone(buzzerpin, abc[3], 50);
  waitRelease(selectButton);

  char title[17];
  snprintf(title, 17, "PAUSED     %02d:%02d", (int)(remaining / 60), (int)(remaining % 60));
  return chooseTwo(title, "RESUME", "EXIT", 0) == 0;
}

//---------------------------------------- SCREENS ----------------------------------------------
void getReady(){
  lcd.clear();
  lcd.print("---- ALERT! ----");
  buzz();
  for (int i = 5; i >= 0; i--){
    lcd.setCursor(1,1);
    lcd.print("GET READY IN ");
    lcd.setCursor(14,1);
    lcd.print(i);
    delay(1000);
  }
}

void focusDone(){
  lcd.clear();
  lcd.print("-- COMPLETED! --");
  buzz();
  lcd.setCursor(3,1);
  lcd.print("REST TIME");
  delay(2000);
}

void checkpoint(){
  lcd.clear();
  lcd.print("-- CHECKPOINT --");
  digitalWrite(buzzerpin, HIGH);
  delay(100);
  digitalWrite(buzzerpin, LOW);
  lcd.setCursor(0,1);
  printCount();
  delay(1500);
}

void showSummary(){
  if (exitRequested){
    lcd.clear();
    lcd.print("- TIMER EXITED -");
    buzz();
    delay(1500);
  }
  lcd.clear();
  lcd.print("--- SUMMARY ----");
  lcd.setCursor(0,1);
  printCount();
  delay(2500);
}

// COUNT = focus sessions finished, then total focus time as HH:MM
void printCount(){
  snprintf(line, 17, "COUNT:%02d   %02d:%02d", count, totalFocus / 60, totalFocus % 60);
  lcd.print(line);
}

//---------------------------------------- SOUND -------------------------------------------------
void buzz(){
  for (int i = 0; i < 3; i++){
    digitalWrite(buzzerpin, HIGH);
    delay(50);
    digitalWrite(buzzerpin, LOW);
    delay(50);
  }
}

void mtone(int dx, int hz, unsigned long tm){
  unsigned long t = millis();
  unsigned long ns = (long)500000 / hz;
  while (millis() - t < tm){
    digitalWrite(dx, HIGH);
    delayMicroseconds(ns);
    digitalWrite(dx, LOW);
    delayMicroseconds(ns);
  }
}