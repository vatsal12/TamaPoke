// Equipo, caja y copia de seguridad (party.cpp / save.cpp) y como las acaba la
// mascota (pet.cpp): finales, traer de vuelta (callBack) y el huevo en espera.
#include "framework.h"
#include "shim/Arduino.h"
#include "shim/Preferences.h"
#include "shim/nvs_flash.h"
#include "../pet.h"
#include "../party.h"
#include "../save.h"
#include "../dex.h"
#include <string.h>
#include <vector>

// ---------------------------------------------------------------- helpers
static void hatchAs(Pet &p, int16_t dex) {
  mockNvsReset();
  mockNvsSetPartitionMissing(false);
  mockNvsSetInitResult(ESP_OK, 0);
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(dex);
  p.eggTap(); p.eggTap(); p.eggTap();
}

// deja correr la ceremonia hasta el final (update() banca y pone el huevo)
static void finishCeremony(Pet &p) {
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
}

static PartyMon mon(int16_t dex, uint16_t level, const char *nick = "") {
  PartyMon m;
  m.dex = dex;
  m.level = level;
  m.ivAtk = 20; m.ivDef = 21; m.ivSpe = 22; m.ivHp = 23;
  m.trAtk = 5; m.trDef = 6; m.trSpe = 7;
  strncpy(m.nick, nick, sizeof(m.nick) - 1);
  return m;
}

// ---------------------------------------------------------------- finales
// Hoy: TODOS los finales bancan al bicho (antes solo despedida y liberacion)
TEST(ending, la_despedida_banca_al_bicho) {
  Pet p;
  hatchAs(p, 6);  // CHARIZARD
  p.ageMinutes = FAREWELL_AGE_MIN;
  p.rename("ZARDY");
  p.startFarewell();
  finishCeremony(p);
  CHECK(p.isEgg());
  CHECK_EQ(p.endedKind, (uint8_t)CER_FAREWELL);
  CHECK_EQ(p.endedMon.dex, (int16_t)6);
  CHECK_EQ(p.endedMon.level, (uint16_t)(1 + FAREWELL_AGE_MIN / MINUTES_PER_LEVEL));
  CHECK_STREQ(p.endedMon.nick, "ZARDY");
}

TEST(ending, soltar_banca_al_bicho) {
  Pet p;
  hatchAs(p, 4);
  uint8_t iv = p.ivAtk;
  p.release();
  finishCeremony(p);
  CHECK_EQ(p.endedKind, (uint8_t)CER_RELEASE);
  CHECK_EQ(p.endedMon.dex, (int16_t)4);
  CHECK_EQ(p.endedMon.ivAtk, iv);
}

TEST(ending, la_escapada_tambien_banca_pero_castiga_el_huevo) {
  Pet p;
  hatchAs(p, 4);
  p.dbgRunawayReady();
  CHECK(p.canRunawayNow());
  p.startRunaway();
  finishCeremony(p);
  CHECK_EQ(p.endedKind, (uint8_t)CER_RUNAWAY);
  CHECK_EQ(p.endedMon.dex, (int16_t)4);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_RUNAWAY);  // el siguiente huevo sigue castigado
}

TEST(ending, la_retirada_temprana_banca_y_cobra_un_dia_al_siguiente) {
  Pet p;
  hatchAs(p, 4);
  p.ageMinutes = 10 * 60;
  CHECK(p.canRetireNow());
  CHECK(!p.retireIsFree());
  p.startRetire();
  CHECK(p.retireIsEarly());
  CHECK_EQ(p.lastEnd, (uint8_t)CER_RELEASE);  // no bendice el huevo como una despedida
  finishCeremony(p);
  CHECK_EQ(p.endedKind, (uint8_t)CER_FAREWELL);
  CHECK_EQ(p.endedMon.dex, (int16_t)4);
  CHECK_EQ(p.endedMon.level, (uint16_t)11);
  CHECK_EQ(p.evoPenalty(), (uint8_t)EVO_PENALTY_LEVELS);
}

TEST(ending, un_huevo_no_banca_nada) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.release();
  finishCeremony(p);
  CHECK_EQ(p.endedKind, (uint8_t)CER_NONE);
}

