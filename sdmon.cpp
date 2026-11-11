#include "sdmon.h"
#include "noart.h"
#include "pin_config.h"
#include "pet.h"   // gRegionArt, REGIONS -- the mask this narrows
#if TAMAPOKE_HAS_SD
#include <FS.h>
#include <SD_MMC.h>
#endif
#if TAMAPOKE_FLASH_SPRITES
#include <esp_partition.h>
#include "miniz.h"  // tinfl de la ROM del ESP32-S3 (esp_rom)
#endif

// "sdReady" = hay sprites disponibles. En la 1.75 vienen de la microSD; en la
// 1.75C (sin ranura) de la particion "sprites" de la flash, que el instalador
// graba aparte del programa (ver tools/pack_flash.py y partitions.csv).
bool sdReady = false;
bool sdDirty = false;
bool sdArtDirty = false;
SdThumbs thumbs;

#if TAMAPOKE_FLASH_SPRITES
// ---------------------------------------------------------------------------
// Paquete de sprites en flash (formato TPKZ, lo genera tools/pack_flash.py):
//   "TPKZ" u16 count u16 version, luego count entradas de 28 bytes
//   { char name[16]; u32 offset; u32 zlen; u32 rawLen } ordenadas por nombre,
//   y los datos: cada fichero de /mons comprimido con zlib (con cabecera).
// Los offsets cuentan desde el principio de la particion. El indice (~44 KB con
// las 7 regiones) se copia a PSRAM al arrancar; los datos se leen al cargar.
// ---------------------------------------------------------------------------
struct PackEntry {
  char name[16];    // "p001.bin", "ps025.bin", "thumbs.bin" (sin /mons/)
  uint32_t offset;
  uint32_t zlen;
  uint32_t rawLen;
};
static_assert(sizeof(PackEntry) == 28, "TPKZ: entradas de 28 bytes");

static const esp_partition_t *gPackPart = nullptr;
static PackEntry *gPack = nullptr;
static uint16_t gPackCount = 0;

static bool packOpen() {
  gPackPart = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "sprites");
  if (!gPackPart) {
    Serial.println("sin particion 'sprites' (tabla de particiones antigua?)");
    return false;
  }
  uint8_t hdr[8];
  if (esp_partition_read(gPackPart, 0, hdr, sizeof(hdr)) != ESP_OK || memcmp(hdr, "TPKZ", 4) != 0) {
    Serial.println("particion 'sprites' vacia: instala los sprites con el instalador web");
    return false;
  }
  uint16_t n;
  memcpy(&n, hdr + 4, 2);
  uint32_t idxBytes = (uint32_t)n * sizeof(PackEntry);
  if (n == 0 || sizeof(hdr) + idxBytes > gPackPart->size) return false;
  gPack = (PackEntry *)ps_malloc(idxBytes);
  if (!gPack || esp_partition_read(gPackPart, sizeof(hdr), gPack, idxBytes) != ESP_OK) {
    if (gPack) { free(gPack); gPack = nullptr; }
    return false;
  }
  gPackCount = n;
  return true;
}

// busqueda binaria: el empaquetador ordena por nombre (orden de bytes, como strncmp)
static const PackEntry *packFind(const char *path) {
  if (strncmp(path, "/mons/", 6) == 0) path += 6;
  int lo = 0, hi = (int)gPackCount - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    int c = strncmp(path, gPack[mid].name, sizeof(gPack[mid].name));
    if (c == 0) return &gPack[mid];
    if (c < 0) hi = mid - 1; else lo = mid + 1;
  }
  return nullptr;
}

