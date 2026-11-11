#!/usr/bin/env python3
"""Tests de las herramientas del taller (tools/) y de los ficheros generados.

  python3 test/test_tools.py
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, 'tools')
sys.path.insert(0, TOOLS)


class TestToolsCompile(unittest.TestCase):
    def test_todos_los_scripts_compilan(self):
        scripts = sorted(f for f in os.listdir(TOOLS) if f.endswith('.py'))
        self.assertTrue(scripts, 'no se encontro ningun script en tools/')
        for name in scripts:
            with self.subTest(script=name):
                with open(os.path.join(TOOLS, name), encoding='utf-8') as fh:
                    src = fh.read()
                try:
                    compile(src, name, 'exec')
                except SyntaxError as e:
                    self.fail(f'{name} no compila: {e}')

    def test_los_scripts_ejecutables_declaran_shebang(self):
        for name in sorted(f for f in os.listdir(TOOLS) if f.endswith('.py')):
            path = os.path.join(TOOLS, name)
            if not os.access(path, os.X_OK):
                continue
            with open(path) as fh:
                self.assertTrue(fh.readline().startswith('#!'),
                                f'{name} es ejecutable pero no tiene shebang')


class TestDexData(unittest.TestCase):
    """Invariantes de la fuente de datos de la Pokedex."""

    @classmethod
    def setUpClass(cls):
        from dex_data import (DEX, TYPE_ACCENTS, CLASSIC, RARE, LEGENDARY, SLUGS,
                              REGIONS, EEVEE_BRANCHES)
        from dex_stats import BASE_STATS
        from dex_names import LOCAL_NAMES
        cls.DEX, cls.ACCENTS, cls.CLASSIC = DEX, TYPE_ACCENTS, CLASSIC
        cls.RARE, cls.LEGENDARY, cls.SLUGS = RARE, LEGENDARY, SLUGS
        cls.REGIONS, cls.EEVEE = REGIONS, EEVEE_BRANCHES
        cls.STATS, cls.NAMES = BASE_STATS, LOCAL_NAMES
        cls.byNum = {row[0]: row for row in DEX}
        # lo que solo sale por evolucion: destino de alguna evolucion o rama de
        # Eevee (gen_dex.py lo marca R_EVO por encima de RARE/LEGENDARY)
        cls.evolucionadas = {row[4] for row in DEX if row[4]} | set(EEVEE_BRANCHES)

    # v3: 809 especies en 7 regiones (antes 151)
    def test_estan_todas_las_especies_sin_huecos_ni_repetidos(self):
        nums = [row[0] for row in self.DEX]
        total = max(hi for _, _, hi, _ in self.REGIONS)
        self.assertEqual(len(nums), total)
        self.assertEqual(sorted(nums), list(range(1, total + 1)))

    def test_las_regiones_cubren_la_pokedex_una_vez(self):
        visto = []
        for name, lo, hi, starters in self.REGIONS:
            visto += list(range(lo, hi + 1))
            for s in starters:
                self.assertTrue(lo <= s <= hi, f'{name}: inicial {s} fuera de su region')
        self.assertEqual(sorted(visto), sorted(self.byNum), 'regiones con huecos o solapes')

    def test_los_slugs_son_unicos_y_en_minusculas(self):
        slugs = [row[1] for row in self.DEX]
        self.assertEqual(len(set(slugs)), len(slugs), 'hay slugs repetidos')
        for s in slugs:
            self.assertEqual(s, s.lower(), f'slug con mayusculas: {s}')
            self.assertTrue(s.isascii(), f'slug no ASCII: {s}')

    def test_los_nombres_de_pantalla_son_ascii_en_mayusculas(self):
        # la fuente GFX del firmware no tiene acentos ni minusculas
        for num, slug, display, *_ in self.DEX:
            self.assertTrue(display.isascii(), f'{num} {display} no es ASCII')
            self.assertEqual(display, display.upper(), f'{num} {display} lleva minusculas')
            self.assertLessEqual(len(display), 12, f'{num} {display} no cabe en pantalla')

    def test_las_evoluciones_apuntan_a_especies_reales(self):
        for num, slug, display, tipo, evo, lvl in self.DEX:
            if evo:
                self.assertIn(evo, self.byNum, f'{display} evoluciona a {evo}, que no existe')
                self.assertNotEqual(evo, num, f'{display} evoluciona a si mismo')
                self.assertGreater(lvl, 0, f'{display} evoluciona sin nivel')
            else:
                self.assertEqual(lvl, 0, f'{display} es forma final pero tiene nivel {lvl}')

    def test_ninguna_cadena_evolutiva_hace_bucle(self):
        for num in self.byNum:
            seen, cur = set(), num
            while cur and self.byNum[cur][4]:
                self.assertNotIn(cur, seen, f'bucle evolutivo desde {num}')
                seen.add(cur)
                cur = self.byNum[cur][4]
                self.assertLessEqual(len(seen), 5, f'cadena demasiado larga desde {num}')

    def test_todos_los_tipos_tienen_color(self):
        for num, slug, display, tipo, *_ in self.DEX:
            self.assertIn(tipo, self.ACCENTS, f'{display}: tipo "{tipo}" sin color')

    def test_los_colores_son_hex_de_6_digitos(self):
        for tipo, col in self.ACCENTS.items():
            self.assertRegex(col, r'^#[0-9a-fA-F]{6}$', f'color raro en {tipo}: {col}')

    # v3: al anadir bebes de gen 2+ (TYROGUE, MUNCHLAX, HAPPINY...) varias
    # especies de RARE/LEGENDARY pasaron a ser destino de evolucion. gen_dex.py
    # da prioridad a R_EVO, asi que lo que importa es que en dex.h NINGUNA forma
    # evolucionada pueda salir de un huevo, y que lo raro/legendario que si es
    # forma base lo sea de verdad.
    def test_las_rarezas_apuntan_a_formas_base(self):
        with open(os.path.join(ROOT, 'dex.h')) as fh:
            filas = re.findall(r'^\s*\{ "[^"]*", \d+, \d+, (R_\w+),.*// (\d+) ', fh.read(), re.M)
        rareza = {int(n): r for r, n in filas}
        self.assertEqual(set(rareza), set(self.byNum), 'dex.h no tiene una fila por especie')
        for num in sorted(self.RARE | self.LEGENDARY):
            self.assertIn(num, self.byNum, f'rareza para una especie inexistente: {num}')
        for num in self.evolucionadas:
            self.assertEqual(rareza[num], 'R_EVO',
                             f'{self.byNum[num][2]} solo sale por evolucion y saldria de un huevo')
        for num in sorted((self.LEGENDARY - self.evolucionadas)):
            self.assertEqual(rareza[num], 'R_LEGENDARIO', f'{self.byNum[num][2]}')
        for num in sorted((self.RARE - self.evolucionadas - self.LEGENDARY)):
            self.assertEqual(rareza[num], 'R_RARO', f'{self.byNum[num][2]}')
        self.assertFalse(self.RARE & self.LEGENDARY, 'una especie no puede ser rara y legendaria')

    # v3: iniciales por region. PIKACHU (Amarillo) es la unica forma
    # evolucionada permitida: con PICHU en la Pokedex es destino de evolucion.
    def test_los_iniciales_clasicos_existen(self):
        iniciales = set(self.CLASSIC)
        for _, _, _, starters in self.REGIONS:
            iniciales |= set(starters)
        for num in sorted(iniciales):
            self.assertIn(num, self.byNum, f'inicial inexistente: {num}')
            if num != 25:
                self.assertNotIn(num, self.evolucionadas,
                                 f'{self.byNum[num][2]} no puede ser inicial')

    # v3: (hp, atk, def, vel, spa, spd) -- 6 valores, antes 4
    def test_hay_stats_base_de_todas(self):
        for num in self.byNum:
            self.assertIn(num, self.STATS, f'faltan stats de {self.byNum[num][2]}')
            self.assertEqual(len(self.STATS[num]), 6, f'{self.byNum[num][2]}: se esperan 6 stats')
            for v in self.STATS[num]:
                self.assertTrue(0 < v <= 255, f'stat fuera de rango en {self.byNum[num][2]}: {v}')

    def test_los_nombres_traducidos_cuadran_con_su_fuente(self):
        # Los latinos se pintan con la fuente CP437 (un byte por caracter, sin
        # minusculas en los nombres); el japones y el coreano con una fuente
        # U8g2 y van en UTF-8, asi que ni son ASCII ni tienen mayusculas que
        # comprobar.
        LATINOS, CJK = ('fr', 'de'), ('ja', 'ko')
        for num, langs in self.NAMES.items():
            self.assertIn(num, self.byNum, f'nombre traducido de una especie inexistente: {num}')
            for lang, name in langs.items():
                self.assertIn(lang, LATINOS + CJK, f'idioma inesperado: {lang}')
                self.assertTrue(name, f'{num} {lang} vacio')
                if lang in LATINOS:
                    self.assertTrue(name.isascii(), f'{num} {lang} no es ASCII: {name}')
                    self.assertEqual(name, name.upper(), f'{num} {lang} lleva minusculas: {name}')
                    self.assertLessEqual(len(name), 12, f'{num} {lang} no cabe en pantalla: {name}')
                else:
                    # el limite util son CARACTERES, no bytes: en UTF-8 cada kana o silaba hangul ocupa 3
                    self.assertLessEqual(len(name), 8, f'{num} {lang} no cabe en pantalla: {name}')


class TestGeneratedFiles(unittest.TestCase):
    """dex.h esta generado: si no coincide con su generador, alguien lo edito a mano."""

    def test_gen_dex_reproduce_dex_h(self):
        tmp = tempfile.mkdtemp(prefix='tamapoke-gen-')
        try:
            shutil.copytree(TOOLS, os.path.join(tmp, 'tools'))
            r = subprocess.run([sys.executable, os.path.join(tmp, 'tools', 'gen_dex.py')],
                               capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, f'gen_dex.py fallo:\n{r.stderr}')
            generated = os.path.join(tmp, 'dex.h')
            self.assertTrue(os.path.exists(generated), 'gen_dex.py no escribio dex.h')
            with open(generated) as a, open(os.path.join(ROOT, 'dex.h')) as b:
                self.assertEqual(a.read(), b.read(),
                                 'dex.h no coincide con lo que genera tools/gen_dex.py '
                                 '(vuelve a generarlo en vez de editarlo a mano)')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_el_paquete_de_sprites_de_la_175c_coincide_con_los_bundles(self):
        # la 1.75C (sin SD) lee los sprites de la particion "sprites": cada entrada
        # del paquete TPKZ tiene que descomprimirse a exactamente el fichero del
        # bundle de su region, y el indice tiene que ir ordenado (el firmware hace
        # busqueda binaria). Se compara el contenido, no los bytes comprimidos.
        import glob
        import struct
        import zlib
        sys.path.insert(0, TOOLS)
        import pack_flash
        path = os.path.join(ROOT, 'web', 'firmware', '175c', 'sprites.bin')
        if not os.path.exists(path):
            self.skipTest('sin web/firmware/175c/sprites.bin (ejecuta tools/build_web.sh)')
        with open(path, 'rb') as fh:
            blob = fh.read()
        self.assertEqual(blob[:4], b'TPKZ')
        self.assertLessEqual(len(blob), pack_flash.PARTITION_SIZE)
        n = struct.unpack_from('<H', blob, 4)[0]
        idx = [pack_flash.ENTRY.unpack_from(blob, 8 + i * pack_flash.ENTRY.size) for i in range(n)]
        names = [e[0].rstrip(b'\0') for e in idx]
        self.assertEqual(names, sorted(names), 'el indice TPKZ tiene que ir ordenado')
        by_name = dict(zip(names, idx))
        files = pack_flash.collect(glob.glob(pack_flash.PAKS))
        self.assertEqual(set(by_name), {k.encode() for k in files},
                         'sprites.bin no lleva los mismos ficheros que web/sprites-*.pak '
                         '(ejecuta python3 tools/pack_flash.py)')
        for name, data in files.items():
            _, off, zlen, raw = by_name[name.encode()]
            self.assertEqual(raw, len(data), f'{name}: rawLen no cuadra')
            self.assertEqual(zlib.decompress(blob[off:off + zlen]), data,
                             f'{name} en sprites.bin no coincide con su bundle')

    def test_los_ficheros_generados_avisan_de_que_lo_son(self):
        for name in ('dex.h', 'species.h'):
            with open(os.path.join(ROOT, name)) as fh:
                head = fh.read(400)
            self.assertIn('GENERADO', head, f'{name} deberia decir que esta generado')


class TestInstalador(unittest.TestCase):
    """El instalador web no puede borrar la partida al actualizar.

    La partida vive en la particion NVS. Una imagen combinada desde 0x0 la
    escribia entera con 0xFF (merge-bin rellena los huecos), y
    new_install_prompt_erase=false hace que ESP Web Tools borre el chip entero
    sin preguntar. Las dos cosas destruian el guardado en cada actualizacion.
    """

    MANIFESTS = ('manifest.json', 'manifest-175c.json')
    PROTECTED = ('nvs', 'tamabox')  # la partida y la caja

    @staticmethod
    def partitions(path):
        import struct
        with open(path, 'rb') as fh:
            data = fh.read()
        table = {}
        for i in range(0, len(data), 32):
            e = data[i:i + 32]
            if e[:2] != b'\xaa\x50':
                break
            off, size = struct.unpack_from('<II', e, 4)
            table[e[12:28].rstrip(b'\0').decode()] = (off, size)
        return table

    def test_ninguna_pieza_pisa_la_partida_ni_se_borra_el_chip(self):
        import json
        web = os.path.join(ROOT, 'web')
        for name in self.MANIFESTS:
            with open(os.path.join(web, name)) as fh:
                m = json.load(fh)
            self.assertIs(m.get('new_install_prompt_erase'), True,
                          f'{name}: new_install_prompt_erase tiene que ser true '
                          '(false = borrar el chip entero sin preguntar)')
            parts = m['builds'][0]['parts']
            ptable = next((p['path'] for p in parts if p['path'].endswith('partitions.bin')), None)
            self.assertIsNotNone(ptable, f'{name}: falta partitions.bin entre las piezas')
            table = self.partitions(os.path.join(web, ptable))
            for prot in self.PROTECTED:
                self.assertIn(prot, table, f'{name}: la tabla no tiene particion {prot}')
            for p in parts:
                path = os.path.join(web, p['path'])
                self.assertTrue(os.path.exists(path), f'{name}: no existe {p["path"]}')
                start, end = p['offset'], p['offset'] + os.path.getsize(path)
                for prot in self.PROTECTED:
                    po, ps = table[prot]
                    self.assertTrue(end <= po or start >= po + ps,
                                    f'{name}: {p["path"]} (0x{start:x}-0x{end:x}) pisa la '
                                    f'particion {prot} (0x{po:x}-0x{po + ps:x}): borraria la partida')

    def test_la_partida_sigue_donde_estaba(self):
        # la tabla de particiones cambio (app de 4 MB, caja, sprites), pero la NVS
        # tiene que seguir EXACTAMENTE en 0x9000 con 20 KB, como en app3M_fat9M_16MB:
        # si se moviera, la primera actualizacion perderia la partida
        for d in ('175', '175c'):
            path = os.path.join(ROOT, 'web', 'firmware', d, 'partitions.bin')
            table = self.partitions(path)
            self.assertEqual(table['nvs'], (0x9000, 0x5000), f'{d}: la NVS se ha movido')
            self.assertIn('sprites', table)
            self.assertIn('tamabox', table)

    def test_los_sprites_de_la_175c_caben_en_su_particion(self):
        import json
        web = os.path.join(ROOT, 'web')
        with open(os.path.join(web, 'manifest-175c.json')) as fh:
            parts = json.load(fh)['builds'][0]['parts']
        table = self.partitions(os.path.join(web, 'firmware', '175c', 'partitions.bin'))
        sp = next(p for p in parts if p['path'].endswith('sprites.bin'))
        po, ps = table['sprites']
        size = os.path.getsize(os.path.join(web, sp['path']))
        self.assertEqual(sp['offset'], po, 'sprites.bin no va al principio de la particion sprites')
        self.assertLessEqual(size, ps, 'sprites.bin no cabe en la particion sprites')


class TestSketchSanity(unittest.TestCase):
    """Comprobaciones baratas sobre el .ino que no necesitan compilarlo."""

    @classmethod
    def setUpClass(cls):
        with open(os.path.join(ROOT, 'TamaPoke.ino')) as fh:
            cls.ino = fh.read()

    def test_el_sketch_no_lleva_caracteres_no_ascii_en_literales_de_pantalla(self):
        # los comentarios pueden llevar lo que quieran; los literales, no:
        # la fuente GFX es ASCII y un acento sale como basura
        for lineno, line in enumerate(self.ino.splitlines(), 1):
            code = line.split('//')[0]
            for chunk in code.split('"')[1::2]:
                self.assertTrue(chunk.isascii(),
                                f'TamaPoke.ino:{lineno}: literal no ASCII: {chunk!r}')

    def test_las_llaves_estan_equilibradas(self):
        self.assertEqual(self.ino.count('{'), self.ino.count('}'),
                         'llaves descompensadas en TamaPoke.ino')



class TestBuffersDeTexto(unittest.TestCase):
    """Cada snprintf(buf, sizeof(buf), T(S_x), ...) cabe en su buffer en TODOS los idiomas.

    Los buffers se dimensionaron cuando toda cadena era de un byte por
    caracter. En UTF-8 un kana o una silaba hangul ocupan 3, y snprintf corta
    en silencio: a veces se pierden cifras ("きろく 2" por "きろく 219") y a
    veces corta a mitad de un caracter, que la fuente ya no sabe dibujar. No lo
    avisa ni el compilador ni la pantalla, asi que se comprueba aqui con el
    peor caso de cada argumento segun su tipo:

      %u / %d  -> 5 cifras (todos los argumentos son de 16 bits o menos)
      %lu      -> 10 cifras
      %s       -> lo que declare ARGS_S; un %s sin declarar hace fallar el
                  test, para que quien lo anada piense cuanto puede ocupar
    """

    ARGS_S = {  # que puede llegar por cada %s, en orden
        'S_NAME_FMT': ('estrella', 'nombre_o_apodo'),
        'S_RELEASE_FMT': ('nombre',),
        'S_INFO_FMT': ('baya',),
        'S_FAREWELL_BTN': ('nombre_o_apodo',),
        'S_RUNAWAY_BTN': ('nombre_o_apodo',),
        # v3: equipo, caja y combates. Estos pasan DEX_TBL[].name (el nombre base,
        # sin traducir) o el apodo, no dexName()
        'S_PARTY_JOINED': ('nombre_base_o_apodo',),
        'S_LEARN_Q': ('nombre_base_o_apodo',),
        'S_BOX_SWAP': ('nombre_base_o_apodo',),
        'S_BTL_BEAT': ('entrenador',),
        'S_WIN_TRAIN_FMT': ('stat_entreno',),
    }
    CIFRAS = {'u': 5, 'd': 5, 'lu': 10}
    ESPEC = re.compile(r'%0?\d*(lu|u|d|s)')

    @classmethod
    def setUpClass(cls):
        import test_i18n_formats as fmt
        from dex_data import DEX
        from dex_names import LOCAL_NAMES

        def leer(nombre):
            with open(os.path.join(ROOT, nombre), encoding='utf-8') as fh:
                return fh.read()

        cls.LANGS = fmt.LANGS
        cuerpo = re.search(r'enum\s+StrId[^{]*\{(.*?)\}', leer('i18n.h'), re.S).group(1)
        # sin comentarios: i18n.h nombra ids en ellos ("reuses S_RELEASE_FMT /
        # S_YES / S_NO") y descuadraban la correspondencia id -> cadena
        cuerpo = re.sub(r'//[^\n]*', '', cuerpo)
        ids = re.findall(r'\b(S_[A-Z0-9_]+)\b', cuerpo)
        filas = fmt.extract_table(leer('i18n.cpp'), 'STRINGS', len(cls.LANGS))
        cls.IDS, cls.FILAS = ids, filas
        # una fila corta deja huecos a NULL en el firmware: aqui quedan a None y
        # test_cada_idioma_tiene_todas_las_cadenas lo informa (antes reventaba
        # el setUpClass con un IndexError y no se veia ningun otro resultado)
        cls.LIT = {sid: [fila[k] if k < len(fila) else None for fila in filas]
                   for k, sid in enumerate(ids)}
        cls.ino = leer('TamaPoke.ino').splitlines()
        cls.APODO = int(re.search(r'char\s+nick\[(\d+)\]', leer('pet.h')).group(1)) - 1
        base = max(len(row[2].encode()) for row in DEX)
        cls.BASE = base
        cls.ENTRENADOR = max(len(n.encode()) for n in
                             re.findall(r'\{\s*"([^"]+)"', leer('trainers.h')))
        cls.NOMBRE = {}
        for lang in cls.LANGS:
            propios = [n[lang.lower()] for n in LOCAL_NAMES.values() if lang.lower() in n]
            cls.NOMBRE[lang] = max([base] + [len(n.encode('utf-8')) for n in propios])

    @staticmethod
    def bytes_literal(literal):
        """Bytes que ocupa en memoria un literal C escrito como en el fuente."""
        cuerpo = literal[1:-1].encode('utf-8')
        n = i = 0
        while i < len(cuerpo):
            if cuerpo[i] == 0x5C:  # barra invertida: \ooo octal, o \n \" \\ ...
                j = i + 1
                while j < len(cuerpo) and j < i + 4 and 0x30 <= cuerpo[j] <= 0x37:
                    j += 1
                i = j if j > i + 1 else i + 2
            else:
                i += 1
            n += 1
        return n

    def test_cada_idioma_tiene_todas_las_cadenas(self):
        for lang, fila in zip(self.LANGS, self.FILAS):
            with self.subTest(idioma=lang):
                faltan = self.IDS[len(fila):]
                self.assertEqual(len(fila), len(self.IDS),
                                 f'STRINGS[{lang}] tiene {len(fila)} cadenas de {len(self.IDS)}: '
                                 f'T() devuelve NULL para {faltan[0] if faltan else "?"}..'
                                 f'{faltan[-1] if faltan else "?"}')

    def peor_caso(self, sid, li):
        lang, lit = self.LANGS[li], self.LIT[sid][li]
        if lit is None:
            self.skipTest(f'{sid} no existe en {lang} (lo informa test_cada_idioma_tiene_todas_las_cadenas)')
        total = self.bytes_literal(lit) + 1  # + terminador
        pendientes = list(self.ARGS_S.get(sid, ()))
        for m in self.ESPEC.finditer(lit):
            total -= len(m.group(0))
            if m.group(1) != 's':
                total += self.CIFRAS[m.group(1)]
                continue
            self.assertTrue(pendientes, f'{sid}: tiene un %s sin declarar en ARGS_S')
            arg = pendientes.pop(0)
            if arg == 'estrella':
                total += 1
            elif arg == 'nombre':
                total += self.NOMBRE[lang]
            elif arg == 'nombre_o_apodo':
                total += max(self.NOMBRE[lang], self.APODO)
            elif arg == 'nombre_base_o_apodo':
                total += max(self.BASE, self.APODO)
            elif arg == 'entrenador':
                total += self.ENTRENADOR
            elif arg == 'stat_entreno':
                total += max(self.bytes_literal(self.LIT[b][li]) for b in
                             ('S_TR_ATK', 'S_TR_DEF', 'S_TR_SPE'))
            elif arg == 'baya':
                total += max(self.bytes_literal(self.LIT[b][li]) for b in
                             ('S_BERRY_UNK', 'S_BERRY_RED', 'S_BERRY_BLUE', 'S_BERRY_GREEN'))
        return total

    def test_cada_texto_formateado_cabe_en_su_buffer_en_todos_los_idiomas(self):
        llamada = re.compile(r'snprintf\((\w+),\s*sizeof\(\1\),\s*T\((S_[A-Z0-9_]+)\)')
        revisadas = 0
        for n, linea in enumerate(self.ino, 1):
            m = llamada.search(linea.split('//')[0])
            if not m:
                continue
            var, sid = m.groups()
            tam = next((int(d.group(1)) for k in range(n - 2, -1, -1)
                        for d in [re.search(r'\bchar\s+' + var + r'\s*\[(\d+)\]', self.ino[k])]
                        if d), None)
            self.assertIsNotNone(tam, f'TamaPoke.ino:{n}: no encuentro la declaracion de {var}')
            revisadas += 1
            for li, lang in enumerate(self.LANGS):
                with self.subTest(linea=n, id=sid, idioma=lang):
                    peor = self.peor_caso(sid, li)
                    self.assertLessEqual(
                        peor, tam,
                        f'TamaPoke.ino:{n}: {var}[{tam}] con {sid} en {lang} necesita {peor} bytes')
        self.assertGreater(revisadas, 10, 'el patron de snprintf ya no encuentra las llamadas')

if __name__ == '__main__':
    unittest.main(verbosity=2)
