// Letting a banked creature go for good, from the party and from the box.
//
// This is an IRREVERSIBLE action on a round panel, which is the exact shape
// CLAUDE.md section 4 warns about, so what is pinned here is not "does release
// work" but the things that make it safe. There is no RELEASE button any more:
// the sheet's buttons are BRING BACK (left) and TO BOX / TO PARTY (right), and
// release is hidden behind a 2 s HOLD on the creature's name (y < 80).
//
//   * a tap on the name never arms anything -- only the hold does, and it only
//     arms a confirm
//   * NO really cancels, and the creature is still there afterwards
//   * the confirm is MODAL: a tap that would otherwise hit a move row underneath
//     it does nothing, rather than falling through to the picker
//   * a released creature is gone from BOTH the party and the box, not quietly
//     pushed from one into the other
//   * any half-finished box swap is disarmed, so nothing is left pointing at a
//     slot whose creature no longer exists
#include "Arduino.h"
#include "Arduino_GFX_Library.h"
#include "Preferences.h"
#include "pet.h"
#include "party.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
uint32_t g_seed=1234; FakeSerial Serial; FakeESP ESP; FakeWire Wire;
volatile int g_touchX=0,g_touchY=0; volatile bool g_touchDown=false;
void FakeESP::restart(){exit(0);}
int FakeSerial::available(){return 0;}
String FakeSerial::readStringUntil(char){return String("");}
void setup(); void loop(); void render();
void partyTap(int16_t,int16_t); void boxTap(int16_t,int16_t);
uint8_t uiCurrentScreen();
extern const char *const SCREEN_NAME[];
extern uint32_t confirmUntil;
extern uint8_t dimStage;
extern bool holdFired;
extern Pet pet;
extern bool partyOpen, boxOpen, movePickOpen, releaseConfirm;
extern uint8_t partyDetail, boxDetail, boxSwapFrom, boxSel;

static int bad=0;
static void ck(bool ok,const char*w){printf("%s  %s\n",ok?"PASS":"FAIL",w); if(!ok)bad++;}

// Geometry taken from the sketch's own constants rather than copied as numbers:
// a test that restates the layout drifts from it and then proves nothing.
#define PCELL_X(i) (78 + ((i) % 2) * 160)
#define PCELL_Y(i) (88 + ((i) / 2) * 78)
bool monSheetBtn(int16_t,int16_t,bool);
#define SHEET_L_X 160
#define SHEET_R_X 320
#define SHEET_BTN_Y (336 + 48/2)
#define CONF_YES_Y (206 + 52/2)
#define CONF_NO_Y  (268 + 52/2)
#define CONF_X 233

// handleTouch() self-gates to 50 Hz off millis(), so real time has to pass
// between polls; a tight spin is swallowed by that gate.
static void pump(int n){ for(int i=0;i<n;i++){ std::this_thread::sleep_for(std::chrono::milliseconds(30)); loop(); } }
static void tap(int x,int y){
  g_touchX=x; g_touchY=y; g_touchDown=true; emuFireInterrupt();
  pump(5);
  g_touchDown=false; emuFireInterrupt(); pump(3);
}
static void hold(int x,int y,int ms){
  g_touchX=x; g_touchY=y; g_touchDown=true; emuFireInterrupt();
  pump(ms/30);
  g_touchDown=false; emuFireInterrupt(); pump(3);
}

static PartyMon mon(int16_t dex, uint16_t lvl, const char *nick){
  PartyMon m; m.dex=dex; m.level=lvl; m.ivAtk=m.ivDef=m.ivSpe=m.ivHp=20;
  snprintf(m.nick,sizeof(m.nick),"%s",nick);
  return m;
}
// The name band at the top of the sheet, where the release hold lives.
#define NAME_X 233
#define NAME_Y 40
#define NAME_HOLD_MS 2400
// Wake the panel from the main screen: swallowGesture is sampled when the
// finger goes down, so a dimmed panel would eat the hold. (440,440) is off
// every control on the main screen.
static void wake(){
  partyOpen=boxOpen=false; partyDetail=boxDetail=0;
  tap(440,440);
}
static void clearAll(){
  for(int i=0;i<PARTY_SLOTS;i++) party.releaseAt(i);
  for(int i=0;i<BOX_SLOTS;i++) party.boxReleaseAt(i);
  partyOpen=boxOpen=movePickOpen=releaseConfirm=false;
  partyDetail=boxDetail=boxSwapFrom=boxSel=0;
}