// lee el stream comprimido a PSRAM y lo descomprime entero. El estado del
// descompresor (~11 KB) va al heap: en la pila del loop (8 KB) no cabe.
static uint8_t *packRead(const PackEntry *e, uint32_t minSize, uint32_t maxSize, uint32_t *outSize) {
  if (e->rawLen < minSize || e->rawLen > maxSize) return nullptr;
  if ((uint64_t)e->offset + e->zlen > gPackPart->size) return nullptr;
  uint8_t *z = (uint8_t *)ps_malloc(e->zlen);
  uint8_t *out = (uint8_t *)ps_malloc(e->rawLen);
  tinfl_decompressor *d = (tinfl_decompressor *)malloc(sizeof(tinfl_decompressor));
  bool ok = z && out && d && esp_partition_read(gPackPart, e->offset, z, e->zlen) == ESP_OK;
  if (ok) {
    tinfl_init(d);
    size_t inLen = e->zlen, outLen = e->rawLen;
    tinfl_status st = tinfl_decompress(d, z, &inLen, out, out, &outLen,
                                       TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    ok = (st == TINFL_STATUS_DONE && outLen == e->rawLen);
    if (!ok) Serial.printf("%.16s: fallo al descomprimir\n", e->name);
  }
  if (d) free(d);
  if (z) free(z);
  if (!ok) {
    if (out) free(out);
    return nullptr;
  }
  *outSize = e->rawLen;
  return out;
}
#endif  // TAMAPOKE_FLASH_SPRITES

// Lee un fichero de sprites entero a PSRAM (de la SD o de la flash). Devuelve
// nullptr si no existe, no cabe en [minSize, maxSize] o falla la lectura; el
// llamador libera con free().
static uint8_t *readSpriteFile(const char *path, uint32_t minSize, uint32_t maxSize, uint32_t *outSize) {
  if (!sdReady) return nullptr;
#if TAMAPOKE_FLASH_SPRITES
  const PackEntry *e = packFind(path);
  return e ? packRead(e, minSize, maxSize, outSize) : nullptr;
#else
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return nullptr;
  uint32_t size = f.size();
  if (size < minSize || size > maxSize) { f.close(); return nullptr; }
  uint8_t *buf = (uint8_t *)ps_malloc(size);
  if (!buf || f.read(buf, size) != size) {
    if (buf) free(buf);
    f.close();
    return nullptr;
  }
  f.close();
  *outSize = size;
  return buf;
#endif
}

// existe el fichero? (sin leerlo: para el sondeo de regiones)
static bool spriteFileExists(const char *path) {
  if (!sdReady) return false;
#if TAMAPOKE_FLASH_SPRITES
  return packFind(path) != nullptr;
#else
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  f.close();
  return true;
#endif
}

bool PmdMon::load(int16_t dexNum, bool shiny) {
  // int16_t, NOT uint8_t. The dex reached 386 and this did not follow, so
  // everything from 256 up wrapped into Kanto: MARSHTOMP (258) opened
  // p002.bin and drew an IVYSAUR. Same trap that caught DexEntry::evolvesTo
  // and TrainerMon::dex when the expansion landed.
  if (dexNum < 1 || dexNum > 999) return false;
  unload();
  if (!sdReady) return false;

  char path[28];
  uint32_t size = 0;
  snprintf(path, sizeof(path), "/mons/p%s%03u.bin", shiny ? "s" : "", (unsigned)dexNum);
  blob = readSpriteFile(path, 7, 3UL * 1024 * 1024, &size);
  if (!blob && shiny) {  // sin shiny PMD: usa el normal
    snprintf(path, sizeof(path), "/mons/p%03u.bin", (unsigned)dexNum);
    blob = readSpriteFile(path, 7, 3UL * 1024 * 1024, &size);
  }
  if (!blob) return false;
  if (memcmp(blob, "TPK2", 4) != 0) {
    free(blob);
    blob = nullptr;
    return false;
  }

  dex = dexNum;                  // what is actually in here, for the tests
  uint8_t nActs = blob[4];
  memcpy(&palCount, blob + 5, 2);
  if (palCount > 256 || (uint32_t)7 + palCount * 2 > size) { unload(); return false; }
  memcpy(pal, blob + 7, palCount * 2);

  const uint8_t *p = blob + 7 + palCount * 2;
  const uint8_t *end = blob + size;
  for (uint8_t i = 0; i < nActs && p + 4 <= end; i++) {
    uint8_t id = p[0], w = p[1], h = p[2], nf = p[3];
    p += 4;
    if (id >= PMD_NACTS || nf > 24) { unload(); return false; }
    // valida que ms[] y los datos del frame caben en el blob (archivo truncado)
    uint32_t bytes = (uint32_t)nf * 2 + (uint32_t)w * h * nf;
    if (w == 0 || h == 0 || nf == 0 || p + bytes > end) { unload(); return false; }
    PmdAct &a = acts[id];
    a.w = w;
    a.h = h;
    a.frames = nf;
    for (uint8_t k = 0; k < nf; k++) {
      a.ms[k] = p[0] | (p[1] << 8);
      if (a.ms[k] == 0) a.ms[k] = 100;  // nunca 0: pmdFrameAt() giraria sin avanzar
      p += 2;
    }
    a.data = p;
    p += (uint32_t)w * h * nf;
    // fila mas baja con contenido en cualquier frame: anclar por los pies
    uint8_t base = 1;
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = h - 1; r >= 0; r--) {
        bool any = false;
        for (int c = 0; c < w && !any; c++)
          if (fr[r * w + c] != 0xFF) any = true;
        if (any) { if (r + 1 > base) base = r + 1; break; }
      }
    }
    a.base = base;
  }
  loaded = true;
  Serial.printf("cargado %s (%u KB)\n", path, (unsigned)(size / 1024));
  return true;
}

