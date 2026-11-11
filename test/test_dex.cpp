// Integridad de la tabla de la Pokedex (dex.h, generada por tools/gen_dex.py).
#include "framework.h"
#include "shim/Arduino.h"
#include "../dex.h"
#include "../i18n.h"
#include "../pet.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <set>

static const int N = DEX_COUNT;

TEST(dex, la_tabla_tiene_DEX_COUNT_mas_1_entradas) {
  CHECK_EQ((int)(sizeof(DEX_TBL) / sizeof(DEX_TBL[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_FR) / sizeof(DEX_NAME_FR[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_DE) / sizeof(DEX_NAME_DE[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_JA) / sizeof(DEX_NAME_JA[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_KO) / sizeof(DEX_NAME_KO[0])), N + 1);
}

TEST(dex, todos_los_nombres_son_ascii_en_mayusculas) {
  for (int d = 1; d <= N; d++) {
    const char *n = DEX_TBL[d].name;
    CHECK_MSG(n != NULL && n[0] != 0, "entrada sin nombre");
    if (!n || !n[0]) continue;
    for (const char *c = n; *c; c++) {
      // la fuente GFX del firmware es ASCII y sin minusculas
      CHECK_MSG((unsigned char)*c < 128, std::string("caracter no ASCII en ") + n);
      CHECK_MSG(!islower((unsigned char)*c), std::string("minuscula en ") + n);
    }
  }
}

TEST(dex, los_nombres_traducidos_tambien_son_ascii) {
  const char *const *tables[2] = { DEX_NAME_FR, DEX_NAME_DE };
  for (int t = 0; t < 2; t++) {
    for (int d = 1; d <= N; d++) {
      const char *n = tables[t][d];
      if (!n) continue;  // sin nombre propio: cae al de DEX_TBL
      CHECK_MSG(n[0] != 0, "nombre traducido vacio");
      for (const char *c = n; *c; c++) {
        CHECK_MSG((unsigned char)*c < 128, std::string("caracter no ASCII en ") + n);
        CHECK_MSG(!islower((unsigned char)*c), std::string("minuscula en ") + n);
      }
    }
  }
}

// la cabecera de la ficha es char[26] con "%s%s Lv.%u": ningun nombre en ningun
// idioma puede desbordarla (el nombre se cortaria en pantalla)
TEST(dex, ningun_nombre_desborda_la_cabecera) {
  char head[26];
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int d = 1; d <= N; d++) {
      int need = snprintf(head, sizeof(head), T(S_NAME_FMT), "*", dexName(d), 255);
      CHECK_MSG(need >= 0 && need < (int)sizeof(head),
                std::string("cabecera cortada: ") + dexName(d));
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(dex, las_evoluciones_apuntan_a_entradas_validas) {
  for (int d = 1; d <= N; d++) {
    // uint16_t: con gen 2+ evolvesTo pasa de 255 (en uint8_t se truncaba)
    uint16_t to = DEX_TBL[d].evolvesTo;
    CHECK_MSG(to <= N, std::string("evolvesTo fuera de rango en ") + DEX_TBL[d].name);
    CHECK_MSG(to != d, std::string("se evoluciona a si mismo: ") + DEX_TBL[d].name);
  }
}

TEST(dex, evolucionar_y_nivel_van_de_la_mano) {
  for (int d = 1; d <= N; d++) {
    bool evolves = DEX_TBL[d].evolvesTo != 0;
    bool hasLevel = DEX_TBL[d].evolveLevel > 0;
    CHECK_MSG(evolves == hasLevel,
              std::string("evolvesTo y evolveLevel no cuadran en ") + DEX_TBL[d].name);
  }
}

TEST(dex, ninguna_cadena_evolutiva_hace_bucle) {
  for (int d = 1; d <= N; d++) {
    std::set<int> seen;
    int cur = d;
    int guard = 0;
    while (cur >= 1 && cur <= N && DEX_TBL[cur].evolvesTo != 0 && guard++ < N) {
      CHECK_MSG(seen.insert(cur).second, std::string("bucle evolutivo en ") + DEX_TBL[d].name);
      if (!seen.count(cur)) break;
      cur = DEX_TBL[cur].evolvesTo;
    }
    CHECK_MSG(guard < 6, std::string("cadena demasiado larga desde ") + DEX_TBL[d].name);
  }
}

// v3: las evoluciones por objeto/intercambio/amistad de gen 2+ llevan un nivel
// fijo (30/40) en tools/dex_data.py, que puede quedar por debajo del paso
// anterior de la cadena (p.ej. AZURILL 25 -> MARILL 18). Son esas y solo esas;
// cualquier otra cadena con niveles que no suben es un error de datos.
static bool itemEvoException(int d) {
  static const int16_t EXC[] = {
    82,   // MAGNETON -> MAGNEZONE (lugar especial)
    112,  // RHYDON -> RHYPERIOR (intercambio con protector)
    233,  // PORYGON2 -> PORYGON-Z (intercambio con disco extrano)
    221,  // PILOSWINE -> MAMOSWINE (aprende poder pasado)
    183,  // MARILL (tras AZURILL, bebe de gen 3)
    113,  // CHANSEY (tras HAPPINY, bebe de gen 4)
    603,  // EELEKTRIK -> EELEKTROSS (piedra trueno)
    608,  // LAMPENT -> CHANDELURE (piedra noche)
    680,  // DOUBLADE -> AEGISLASH (piedra noche)
  };
  for (size_t i = 0; i < sizeof(EXC) / sizeof(EXC[0]); i++)
    if (EXC[i] == d) return true;
  return false;
}

TEST(dex, los_niveles_de_evolucion_suben_a_lo_largo_de_la_cadena) {
  for (int d = 1; d <= N; d++) {
    int cur = d, prev = 0, guard = 0;
    while (DEX_TBL[cur].evolvesTo != 0 && guard++ < 6) {
      CHECK_MSG(DEX_TBL[cur].evolveLevel > prev || itemEvoException(cur),
                std::string("nivel de evolucion no creciente en ") + DEX_TBL[cur].name);
      prev = DEX_TBL[cur].evolveLevel;
      cur = DEX_TBL[cur].evolvesTo;
    }
  }
}

// una especie que solo sale por evolucion tiene que ser alcanzable desde algun
// huevo, o seria imposible de registrar en la pokedex
TEST(dex, toda_especie_es_alcanzable) {
  bool reach[DEX_COUNT + 1] = { false };
  for (int d = 1; d <= N; d++) {
    if (DEX_TBL[d].rarity == R_EVO) continue;  // no sale de huevo
    int cur = d, guard = 0;
    while (cur >= 1 && cur <= N && guard++ < 6) {
      reach[cur] = true;
      if (cur == DEX_EEVEE) {  // rama especial del codigo del juego
        for (int i = 0; i < EEVEE_EVO_COUNT; i++) reach[EEVEE_EVOS[i]] = true;
        break;
      }
      if (DEX_TBL[cur].evolvesTo == 0) break;
      cur = DEX_TBL[cur].evolvesTo;
    }
  }
  for (int d = 1; d <= N; d++)
    CHECK_MSG(reach[d], std::string("inalcanzable: ") + DEX_TBL[d].name);
}

// con la despedida a los 3 dias (nivel 72), toda linea tiene que poder llegar a
// su forma final antes de que la mascota cumpla el ciclo
TEST(dex, toda_linea_se_completa_antes_de_la_despedida) {
  const uint32_t maxLevel = FAREWELL_AGE_MIN / MINUTES_PER_LEVEL;
  for (int d = 1; d <= N; d++) {
    if (DEX_TBL[d].rarity == R_EVO) continue;
    int cur = d, guard = 0;
    uint32_t need = 0;
    while (DEX_TBL[cur].evolvesTo != 0 && guard++ < 6) {
      if (DEX_TBL[cur].evolveLevel > need) need = DEX_TBL[cur].evolveLevel;
      cur = DEX_TBL[cur].evolvesTo;
    }
    CHECK_MSG(need <= maxLevel,
              std::string("la linea de ") + DEX_TBL[d].name + " no llega a forma final a tiempo");
  }
}

TEST(dex, las_stats_base_son_plausibles) {
  for (int d = 1; d <= N; d++) {
    const DexEntry &e = DEX_TBL[d];
    CHECK_MSG(e.bHp > 0 && e.bAtk > 0 && e.bDef > 0 && e.bSpe > 0,
              std::string("stat base a cero en ") + e.name);
    CHECK_MSG(e.biome <= 5, std::string("bioma desconocido en ") + e.name);
    CHECK_MSG(e.rarity <= R_LEGENDARIO, std::string("rareza desconocida en ") + e.name);
  }
}

TEST(dex, la_rama_de_eevee_cuadra_con_el_codigo) {
  CHECK_EQ((int)DEX_TBL[DEX_EEVEE].rarity, (int)R_COMUN);
  bool tableTargetIsBranch = false;
  for (int i = 0; i < EEVEE_EVO_COUNT; i++) {
    int d = EEVEE_EVOS[i];
    CHECK_RANGE(d, 1, N);
    if (d < 1 || d > N) continue;
    CHECK_EQ((int)DEX_TBL[d].rarity, (int)R_EVO);
    CHECK_EQ((int)DEX_TBL[d].evolvesTo, 0);
    if (d == DEX_TBL[DEX_EEVEE].evolvesTo) tableTargetIsBranch = true;
  }
  CHECK_MSG(tableTargetIsBranch, "el evolvesTo de la tabla es una de las ramas");
}

// v3: iniciales por region. PIKACHU es inicial (Amarillo) aunque con PICHU en la
// Pokedex sea R_EVO: es la unica forma evolucionada permitida como inicial.
static void checkStarter(int16_t d, int lo, int hi) {
  CHECK_RANGE(d, (int16_t)lo, (int16_t)hi);
  if (d < 1 || d > N) return;
  CHECK_MSG(DEX_TBL[d].rarity != R_EVO || d == 25,
            std::string("un inicial no puede ser una forma evolucionada: ") + DEX_TBL[d].name);
}

TEST(dex, los_iniciales_clasicos_son_validos) {
  CHECK_EQ((int)(sizeof(CLASSIC_DEX) / sizeof(CLASSIC_DEX[0])), NUM_CLASSIC_DEX);
  for (int i = 0; i < NUM_CLASSIC_DEX; i++) checkStarter(CLASSIC_DEX[i], 1, 151);
  for (int r = 0; r < REGION_COUNT; r++) {
    CHECK_MSG(REGIONS[r].starterCount > 0, std::string("region sin iniciales: ") + REGIONS[r].name);
    for (int i = 0; i < REGIONS[r].starterCount; i++)
      checkStarter(REGIONS[r].starters[i], REGIONS[r].lo, REGIONS[r].hi);
  }
}

// las regiones cubren la Pokedex una sola vez (REGION_ALL es la union)
TEST(dex, las_regiones_cubren_la_pokedex_sin_solaparse) {
  int owner[DEX_COUNT + 1] = { 0 };
  for (int r = 0; r < REGION_COUNT; r++) {
    if (r == REGION_ALL) continue;
    for (int d = REGIONS[r].lo; d <= REGIONS[r].hi && d <= N; d++) owner[d]++;
  }
  for (int d = 1; d <= N; d++)
    CHECK_MSG(owner[d] == 1, std::string("region mal cubierta en ") + DEX_TBL[d].name);
  CHECK_EQ((int)REGIONS[REGION_ALL].lo, 1);
  CHECK_EQ((int)REGIONS[REGION_ALL].hi, N);
}

TEST(dex, hay_al_menos_una_especie_de_cada_rareza) {
  int count[4] = { 0, 0, 0, 0 };
  for (int d = 1; d <= N; d++) count[DEX_TBL[d].rarity]++;
  for (int r = 0; r <= R_LEGENDARIO; r++)
    CHECK_MSG(count[r] > 0, "hay una rareza sin especies: el sorteo del huevo se quedaria sin candidatos");
}

// pickEggSpecies mete los candidatos de un tier de UNA region en un array de
// CAND_MAX (pet.cpp, hoy 260): si algun tier tuviera mas, se sortearia solo
// entre los primeros y el final de la Pokedex no saldria nunca de un huevo.
// REGION_ALL es el caso grande. Si cambia CAND_MAX en pet.cpp, cambialo aqui.
#define TEST_CAND_MAX 260
TEST(dex, ningun_tier_desborda_el_array_de_candidatos) {
  for (int rg = 0; rg < REGION_COUNT; rg++) {
    for (int r = R_COMUN; r <= R_LEGENDARIO; r++) {
      int n = 0;
      for (int d = REGIONS[rg].lo; d <= REGIONS[rg].hi && d <= N; d++)
        if (DEX_TBL[d].rarity == r) n++;
      char msg[96];
      snprintf(msg, sizeof(msg), "%s tier %d: %d candidatos > %d, pickEggSpecies los recorta",
               REGIONS[rg].name, r, n, TEST_CAND_MAX);
      CHECK_MSG(n <= TEST_CAND_MAX, msg);
    }
  }
}

TEST(dex, dexName_cae_al_nombre_base_si_no_hay_traduccion) {
  gLang = LANG_ES;
  CHECK_STREQ(dexName(1), DEX_TBL[1].name);
  gLang = LANG_FR;
  for (int d = 1; d <= N; d++) {
    const char *want = DEX_NAME_FR[d] ? DEX_NAME_FR[d] : DEX_TBL[d].name;
    CHECK_STREQ(dexName(d), want);
  }
  gLang = LANG_DE;
  for (int d = 1; d <= N; d++) {
    const char *want = DEX_NAME_DE[d] ? DEX_NAME_DE[d] : DEX_TBL[d].name;
    CHECK_STREQ(dexName(d), want);
  }
  gLang = LANG_DEFAULT;
}

TEST(dex, dexName_fuera_de_rango_no_revienta) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    CHECK_STREQ(dexName(0), DEX_TBL[0].name);
    CHECK_STREQ(dexName(-1), DEX_TBL[0].name);
    CHECK_STREQ(dexName(DEX_COUNT + 1), DEX_TBL[0].name);
    CHECK_STREQ(dexName(30000), DEX_TBL[0].name);
  }
  gLang = LANG_DEFAULT;
}