// ---------------------------------------------------------------- traer de vuelta
// Hoy: reviveFrom ya NO congela. Crece, evoluciona y se puede despedir.
TEST(revive, traido_de_vuelta_no_queda_congelado) {
  Pet p;
  hatchAs(p, 4);
  PartyMon m = mon(4, 20, "CHAR");
  p.reviveFrom(m);
  CHECK(!p.frozen);
  CHECK_EQ(p.speciesId, (int16_t)4);
  CHECK_EQ(p.level(), (uint8_t)20);
  CHECK_EQ(p.ivHp, (uint8_t)23);
  CHECK_EQ(p.trSpe, (uint8_t)7);
  CHECK_STREQ(p.nick, "CHAR");
  uint32_t age = p.ageMinutes;
  for (int i = 0; i < 5; i++) { p.fullness = p.joy = p.energy = p.hygiene = 100; p.dbgTick(); }
  CHECK_EQ(p.ageMinutes, age + 5);   // envejece
  CHECK(p.canEvolveNow());           // nivel 20 >= 16 de CHARMANDER
  CHECK(p.canRetireNow());
}

// un guardado de v3.11 con un acompanante congelado (froz=true) ya no lo congela
TEST(revive, un_froz_antiguo_en_la_nvs_se_ignora) {
  Pet p;
  hatchAs(p, 4);
  Preferences pr;
  pr.begin("tamapoke", false);
  pr.putBool("froz", true);
  pr.end();
  Pet q;
  q.begin();
  CHECK(!q.frozen);
  uint32_t age = q.ageMinutes;
  q.dbgTick();
  CHECK_EQ(q.ageMinutes, age + 1);
}

TEST(revive, sin_movimientos_aprende_los_de_su_nivel) {
  Pet p;
  hatchAs(p, 4);
  PartyMon m = mon(4, 30);
  for (int i = 0; i < MOVE_SLOTS; i++) m.moves[i] = 0;   // banked por USB (BANK)
  p.reviveFrom(m);
  CHECK(p.moveCount() > 0);
}

TEST(revive, un_veterano_no_recibe_el_boton_de_despedida_de_golpe) {
  Pet p;
  hatchAs(p, 6);
  p.reviveFrom(mon(6, 90));          // ya pasado de la edad de despedida
  CHECK(p.canFarewellNow());
  CHECK_MSG(!p.wantFarewellButton(), "no se le recibe con el adios");
  p.ageMinutes += 1440;
  CHECK_MSG(p.wantFarewellButton(), "se ofrece al dia siguiente");
}

TEST(callback, el_bicho_vivo_ocupa_el_hueco_del_traido) {
  Pet p;
  hatchAs(p, 25);
  p.rename("PIKA");
  PartyMon out;
  int r = p.callBack(mon(7, 12, "SQUIRT"), out);
  CHECK_EQ(r, 1);
  CHECK_EQ(out.dex, (int16_t)25);
  CHECK_STREQ(out.nick, "PIKA");
  CHECK_EQ(p.speciesId, (int16_t)7);
  CHECK(!p.eggHeld);
}

TEST(callback, sobre_un_huevo_el_huevo_espera_y_vuelve_despues) {
  Pet p;
  hatchAs(p, 4);
  p.newEgg();                         // ya hay pokedex: huevo normal, sin elegir
  CHECK(p.isEgg());
  CHECK(!p.awaitingStarter());
  int16_t egg = p.eggPeek();
  PartyMon out;
  out.dex = 0;
  int r = p.callBack(mon(7, 12), out);
  CHECK_EQ(r, 0);
  CHECK(out.empty());
  CHECK(p.eggHeld);
  CHECK_EQ(p.speciesId, (int16_t)7);

  // el huevo en espera persiste en NVS ("egghld")
  Pet q;
  q.begin();
  CHECK(q.eggHeld);
  CHECK_EQ(q.speciesId, (int16_t)7);
  CHECK_EQ(q.eggPeek(), egg);

  // cuando el traido acaba, vuelve EL MISMO huevo en lugar de sortear otro
  q.release();
  finishCeremony(q);
  CHECK(q.isEgg());
  CHECK(!q.eggHeld);
  CHECK_EQ(q.eggPeek(), egg);
  CHECK_EQ(q.endedMon.dex, (int16_t)7);   // y el traido queda bancado
}

