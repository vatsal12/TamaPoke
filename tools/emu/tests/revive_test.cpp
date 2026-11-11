// Bringing a banked creature back as THE pet.
//
// It used to come back FROZEN (no ageing, no evolving, could not be lost). It
// no longer does: brought back, it is an ordinary pet again -- it grows,
// evolves and can end like any other, and only stays frozen while it sits in
// the party or the box. What is pinned here:
//   * reviveFrom keeps what was banked (level, shiny, name, moves)
//   * and is NOT frozen: it ages, the farewell is held off for a day, no debt
//   * a creature banked with no moves at all relearns its level's moves
//   * callBack() swaps the live creature into the slot (nothing is lost)
//   * over an egg, the egg is HELD and the next newEgg() resumes the same one
//   * callBack() refuses during the starter pick and during a ceremony
#include "Arduino.h"
#include "Preferences.h"
#include "pet.h"
#include "party.h"
#include "dex.h"
#include <cstdio>
uint32_t g_seed=11; FakeSerial Serial; FakeESP ESP; FakeWire Wire;
volatile int g_touchX=0,g_touchY=0; volatile bool g_touchDown=false; bool wasPressed=false;
static uint32_t g_ms=0; uint32_t millis(){return g_ms;}
void FakeESP::restart(){exit(0);}
int FakeSerial::available(){return 0;} String FakeSerial::readStringUntil(char){return String("");}
void sfxPlay(uint8_t){}
static int bad=0;
static void ck(bool ok,const char*w){printf("%s  %s\n",ok?"PASS":"FAIL",w); if(!ok)bad++;}

static PartyMon banked(int16_t dex, uint16_t lvl, const char *nick){
  PartyMon m; m.dex=dex; m.level=lvl; m.shiny=1;
  m.ivAtk=m.ivDef=m.ivSpe=m.ivHp=24; m.trAtk=m.trDef=m.trSpe=35;
  m.moves[0]=1; m.moves[1]=2;
  snprintf(m.nick,sizeof(m.nick),"%s",nick);
  return m;
}

int main(){
  // ---- reviveFrom: what comes back, and that it is no longer frozen
  {
    Pet p; p.begin();
    if (p.awaitingStarter()) p.chooseStarter(4);
    PartyMon m = banked(6, 61, "BLAZE");

    p.reviveFrom(m);
    ck(p.speciesId==6 && p.level()==61, "comes back at the level it was banked at");
    ck(p.shiny && !strcmp(p.nick,"BLAZE"), "keeps its shininess and its name");
    ck(p.moves[0]==1 && p.moves[1]==2, "and its moveset");
    ck(!p.frozen, "and is NOT frozen any more");
    ck(p.evoPenalty()==0, "and owes no evolution debt");

    // it ages again
    uint32_t age0 = p.ageMinutes;
    for (int i=0;i<200;i++){ g_ms += 60000; p.update(g_ms);
      p.fullness=p.joy=p.energy=p.hygiene=100; }
    printf("     after ~200 game-minutes: age %u -> %u, level %u\n",
           (unsigned)age0, (unsigned)p.ageMinutes, p.level());
    ck(p.ageMinutes > age0 && p.level() > 61, "it ages and levels like any other pet");

    // a long-lived one (level 80 = past FAREWELL_AGE_MIN in final form) must not
    // be greeted with the goodbye button -- a day's grace, as "stay together" gives
    p.reviveFrom(banked(6, 80, "OLD"));
    p.fullness=p.joy=p.energy=p.hygiene=100;
    ck(p.canFarewellNow(), "an old final-form creature has earned its farewell");
    ck(!p.wantFarewellButton(), "but it is not offered the moment it comes back");
    p.ageMinutes += 1440;
    ck(p.wantFarewellButton(), "only a day later");

    // it survives a reload, still not frozen
    p.saveNow();
    Pet q; q.begin();
    ck(!q.frozen && q.speciesId==6, "stays an ordinary pet across a reload");
  }

  // ---- banked with no moves (BANK over USB): it relearns its level's moves
  {
    Pet p; p.begin();
    PartyMon m = banked(25, 30, "PIKA");
    for (int i=0;i<MOVE_SLOTS;i++) m.moves[i]=0;
    p.reviveFrom(m);
    int n=0; for (int i=0;i<MOVE_SLOTS;i++) if (p.moves[i]) n++;
    ck(n>0, "a creature banked with no moves comes back knowing some");
  }

  // ---- callBack over a hatched pet: the live one takes the slot
  {
    Pet p; p.begin();
    p.dbgHatchAs(1,false);
    while (p.hasLearnOffer()) p.declineLearn();
    PartyMon out;
    int r = p.callBack(banked(6, 40, "BLAZE"), out);
    ck(r==1, "calling back over a hatched pet returns 1");
    ck(p.speciesId==6 && p.level()==40, "the called-back creature is now the pet");
    ck(out.dex==1 && !out.empty(), "and the one that was alive is handed back for the slot");
  }

  // ---- callBack over an egg: the egg is held, and the same egg comes back
  {
    Pet p; p.begin();
    p.newEgg();
    if (p.awaitingStarter()) p.chooseStarter(4);
    ck(p.isEgg(), "an egg is waiting");
    int16_t target = p.eggPeek();
    int16_t regs[REGION_COUNT]; memcpy(regs, p.eggByRegion, sizeof(regs));
    PartyMon out;
    int r = p.callBack(banked(9, 45, "SHELL"), out);
    ck(r==0, "calling back over an egg returns 0");
    ck(out.empty(), "and nothing is handed back -- an egg is not banked");
    ck(p.eggHeld && p.speciesId==9, "the egg is held and the creature is the pet");

    Pet q; q.begin();
    ck(q.eggHeld, "the held egg survives a reload (egghld)");

    // end the called-back creature; the held egg returns, not a fresh roll
    p.release();
    g_ms += CEREMONY_MS + 1000; p.update(g_ms);
    ck(p.isEgg(), "when that creature ends, an egg is waiting again");
    ck(p.eggPeek()==target, "and it is the SAME egg, not a new roll");
    ck(!memcmp(regs, p.eggByRegion, sizeof(regs)), "with the same region answers");
    ck(!p.eggHeld, "and nothing is held any more");
    ck(p.endedKind==CER_RELEASE && p.endedMon.dex==9, "the released creature was banked too");
  }

  // ---- when callBack must refuse
  {
    Pet p; p.begin();
    p.dbgHatchAs(4,false);
    p.ageMinutes = 4UL*24*60;
    p.release();
    PartyMon out;
    ck(!p.canCallBack() && p.callBack(banked(6,40,"X"), out)==-1,
       "not while an ending is on screen");
  }

  printf("%s\n", bad?"FAILURES":"all good");
  return bad?1:0;
}