int main(){
  setup();
  for(int i=0;i<4;i++) render();
  if (pet.awaitingStarter()) pet.chooseStarter(4);
  if (pet.isEgg()) pet.dbgHatchAs(6,false);
  pet.ageMinutes = 60UL*40;
  while (pet.hasLearnOffer()) pet.declineLearn();

  // ---- the party sheet: release is a HOLD on the name, then a confirm
  {
    clearAll();
    party.slots[0]=mon(25,30,"PIKA");
    party.save();
    wake();
    ck(dimStage==0, "the panel is awake, so the hold gate is what is tested");
    partyOpen=true;
    partyTap(PCELL_X(0)+40, PCELL_Y(0)+30);      // open its sheet
    ck(partyDetail==1, "tapping a party slot opens its sheet");
    ck(!releaseConfirm, "with no confirm up yet");

    tap(NAME_X, NAME_Y);                         // a plain tap on the name
    ck(!releaseConfirm, "a TAP on the name does not arm the release");
    ck(party.count()==1, "and nothing is let go");

    // reopen (the tap backed out). Disarm the swap first: the slot is still
    // armed from the first open, and a second tap on an armed slot opens the box
    boxSwapFrom=0;
    partyOpen=true;
    partyTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    ck(partyDetail==1 && !boxOpen, "(the sheet is open again)");
    hold(NAME_X, NAME_Y, NAME_HOLD_MS);
    ck(releaseConfirm, "holding the name arms a confirm");
    ck(party.count()==1, "and does NOT act by itself");

    // modal: y=170 is inside the panel and over move row 1, which would open
    // the picker if the confirm leaked taps through to what is under it
    movePickOpen=false;
    partyTap(233, 170);
    ck(!movePickOpen, "the confirm is modal: a move row underneath it is inert");
    ck(party.count()==1, "and the creature is still there");

    partyTap(CONF_X, CONF_NO_Y);                 // NO
    ck(!releaseConfirm, "NO closes the confirm");
    ck(party.count()==1 && party.slots[0].dex==25, "and keeps the creature");

    hold(NAME_X, NAME_Y, NAME_HOLD_MS);          // hold again
    ck(releaseConfirm, "the hold arms it again");
    partyTap(CONF_X, CONF_YES_Y);                // YES
    ck(party.count()==0, "YES lets it go");
    ck(party.boxCount()==0, "and it does NOT reappear in the box");
    ck(!releaseConfirm && partyDetail==0, "the sheet closes behind it");
  }

  // ---- it survives a reload: this is NVS, not just the array in memory
  {
    Party q; q.begin();
    ck(q.count()==0, "the release is persisted, not only in RAM");
  }

  // ---- the box sheet: the same hold, the same confirm
  {
    clearAll();
    party.box[0]=mon(133,25,"EEVEE");
    party.boxSave();
    wake();
    partyOpen=true; boxOpen=true;
    boxTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    ck(boxDetail==1, "tapping a box slot opens its sheet");
    ck(party.count()==0, "rather than moving the creature on one tap");

    hold(NAME_X, NAME_Y, NAME_HOLD_MS);
    ck(releaseConfirm, "holding the name arms a confirm on the box sheet too");
    boxTap(CONF_X, CONF_NO_Y);
    ck(party.boxCount()==1, "NO keeps it");

    hold(NAME_X, NAME_Y, NAME_HOLD_MS);
    boxTap(CONF_X, CONF_YES_Y);
    ck(party.boxCount()==0, "YES lets a boxed creature go");
    ck(party.count()==0, "and it is not pushed into the party instead");
  }

  // ---- the right-hand button never releases anything
  {
    clearAll();
    party.slots[0]=mon(25,30,"PIKA");
    party.save();
    partyOpen=true;
    partyTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    partyTap(SHEET_R_X, SHEET_BTN_Y);            // TO BOX
    ck(!releaseConfirm, "TO BOX does not arm a release");
    ck(party.count()==0 && party.boxCount()==1 && party.box[0].dex==25,
       "TO BOX moves the creature into the first free box slot");
  }

  // ---- TO PARTY (the box sheet's right button)
  {
    clearAll();
    party.box[0]=mon(143,40,"SNORLAX");
    party.boxSave();
    partyOpen=true; boxOpen=true;
    boxTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    boxTap(SHEET_R_X, SHEET_BTN_Y);              // TO PARTY
    ck(party.count()==1 && party.slots[0].dex==143, "TO PARTY withdraws it");
    ck(party.boxCount()==0, "and the box slot is freed");
  }

  // ---- with a FULL party: the sheet still opens, and TO PARTY asks who steps out
  {
    clearAll();
    for (int i=0;i<PARTY_SLOTS;i++) party.slots[i]=mon(10+i,20,"");
    party.save();
    party.box[0]=mon(143,40,"SNORLAX");
    party.boxSave();
    partyOpen=true; boxOpen=true;
    boxTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    ck(boxDetail==1, "a box creature's sheet opens even with a full party");
    boxTap(SHEET_R_X, SHEET_BTN_Y);              // TO PARTY
    ck(boxSel==1 && !boxOpen && boxDetail==0,
       "TO PARTY with a full party goes to choose who steps out");
    partyTap(PCELL_X(0)+40, PCELL_Y(0)+30);      // that one
    ck(party.slots[0].dex==143 && party.box[0].dex==10,
       "and the chosen one swaps places with it");
    ck(boxSel==0, "and nothing is left armed");
  }

  // ---- a released creature cannot leave a swap armed at an empty slot
  {
    clearAll();
    party.slots[0]=mon(25,30,"PIKA");
    party.save();
    partyOpen=true;
    partyTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    ck(boxSwapFrom==1, "opening a sheet arms the swap side");
    hold(NAME_X, NAME_Y, NAME_HOLD_MS);
    partyTap(CONF_X, CONF_YES_Y);
    ck(party.count()==0, "(released)");
    ck(boxSwapFrom==0 && boxSel==0,
       "and releasing disarms it, so nothing points at a creature that is gone");
  }

  // ---- BRING BACK over a hatched pet: the live one takes the slot
  {
    clearAll();
    party.slots[0]=mon(25,30,"PIKA");
    party.save();
    if (pet.isEgg()) pet.dbgHatchAs(6,false);
    while (pet.hasLearnOffer()) pet.declineLearn();
    int16_t was = pet.speciesId;
    partyOpen=true;
    partyTap(PCELL_X(0)+40, PCELL_Y(0)+30);
    partyTap(SHEET_L_X, SHEET_BTN_Y);            // BRING BACK
    ck(pet.speciesId==25, "BRING BACK works with a hatched pet too");
    ck(party.slots[0].dex==was, "and the pet that was alive takes its party slot");
    ck(!partyOpen, "and the screen returns to the pet");
  }

  // ---- BRING BACK, tapped where a thumb actually lands
  //
  // With an egg waiting -- the state a new board is set up in -- it must work
  // (the egg is held for later), AND it must work from the panel's centre line,
  // which is where the button used to be drawn full-width and where a thumb
  // goes by default.
  {
    clearAll();
    party.slots[0] = mon(3, 100, "");
    party.save();
    if (!pet.isEgg()) { pet.newEgg(); }
    if (pet.awaitingStarter()) pet.chooseStarter(4);
    partyOpen = true;
    partyTap(PCELL_X(0) + 40, PCELL_Y(0) + 30);
    ck(partyDetail == 1, "the sheet opens with an egg waiting");
    partyTap(233, SHEET_BTN_Y);                  // dead centre
    ck(pet.speciesId == 3, "BRING BACK works when tapped at the panel centre");
    ck(party.count() == 0, "and the creature leaves the party to become the pet");
    ck(pet.eggHeld, "and the egg that was waiting is held for later, not lost");
  }

  // ---- the 3 s hold, and WHERE it is allowed to fire
  //
  // The hold opens "release the live pet?" -- unconfirmed, irreversible, and
  // drawn over the creature. It used to be gated by a hand-kept list of screens
  // to EXCLUDE (gallery, card, keyboard, clock), which left it live on the party
  // screen, whose grid overlaps inPetZone. Worse, that dialog's YES box lands on
  // top of party slot 4: hold a slot, tap where you think a creature is, and
  // lose the one you are actually raising.
  //
  // BOTH halves are asserted. "It never fires on the party screen" is vacuously
  // true if the hold has stopped working everywhere, which would delete a real
  // feature and still pass -- and an earlier version of this check did exactly
  // that, silently, until the old gate was put back and it stayed green.
  {
    clearAll();
    party.slots[0]=mon(25,30,"PIKA");
    party.save();
    if (pet.isEgg()) pet.dbgHatchAs(6,false);
    while (pet.hasLearnOffer()) pet.declineLearn();
    if (pet.sleeping) pet.toggleLight();

    // Wake first: swallowGesture is (dimStage > 0 || screenOff), sampled when the
    // finger goes down, so a dimmed panel eats the hold before the gate is even
    // consulted. (440,440) is off every control on both screens.
    partyOpen=true;
    tap(440,440);
    ck(dimStage==0, "the panel is awake, so the gate is what is being tested");
    ck(!strcmp(SCREEN_NAME[uiCurrentScreen()],"party"), "and the party screen is up");
    confirmUntil=0; holdFired=false;
    hold(153,123,3400);                     // dead centre of party slot 0
    ck(confirmUntil==0, "holding a party slot does NOT open the release dialog");
    ck(partyDetail==0 || !holdFired, "and it was a hold, not a tap that opened a sheet");

    clearAll();
    tap(440,440);
    ck(dimStage==0, "still awake for the companion check");
    ck(!strcmp(SCREEN_NAME[uiCurrentScreen()],"main"),
       "and back on the main screen, or the check below proves nothing");
    confirmUntil=0; holdFired=false;
    hold(233,200,3400);                     // the creature itself
    ck(confirmUntil!=0, "the hold STILL works on the main screen, where it belongs");
    confirmUntil=0;
  }

  printf("%s\n", bad?"FAILURES":"all good");
  return bad?1:0;
}