TEST(callback, no_se_puede_en_ceremonia_ni_eligiendo_inicial_ni_con_hueco_vacio) {
  Pet p;
  hatchAs(p, 4);
  PartyMon out;
  CHECK_EQ(p.callBack(PartyMon(), out), -1);
  p.release();
  CHECK(!p.canCallBack());
  CHECK_EQ(p.callBack(mon(7, 5), out), -1);
  CHECK_EQ(p.speciesId, (int16_t)4);

  mockNvsReset();
  Pet e;
  e.begin();
  CHECK(e.awaitingStarter());
  CHECK(!e.canCallBack());
  CHECK_EQ(e.callBack(mon(7, 5), out), -1);
  CHECK(e.isEgg());
}

// ---------------------------------------------------------------- caja
TEST(box, son_102_huecos_en_su_propia_particion) {
  CHECK_EQ(BOX_SLOTS, 102);
  mockNvsReset();
  mockNvsSetPartitionMissing(false);
  Party pt;
  pt.begin();
  CHECK(pt.boxReady);
  CHECK_EQ(pt.boxCount(), (uint8_t)0);
  CHECK(pt.boxAdd(mon(1, 5)));
  CHECK(mockNvsHasKey("box", "box", "tamabox"));
  CHECK_MSG(!mockNvsHasKey("tamapoke", "box"), "la caja ya no vive en el nvs de la partida");
  for (int i = 1; i < BOX_SLOTS; i++) CHECK(pt.boxAdd(mon(4, 5)));
  CHECK_EQ(pt.boxCount(), (uint8_t)BOX_SLOTS);
  CHECK(!pt.boxAdd(mon(7, 5)));        // llena

  Party again;                         // "reinicio"
  again.begin();
  CHECK_EQ(again.boxCount(), (uint8_t)BOX_SLOTS);
  CHECK_EQ(again.box[0].dex, (int16_t)1);
}

TEST(box, la_caja_vieja_de_18_se_muda_a_la_particion_nueva) {
  mockNvsReset();
  mockNvsSetPartitionMissing(false);
  PartyMon old[18];
  for (int i = 0; i < 18; i++) old[i] = mon((int16_t)(i + 1), 10);
  Preferences pr;
  pr.begin("tamapoke", false);
  pr.putBytes("box", old, sizeof(old));
  pr.end();

  Party pt;
  pt.begin();
  CHECK(pt.boxReady);
  CHECK_EQ(pt.boxCount(), (uint8_t)18);
  CHECK_EQ(pt.box[17].dex, (int16_t)18);
  CHECK(mockNvsHasKey("box", "box", "tamabox"));
  CHECK_MSG(!mockNvsHasKey("tamapoke", "box"), "movida: libera la clave del nvs de la partida");
}

TEST(box, una_particion_sin_formato_se_formatea_solo_la_caja) {
  mockNvsReset();
  mockNvsSetPartitionMissing(false);
  Preferences pr;
  pr.begin("tamapoke", false);
  pr.putUChar("full", 42);
  pr.end();
  int erased = mockNvsEraseCount();
  mockNvsSetInitResult(ESP_ERR_NVS_NO_FREE_PAGES, 1);
  Party pt;
  pt.begin();
  CHECK(pt.boxReady);
  CHECK_EQ(mockNvsEraseCount(), erased + 1);
  CHECK_MSG(mockNvsHasKey("tamapoke", "full"), "la partida no se toca");
}

TEST(box, sin_particion_tamabox_no_revienta) {
  mockNvsReset();
  mockNvsSetPartitionMissing(true);
  Party pt;
  pt.begin();
  CHECK(!pt.boxReady);
  CHECK_EQ(pt.boxCount(), (uint8_t)0);
  CHECK(pt.add(mon(1, 5)));            // el equipo sigue funcionando
  mockNvsSetPartitionMissing(false);
}

TEST(box, un_blob_con_dex_basura_se_limpia) {
  mockNvsReset();
  mockNvsSetPartitionMissing(false);
  PartyMon b[BOX_SLOTS];
  b[0] = mon(DEX_COUNT + 50, 5);
  b[1] = mon(-3, 5);
  b[2] = mon(4, 5);
  Preferences pr;
  pr.begin("box", false, "tamabox");
  pr.putBytes("box", b, sizeof(b));
  pr.end();
  Party pt;
  pt.begin();
  CHECK_EQ(pt.boxCount(), (uint8_t)1);
  CHECK_EQ(pt.box[2].dex, (int16_t)4);
}