void PmdMon::unload() {
  if (blob) {
    free(blob);
    blob = nullptr;
  }
  for (auto &a : acts) {
    a.w = a.h = a.frames = a.base = 0;
    a.data = nullptr;
  }
  loaded = false;
}

void SdThumbs::unload() {
  if (data) { free(data); data = nullptr; }
  loaded = false;
  count = 0;
  size = 0;
}

bool SdThumbs::load() {
  unload();  // recargar sin fugar el blob anterior
  uint32_t sz = 0;
  // acota el tamano: evita un ps_malloc absurdo con un archivo corrupto
  data = readSpriteFile("/mons/thumbs.bin", 10, 4UL * 1024 * 1024, &sz);
  if (!data) {
    Serial.println("sin thumbs.bin valido (galeria sin miniaturas)");
    return false;
  }
  if (memcmp(data, "TPTH", 4) != 0) {
    Serial.println("thumbs.bin invalido");
    free(data);
    data = nullptr;
    return false;
  }
  memcpy(&count, data + 4, 2);
  // la tabla de offsets (uno por especie) debe caber entera en lo leido
  if ((uint32_t)6 + 4UL * count > sz) {
    Serial.println("thumbs.bin invalido (tabla de offsets)");
    unload();
    return false;
  }
  size = sz;
  loaded = true;
  Serial.printf("miniaturas cargadas: %u (%u KB)\n", count, (unsigned)(sz / 1024));
  return true;
}

const uint8_t *SdThumbs::get(int16_t dex) const {
  if (!loaded || dex < 1 || dex > count) return nullptr;
  uint32_t off;
  memcpy(&off, data + 6 + 4 * (dex - 1), 4);
  // el offset viene del fichero sin comprobar: sin esto, un thumbs.bin truncado
  // o corrupto hacia leer fuera de la reserva de PSRAM
  if (off > size - 3) return nullptr;  // la cabecera w,h,palCount debe caber (size >= 10 siempre)
  uint32_t need = 3 + (uint32_t)data[off + 2] * 2 + (uint32_t)data[off] * data[off + 1];
  if (need > size || off > size - need) return nullptr;  // y el blob entero tambien
  return data + off;
}

// Which regions actually have their sprite pack on the card (or in flash).
//
// Three probes per region, not one: the realistic failure is a half-finished
// copy of a 100 MB pack, and a single probe would call that region present.
// Start, middle and end, so a copy that stopped partway fails the check.
//
// This NARROWS gRegionArt, which starts as everything. A board with no sprites
// at all is therefore untouched and keeps today's behaviour, which is the
// documented requirement -- an empty game would be a worse answer than a
// graceful one.
void sdScanRegionArt(bool verbose) {
  if (!sdReady) return;                 // no sprites: leave every region enabled
  uint16_t mask = 0;
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    if (r == REGION_ALL) continue;      // derived from the others, never probed
    const RegionInfo &rg = REGIONS[r];
    const int16_t probe[3] = { sdRegionProbe(r, 0), sdRegionProbe(r, 1),
                               sdRegionProbe(r, 2) };
    bool all = true;
    for (int i = 0; i < 3 && all; i++) {
      char path[28];
      snprintf(path, sizeof(path), "/mons/p%03u.bin", (unsigned)probe[i]);
      if (!spriteFileExists(path)) all = false;
    }
    if (all) mask |= (uint16_t)(1u << r);
    if (verbose)
      Serial.printf("art: %-6s %s\n", rg.name, all ? "si" : "NO (falta el pack)");
  }
  gRegionArt = mask;
}

#if TAMAPOKE_FLASH_SPRITES
bool sdBegin() {
  sdReady = packOpen();
  if (sdReady) {
    Serial.printf("placa sin SD: %u ficheros de sprites en la particion 'sprites'\n", gPackCount);
    sdScanRegionArt();
  }
  return sdReady;
}

// sin SD no hay donde guardar ficheros: PUT se rechaza y LS lista lo que hay en
// la flash (los clientes, tools/send_sd.py y web/index.html, paran al ver ERR)
bool sdSerialCommand(const String &line) {
  if (line.startsWith("PUT ")) {
    Serial.println("ERR");
    return true;
  } else if (line == "SDINFO") {
    Serial.printf("sin SD: %u sprites en flash\n", gPackCount);
    Serial.println("DONE");
    return true;
  } else if (line == "LS") {
    for (uint16_t i = 0; i < gPackCount; i++)
      Serial.printf("%.16s %u\n", gPack[i].name, (unsigned)gPack[i].rawLen);
    Serial.println("DONE");
    return true;
  }
  return false;
}

// el formato TPK1 heredado solo existe en la SD
bool SdMon::load(int16_t, bool) {
  unload();
  return false;
}

#else
bool sdBegin() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  sdReady = SD_MMC.begin("/sdcard", true /* modo 1-bit */, true /* formatea si no monta */);
  if (sdReady) {
    Serial.printf("SD montada: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
    SD_MMC.mkdir("/mons");
    sdScanRegionArt();
  } else {
    Serial.println("SD no detectada (el juego usa los sprites de flash)");
  }
  return sdReady;
}

bool SdMon::load(int16_t dexNum, bool shiny) {
  if (dexNum < 1 || dexNum > 999) return false;
  unload();
  if (!sdReady) return false;

  char path[24];
  snprintf(path, sizeof(path), "/mons/%s%03u.bin", shiny ? "s" : "", (unsigned)dexNum);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f && shiny) {  // sin variante shiny: usa la normal
    snprintf(path, sizeof(path), "/mons/%03u.bin", (unsigned)dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) {
    Serial.printf("no existe %s\n", path);
    return false;
  }

  char magic[4];
  uint16_t header[4];
  if (f.read((uint8_t *)magic, 4) != 4 || memcmp(magic, "TPK1", 4) != 0 ||
      f.read((uint8_t *)header, 8) != 8) {
    f.close();
    return false;
  }
  w = header[0];
  h = header[1];
  frames = header[2];
  frameMs = header[3];
  // acota dimensiones: evita size desbordado o absurdo con archivo corrupto
  if (f.read((uint8_t *)&palCount, 2) != 2 || palCount > 256 ||
      w == 0 || w > 256 || h == 0 || h > 256 || frames == 0 || frames > 64) {
    f.close();
    return false;
  }
  if (f.read((uint8_t *)pal, palCount * 2) != palCount * 2) {
    f.close();
    return false;
  }

  uint32_t size = (uint32_t)w * h * frames;
  data = (uint8_t *)ps_malloc(size);
  if (!data) {
    Serial.println("sin PSRAM para el sprite");
    f.close();
    return false;
  }
  uint32_t got = f.read(data, size);
  f.close();
  if (got != size) {
    Serial.printf("%s truncado (%u de %u)\n", path, (unsigned)got, (unsigned)size);
    unload();
    return false;
  }

  // zoom entero para que el bicho mida ~200 px de alto en pantalla
  scale = 200 / h;
  if (scale < 2) scale = 2;
  if (scale > 5) scale = 5;

  Serial.printf("cargado %s: %ux%u x%u frames @%ums, escala %u\n",
                path, w, h, frames, frameMs, scale);
  loaded = true;
  return true;
}