TEST(box, el_equipo_y_la_caja_se_intercambian) {
  mockNvsReset();
  mockNvsSetPartitionMissing(false);
  Party pt;
  pt.begin();
  pt.add(mon(1, 5));
  pt.boxAdd(mon(4, 7));
  pt.swapPartyBox(0, 0);
  CHECK_EQ(pt.slots[0].dex, (int16_t)4);
  CHECK_EQ(pt.box[0].dex, (int16_t)1);
  Party again;
  again.begin();
  CHECK_EQ(again.slots[0].dex, (int16_t)4);
  CHECK_EQ(again.box[0].dex, (int16_t)1);
}

// ---------------------------------------------------------------- copia (save.cpp)
// busca una clave en un blob de EXPORT; devuelve la longitud del valor o -1
static int findKey(const std::vector<uint8_t> &b, const char *key, const uint8_t **val) {
  size_t at = SAVE_HDR;
  while (at + 4 <= b.size() - 2) {
    uint8_t klen = b[at];
    size_t vat = at + 1 + klen;
    uint16_t vlen = (uint16_t)b[vat + 1] | ((uint16_t)b[vat + 2] << 8);
    if (klen == strlen(key) && !memcmp(&b[at + 1], key, klen)) {
      if (val) *val = &b[vat + 3];
      return vlen;
    }
    at = vat + 3 + vlen;
  }
  return -1;
}

TEST(backup, el_export_incluye_la_caja_de_tamabox) {
  Pet p;
  hatchAs(p, 4);
  Party pt;
  pt.begin();
  pt.add(mon(1, 9));
  pt.boxAdd(mon(152, 8, "CHIKO"));

  std::vector<uint8_t> blob(saveExportSize());
  size_t n = saveExport(blob.data(), blob.size());
  CHECK(n > 0);
  blob.resize(n);
  const uint8_t *v = nullptr;
  CHECK_EQ(findKey(blob, "box", &v), (int)sizeof(pt.box));
  if (v) {
    PartyMon first;
    memcpy(&first, v, sizeof(first));
    CHECK_EQ(first.dex, (int16_t)152);
    CHECK_STREQ(first.nick, "CHIKO");
  }
  CHECK_EQ(findKey(blob, "party", nullptr), (int)sizeof(pt.slots));
  CHECK(findKey(blob, "egghld", nullptr) == 1);
  CHECK(findKey(blob, "dexn", nullptr) == 2);
}

TEST(backup, importar_restaura_la_caja_en_su_particion) {
  Pet p;
  hatchAs(p, 4);
  Party pt;
  pt.begin();
  pt.boxAdd(mon(252, 8));
  std::vector<uint8_t> blob(saveExportSize());
  size_t n = saveExport(blob.data(), blob.size());
  blob.resize(n);

  // otra partida encima: la caja tiene a otro
  pt.boxReleaseAt(0);
  pt.boxAdd(mon(7, 3));
  CHECK(saveImport(blob.data(), blob.size()));
  Party again;
  again.begin();
  CHECK_EQ(again.boxCount(), (uint8_t)1);
  CHECK_EQ(again.box[0].dex, (int16_t)252);
  Pet q;
  q.begin();
  CHECK_EQ(q.speciesId, (int16_t)4);
}

TEST(backup, un_blob_corrupto_no_toca_nada) {
  Pet p;
  hatchAs(p, 4);
  Party pt;
  pt.begin();
  pt.boxAdd(mon(252, 8));
  std::vector<uint8_t> blob(saveExportSize());
  size_t n = saveExport(blob.data(), blob.size());
  blob.resize(n);
  blob[SAVE_HDR + 3] ^= 0x55;          // rompe el CRC
  pt.boxReleaseAt(0);
  pt.boxAdd(mon(7, 3));
  CHECK(!saveImport(blob.data(), blob.size()));
  Party again;
  again.begin();
  CHECK_EQ(again.box[0].dex, (int16_t)7);
}

// ---------------------------------------------------------------- NVS: crecer/encoger
// loadBlob(): un blob MAS LARGO que el array (firmware nuevo -> viejo) conserva
// el prefijo en vez de perderlo entero (getBytes del hardware no copia nada)
TEST(save, un_dexreg_mas_largo_conserva_el_prefijo) {
  Pet a;
  hatchAs(a, 4);
  std::vector<uint8_t> big(sizeof(a.dexReg) + 16, 0);
  big[0] = 0x09;                        // dex 1 y 4
  Preferences pr;
  pr.begin("tamapoke", false);
  pr.putBytes("dexreg", big.data(), big.size());
  pr.end();
  Pet b;
  b.begin();
  CHECK(b.isRegistered(1));
  CHECK(b.isRegistered(4));
}