#endif  // TAMAPOKE_FLASH_SPRITES (sdBegin, SdMon::load)

void SdMon::unload() {
  if (data) {
    free(data);
    data = nullptr;
  }
  loaded = false;
}

#if TAMAPOKE_HAS_SD

// ---------------------------------------------------------------------------
// Protocolo de carga por USB (para llenar la SD sin sacarla de la placa):
//   PUT <ruta> <bytes>\n  + datos crudos   -> "OK" ... "DONE"
//   LS\n                                   -> listado de /mons
// Usar con tools/send_sd.py
// ---------------------------------------------------------------------------

bool sdSerialCommand(const String &line) {
  if (line.startsWith("PUT ")) {
    int sp = line.lastIndexOf(' ');
    String path = line.substring(4, sp);
    uint32_t size = line.substring(sp + 1).toInt();
    if (!sdReady || size == 0 || size > 4 * 1024 * 1024) {
      Serial.println("ERR");
      return true;
    }
    if (!path.startsWith("/")) path = "/" + path;
    // acota la escritura a /mons/: la ruta llega tal cual de la linea serie, sin
    // sanear, asi que un PUT manipulado podria escribir en cualquier sitio de la
    // tarjeta. Los dos clientes reales (tools/send_sd.py y web/index.html) ya
    // mandan nombres con el prefijo mons/.
    if (!path.startsWith("/mons/") || path.indexOf("..") >= 0) {
      Serial.println("ERR");
      return true;
    }
    // FILE_WRITE ANADE al final si el fichero ya existe, asi que reintentar uno
    // que quedo a medias lo alargaba en vez de reemplazarlo: quedaba un sprite
    // corrupto y mas grande que el original. Importa mas desde que el instalador
    // reanuda transferencias cortadas, porque reintenta justo los que fallaron.
    if (SD_MMC.exists(path)) SD_MMC.remove(path);
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
      Serial.println("ERR");
      return true;
    }
    Serial.println("OK");
    static uint8_t buf[2048];
    uint32_t remaining = size;
    Serial.setTimeout(5000);
    while (remaining > 0) {
      size_t want = remaining > sizeof(buf) ? sizeof(buf) : remaining;
      size_t n = Serial.readBytes(buf, want);
      if (n == 0) break;  // timeout
      // sin mirar el retorno, una tarjeta llena o con fallo de escritura daba
      // DONE igualmente y dejaba el fichero truncado en la SD: justo la entrada
      // corrupta contra la que hay que protegerse luego al cargar el sprite
      if (f.write(buf, n) != n) break;
      remaining -= n;
      Serial.println("#");  // ack: listo para el siguiente bloque
    }
    f.close();
    Serial.setTimeout(1000);
    sdDirty = (remaining == 0);
    // A pack file just landed, so which regions are playable may have changed.
    // Flag it rather than rescanning here: this runs between the last data block
    // and the DONE the host is waiting on, and 15 file opens belong nowhere near
    // that. loop() picks it up.
    if (remaining == 0) sdArtDirty = true;
    Serial.println(remaining == 0 ? "DONE" : "ERR");
    return true;
  } else if (line == "SDINFO") {  // diagnostico remoto de "no me reconoce la SD"
    Serial.printf("total=%llu used=%llu\n", SD_MMC.totalBytes(), SD_MMC.usedBytes());
    Serial.println("DONE");
    return true;
  } else if (line == "LS") {
    File dir = SD_MMC.open("/mons");
    if (dir) {
      File e;
      while ((e = dir.openNextFile())) {
        Serial.printf("%s %u\n", e.name(), (uint32_t)e.size());
        e.close();
      }
      dir.close();
    }
    Serial.println("DONE");
    return true;
  }
  return false;
}
#endif  // TAMAPOKE_HAS_SD (protocolo PUT/LS)
