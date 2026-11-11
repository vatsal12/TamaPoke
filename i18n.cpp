#include "i18n.h"
#include "pet.h"        // MED_COUNT
#include <Preferences.h>

Lang gLang = LANG_DEFAULT;

// Tabla de cadenas [idioma][id]. Sin acentos ni enes: la fuente bitmap del
// firmware no los tiene (por eso el espanol ya iba "Esta", "bano", etc.).
static const char *const STRINGS[LANG_COUNT][STR_COUNT] = {
  // ---------------- ES ----------------
  {
    "Est\240 evolucionando!", "Nam nam!", "Le gusta!", "Tiene hambre!", "Necesita un ba\244o!",
    "Est\240 agotado...", "Est\240 triste...", "Est\240 rellenito...", "Es SHINY!!", "Est\240 feliz",
    "GRACIAS! Hasta siempre", "Se ha escapado...", "Adi\242s! Se despide...",
    "HUEVO", "Huevo legendario!?", "Huevo raro!", "Toca el huevo...", "Se mueve!", "Est\240 a punto!",
    "POKEDEX %u/%u",
    "%s%s Nv.%u",
    "Soltar a %s?", "SI", "NO",
    "%u GOLPES", "FUERZA +%u", "NUEVO RECORD!", "RECORD: %u", "APORREA RAPIDO!",
    "PUNTOS: %u", "Qu\202 felicidad!", "+felicidad",
    "AJUSTAR HORA", "HORA", "MIN", "desliza arriba: cancelar", "Idioma",
    "MEDALLA!", "GENIAL!", "RACHA %u DIAS!",
    "RACHA %u  rec %u", "VIN", "BAYA ???", "BAYA ROJA", "BAYA AZUL", "BAYA VERDE",
    "%s   EDAD %lud", "toca el nombre: renombrar",
    "COMBATE", "FUE", "DEF", "VEL", "PES", "ENTRENAR FUERZA",
    "MEDALLAS %d/%d", "toca: volver",
    "NOMBRE:", "toca para volver",
    "COM", "FEL", "ENE", "LIM",
    "REC %u",
    "PROGRESO", "Nv.%u", "%u min para Nv.%u", "EVOLUCION", "Forma final",
    "Listo para evolucionar!", "Sube todo a 40 para evolucionar",
    "Evoluciona en %u niv.", "Descuidos: %u",
    "SON ON", "SON OFF",
    "EVOLUCIONAR", "%s quiere decirte algo...", "%s se siente abandonado...",
    "Evolucionar?", "Mantener forma", "Despedirse?", "Despedirse", "Quedaros juntos",
    "Elige tu inicial",
    "Sin sprites", "C\240rgalos en la SD",
    "PS", "IV %u",
    "MENU", "AJUSTES", "CERRAR", "EQUIPO %u/6", "- vacio -", "%s se une al equipo!", "Equipo lleno: elige a quien sustituir", "Dejarlo ir",
    "STATS", "ENTRENAR", "FUERZA", "VELOCIDAD", "DEFENSA", "Sube sola si esta a gusto",
    "MOVIMIENTOS", "- vacio -", "Elige movimiento", "Toca para cambiar", "POT %u", "ESTADO",
    "%s quiere aprender", "No aprender",
    "%s usa %s", "Es muy eficaz!", "No es muy eficaz...", "No le afecta...",
    "%s ha fallado!", "Golpe critico!", "%s se debilita!", "Se ha herido!",
    "%s: %s", "PARALISIS", "QUEMADURA", "VENENO", "DORMIDO", "CONGELADO", "CONFUSION",
    "Has ganado!", "Has perdido...",
    "%s saca a %s", "Adelante, %s!", "GIMNASIOS", "MEDALLAS %u/8", "ENTRENADOR", "VELOCIDAD +%u", "toca: cambiar avatar", "%u en total", "NORMAL", "DIFICIL",
 "ELEGIDOS %u/%u", "LUCHAR", "BLOQUEADO", "POKEMON", "%s derrotado!", "MEDALLA NUEVA!", "VOL %u", "CAJA %u/%u", "cambiar con %s: elige hueco", "CAJA", "TRAER", "solo con un huevo", "COMBATE LAN", "CREAR", "UNIRSE", "buscando...", "listo!", "version distinta", "crear o unirse", "rival: %u mons", "el rival se fue", "esperando al rival...", "OTRA VEZ", "HUIR", "de que region viene el huevo", "%s +%u", "ya no puede entrenar mas", "ELIGE TU REGION", "RETIRAR", "Retirarla ya?", "la siguiente evoluciona un dia mas tarde", "evoluciona un dia mas tarde",   "FALTA PACK", "SOLTAR", "se va para siempre", "AL EQUIPO", "no se unira a tu equipo", },
  // ---------------- EN ----------------
  {
    "Evolving!", "Yum yum!", "It likes it!", "It's hungry!", "Needs a bath!",
    "Worn out...", "Feeling sad...", "A bit chubby...", "It's SHINY!!", "It's happy",
    "THANKS! Farewell", "It ran away...", "Bye! Waving goodbye...",
    "EGG", "Legendary egg!?", "Rare egg!", "Tap the egg...", "It moves!", "Almost there!",
    "POKEDEX %u/%u",
    "%s%s Lv.%u",
    "Release %s?", "YES", "NO",
    "%u HITS", "STR +%u", "NEW RECORD!", "BEST: %u", "HIT FAST!",
    "SCORE: %u", "So much fun!", "+happiness",
    "SET TIME", "HOUR", "MIN", "swipe up: cancel", "Lang",
    "MEDAL!", "AWESOME!", "%u DAY STREAK!",
    "STREAK %u  best %u", "BOND", "BERRY ???", "RED BERRY", "BLUE BERRY", "GREEN BERRY",
    "%s   AGE %lud", "tap name: rename",
    "BATTLE", "ATK", "DEF", "SPD", "WGT", "TRAIN STRENGTH",
    "MEDALS %d/%d", "tap: back",
    "NAME:", "tap to go back",
    "FOOD", "JOY", "ENE", "HYG",
    "BEST %u",
    "PROGRESS", "Lv.%u", "%u min to Lv.%u", "EVOLUTION", "Final form",
    "Ready to evolve!", "All needs >=40 to evolve",
    "Evolves in %u lv.", "Slip-ups: %u",
    "SND ON", "SND OFF",
    "EVOLVE!", "%s wants to tell you...", "%s feels abandoned...",
    "Evolve?", "Keep form", "Say goodbye?", "Goodbye", "Stay together",
    "Choose your starter",
    "No sprites", "Load them onto the SD",
    "HP", "IV %u",
    "MENU", "SETTINGS", "CLOSE", "PARTY %u/6", "- empty -", "%s joined the party!", "Party full: pick who to replace", "Let it go",
    "STATS", "TRAINING", "STRENGTH", "SPEED", "DEFENCE", "Grows on its own when happy",
    "MOVES", "- empty -", "Choose a move", "Tap a slot to change", "PWR %u", "STATUS",
    "%s wants to learn", "Do not learn",
    "%s used %s", "Super effective!", "Not very effective...", "It had no effect...",
    "%s missed!", "Critical hit!", "%s fainted!", "It hurt itself!",
    "%s: %s", "PARALYSED", "BURNED", "POISONED", "ASLEEP", "FROZEN", "CONFUSED",
    "You win!", "You lost...",
    "%s sends out %s", "Go, %s!", "GYMS", "BADGES %u/8", "TRAINER",
 "SPEED +%u", "tap: change avatar", "%u earned in all", "EASY", "HARD", "CHOSEN %u/%u", "FIGHT", "LOCKED", "POKEMON", "%s defeated!", "NEW BADGE!", "VOL %u", "BOX %u/%u", "swap with %s: pick a slot", "BOX", "BRING BACK", "only while an egg waits", "LAN BATTLE", "HOST", "JOIN", "searching...", "ready!", "different version", "host or join", "rival: %u mons", "the rival left", "waiting for the rival...", "AGAIN", "RUN", "where this egg comes from", "%s +%u", "trained as far as it can go", "CHOOSE A REGION", "RETIRE", "Retire it early?", "the next one evolves a day later", "evolves a day later",
   "NEEDS PACK", "RELEASE", "gone for good", "TO PARTY", "it will not join your party", },
  // ---------------- FR ----------------
  {
    "Il \202volue!", "Miam miam!", "Il aime \207a!", "Il a faim!", "Besoin d'un bain!",
    "\220puis\202...", "Triste...", "Un peu rond...", "C'est SHINY!!", "Il est content",
    "MERCI! Adieu", "Il s'est enfui...", "Au revoir!",
    "OEUF", "Oeuf l\202gendaire!?", "Oeuf rare!", "Touche l'oeuf...", "Il bouge!", "Presque l\205!",
    "POKEDEX %u/%u",
    "%s%s Niv.%u",
    "Rel\203cher %s?", "OUI", "NON",
    "%u COUPS", "FORCE +%u", "NOUVEAU RECORD!", "RECORD: %u", "FRAPPE VITE!",
    "SCORE: %u", "Trop bien!", "+bonheur",
    "R\220GLER L'HEURE", "HEURE", "MIN", "glisse haut: annuler", "Langue",
    "M\220DAILLE!", "SUPER!", "S\220RIE %u JOURS!",
    "S\220RIE %u  rec %u", "LIEN", "BAIE ???", "BAIE ROUGE", "BAIE BLEUE", "BAIE VERTE",
    "%s   AGE %lud", "touche le nom: renommer",
    "COMBAT", "ATQ", "DEF", "VIT", "PDS", "ENTRAINER FORCE",
    "M\220DAILLES %d/%d", "touche: retour",
    "NOM:", "touche pour revenir",
    "NOUR", "JOIE", "ENE", "HYG",
    "REC %u",
    "PROGRES", "Niv.%u", "%u min pour Niv.%u", "\220VOLUTION", "Forme finale",
    "Pr\210t \205 \202voluer!", "Tout \205 40 pour \202voluer",
    "\220volue dans %u niv.", "N\202gligences: %u",
    "SON ON", "SON OFF",
    "\220VOLUER", "%s veut te parler...", "%s se sent abandonn\202...",
    "\220voluer?", "Garder forme", "Dire adieu?", "Adieu", "Rester ensemble",
    "Choisis ton starter",
    "Pas de sprites", "Charge-les sur la SD",
    "PV", "IV %u",
    "MENU", "REGLAGES", "FERMER", "EQUIPE %u/6", "- vide -", "%s rejoint l'equipe!", "Equipe pleine: qui remplacer?", "Le laisser partir",
    "STATS", "ENTRAINEMENT", "FORCE", "VITESSE", "DEFENSE", "Monte seule s'il est heureux",
    "CAPACITES", "- vide -", "Choisis une capacite", "Touche pour changer", "PUI %u", "STATUT",
    "%s veut apprendre", "Ne pas apprendre",
    "%s utilise %s", "Tres efficace!", "Peu efficace...", "Aucun effet...",
    "%s a rate!", "Coup critique!", "%s est K.O.!", "Il se blesse!",
    "%s: %s", "PARALYSIE", "BRULURE", "POISON", "ENDORMI", "GELE", "CONFUSION",
    "Gagne!", "Perdu...",
    "%s envoie %s", "Vas-y, %s!", "ARENES", "BADGES %u/8", "DRESSEUR", "VITESSE +%u", "touche: changer d'avatar", "%u au total", "NORMAL", "DIFFICILE", "CHOISIS %u/%u", "COMBATTRE", "VERROUILLE", "POKEMON", "%s vaincu!", "NOUVEAU BADGE!", "VOL %u", "BOITE %u/%u", "echanger avec %s: choisis", "BOITE", "RAMENER", "seulement avec un oeuf", "COMBAT LAN", "CREER", "REJOINDRE", "recherche...", "pret!", "version differente", "creer ou rejoindre", "rival: %u mons", "le rival est parti", "en attente du rival...", "ENCORE", "FUIR", "d ou vient cet oeuf", "%s +%u", "ne peut plus progresser", "CHOISIS TA REGION", "RETIRER", "Retirer maintenant?", "le suivant evolue un jour plus tard", "evolue un jour plus tard",
   "PACK REQUIS", "RELACHER", "parti pour de bon", "A L EQUIPE", "ne rejoindra pas l equipe", },
  // ---------------- DE ----------------
  // El aleman si lleva dieresis: la fuente 5x7 del firmware es una tabla CP437
  // completa (256 glifos), asi que basta con el byte suelto de cada caracter.
  // Van en octal para no depender de la codificacion del fichero fuente:
  //   \204 a   \224 o   \201 u   \216 A   \231 O   \232 U   \341 ss
  // Un byte por caracter: strlen() sigue midiendo bien y el centrado no cambia.
  {
    "Entwickelt sich!", "Mampf mampf!", "Gef\204llt ihm!", "Hat Hunger!", "Braucht ein Bad!",
    "Ersch\224pft...", "Traurig...", "Etwas rundlich...", "Es ist SHINY!!", "Es ist froh",
    "DANKE! Leb wohl", "Es ist weg...", "Tsch\201ss! Winkt",
    "EI", "Legend\204res Ei!?", "Seltenes Ei!", "Ber\201hre das Ei...", "Es bewegt sich!", "Fast so weit!",
    "POKEDEX %u/%u",
    "%s%s Lv.%u",
    "%s freilassen?", "JA", "NEIN",
    "%u TREFFER", "KRAFT +%u", "NEUER REKORD!", "REKORD: %u", "SCHNELL HAUEN!",
    "PUNKTE: %u", "Wie sch\224n!", "+Freude",
    "ZEIT STELLEN", "STD", "MIN", "hoch wischen: Abbruch", "Sprache",
    "MEDAILLE!", "TOLL!", "%u TAGE SERIE!",
    "SERIE %u  rek %u", "BND", "BEERE ???", "ROTE BEERE", "BLAUE BEERE", "GR\232NE BEERE",
    "%s   ALTER %lud", "Name tippen: umbenennen",
    "KAMPF", "ANG", "VER", "INI", "GEW", "KRAFT TRAINIEREN",
    "MEDAILLEN %d/%d", "tippen: zur\201ck",
    "NAME:", "tippen: zur\201ck",
    "ESS", "FRO", "ENE", "HYG",
    "REK %u",
    "FORTSCHRITT", "Lv.%u", "%u min bis Lv.%u", "ENTWICKLUNG", "Endform",
    "Bereit zur Entwicklung!", "Alles >=40 zur Entwicklung",
    "Entwicklung in %u Lv.", "Patzer: %u",
    "TON AN", "TON AUS",
    "ENTWICKELN", "%s will dir etwas sagen...", "%s f\201hlt sich verlassen...",
    "Entwickeln?", "Form behalten", "Abschied?", "Leb wohl", "Zusammen bleiben",
    "W\204hle deinen Starter",
    "Keine Sprites", "Auf die SD laden",
    "KP", "IV %u",
    "MENU", "EINSTELLUNGEN", "SCHLIESSEN", "TEAM %u/6", "- leer -", "%s kommt ins Team!", "Team voll: wen ersetzen?", "Ziehen lassen",
    "WERTE", "TRAINING", "STAERKE", "TEMPO", "ABWEHR", "Steigt von selbst bei guter Laune",
    "ATTACKEN", "- leer -", "Attacke waehlen", "Tippen zum Aendern", "STK %u", "STATUS",
    "%s will lernen", "Nicht lernen",
    "%s setzt %s ein", "Sehr effektiv!", "Nicht sehr effektiv...", "Keine Wirkung...",
    "%s hat verfehlt!", "Volltreffer!", "%s wurde besiegt!", "Es verletzt sich!",
    "%s: %s", "PARALYSE", "VERBRANNT", "VERGIFTET", "SCHLAEFT", "GEFROREN", "VERWIRRT",
    "Gewonnen!", "Verloren...",
    "%s schickt %s", "Los, %s!", "ARENEN", "ORDEN %u/8", "TRAINER", "TEMPO +%u", "tippen: Avatar wechseln", "%u insgesamt", "NORMAL", "SCHWER", "GEWAEHLT %u/%u", "KAEMPFEN", "GESPERRT", "POKEMON", "%s besiegt!", "NEUER ORDEN!", "LAUT %u", "BOX %u/%u", "mit %s tauschen: waehle", "BOX", "ZURUECK", "nur mit einem Ei", "LAN KAMPF", "HOSTEN", "BEITRETEN", "suche...", "bereit!", "andere Version", "hosten oder beitreten", "Gegner: %u", "der Gegner ist weg", "warte auf den Gegner...", "NOCHMAL", "FLUCHT", "woher dieses Ei kommt", "%s +%u", "kann nicht weiter trainieren", "WAEHLE DEINE REGION", "VERABSCHIEDEN", "Jetzt verabschieden?", "das naechste entwickelt sich einen Tag spaeter", "entwickelt sich einen Tag spaeter",
   "PACK FEHLT", "FREILASSEN", "fuer immer weg", "INS TEAM", "kommt nicht ins team", },
  // ---------------- IT ----------------
  {
    "Si evolve!", "Gnam gnam!", "Gli piace!", "Ha fame!", "Vuole un bagno!",
    "Esausto...", "Triste...", "Un po' cicciotto...", "E' SHINY!!", "E' felice",
    "GRAZIE! Addio", "E' scappato...", "Ciao! Saluta",
    "UOVO", "Uovo leggendario!?", "Uovo raro!", "Tocca l'uovo...", "Si muove!", "Ci siamo quasi!",
    "POKEDEX %u/%u",
    "%s%s Lv.%u",
    "Liberare %s?", "SI", "NO",
    "%u COLPI", "FORZA +%u", "NUOVO RECORD!", "RECORD: %u", "COLPISCI VELOCE!",
    "PUNTI: %u", "Che gioia!", "+felicit\205",
    "IMPOSTA ORA", "ORA", "MIN", "scorri su: annulla", "Lingua",
    "MEDAGLIA!", "GRANDE!", "SERIE %u GIORNI!",
    "SERIE %u  rec %u", "LEG", "BACCA ???", "BACCA ROSSA", "BACCA BLU", "BACCA VERDE",
    "%s   ETA %lud", "tocca il nome: rinomina",
    "LOTTA", "ATT", "DIF", "VEL", "PES", "ALLENA FORZA",
    "MEDAGLIE %d/%d", "tocca: indietro",
    "NOME:", "tocca per tornare",
    "CIB", "GIO", "ENE", "IGI",
    "REC %u",
    "PROGRESSI", "Lv.%u", "%u min per Lv.%u", "EVOLUZIONE", "Forma finale",
    "Pronto a evolvere!", "Tutto a 40 per evolvere",
    "Evolve tra %u liv.", "Disattenzioni: %u",
    "AUD ON", "AUD OFF",
    "EVOLVI", "%s vuole dirti qualcosa...", "%s si sente abbandonato...",
    "Evolvere?", "Mantieni forma", "Salutare?", "Addio", "Restare insieme",
    "Scegli l'iniziale",
    "Senza sprite", "Caricali sulla SD",
    "PS", "IV %u",
    "MENU", "IMPOSTAZIONI", "CHIUDI", "SQUADRA %u/6", "- vuoto -", "%s entra in squadra!", "Squadra piena: chi sostituire?", "Lasciarlo andare",
    "STATS", "ALLENAMENTO", "FORZA", "VELOCITA", "DIFESA", "Sale da sola se sta bene",
    "MOSSE", "- vuoto -", "Scegli una mossa", "Tocca per cambiare", "POT %u", "STATO",
    "%s vuole imparare", "Non imparare",
    "%s usa %s", "Superefficace!", "Non molto efficace...", "Nessun effetto...",
    "%s ha mancato!", "Brutto colpo!", "%s e\' esausto!", "Si e ferito!",
    "%s: %s", "PARALISI", "SCOTTATURA", "VELENO", "ADDORMENTATO", "CONGELATO", "CONFUSIONE",
    "Hai vinto!", "Hai perso...",
    "%s manda %s", "Vai, %s!", "PALESTRE", "MEDAGLIE %u/8", "ALLENATORE", "VELOCITA +%u", "tocca: cambia avatar", "%u in totale", "NORMALE", "DIFFICILE", "SCELTI %u/%u", "LOTTA", "BLOCCATO", "POKEMON", "%s sconfitto!", "NUOVA MEDAGLIA!", "VOL %u", "BOX %u/%u", "scambia con %s: scegli", "BOX", "RIPORTA", "solo con un uovo", "LOTTA LAN", "CREA", "ENTRA", "ricerca...", "pronto!", "versione diversa", "crea o entra", "rivale: %u mons", "il rivale se n' e andato", "in attesa del rivale...", "ANCORA", "FUGGI", "da quale regione viene l uovo", "%s +%u", "non puo allenarsi oltre", "SCEGLI LA REGIONE", "RITIRARE", "Ritirarla adesso?", "il prossimo evolve un giorno dopo", "evolve un giorno dopo",
   "MANCA PACK", "LIBERA", "via per sempre", "AL GRUPPO", "non entrera nel gruppo", },
  // ---------------- PT ----------------
  {
    "Evoluindo!", "Nham nham!", "Ele gosta!", "Est\240 com fome!", "Precisa de banho!",
    "Exausto...", "Triste...", "Um pouco gordinho...", "\220 SHINY!!", "Est\240 feliz",
    "OBRIGADO! Adeus", "Fugiu...", "Tchau! Acena",
    "OVO", "Ovo lend\240rio!?", "Ovo raro!", "Toque no ovo...", "Mexe-se!", "Quase l\240!",
    "POKEDEX %u/%u",
    "%s%s Niv.%u",
    "Soltar %s?", "SIM", "NAO",
    "%u GOLPES", "FOR\200A +%u", "NOVO RECORDE!", "RECORDE: %u", "BATA RAPIDO!",
    "PONTOS: %u", "Que alegria!", "+alegria",
    "AJUSTAR HORA", "HORA", "MIN", "deslize cima: cancelar", "Idioma",
    "MEDALHA!", "OTIMO!", "%u DIAS SEGUIDOS!",
    "SEQ %u  rec %u", "LA\200O", "BAGA ???", "BAGA VERMELHA", "BAGA AZUL", "BAGA VERDE",
    "%s   IDADE %lud", "toque no nome: renomear",
    "COMBATE", "ATQ", "DEF", "VEL", "PES", "TREINAR FOR\200A",
    "MEDALHAS %d/%d", "toque: voltar",
    "NOME:", "toque para voltar",
    "COM", "ALE", "ENE", "HIG",
    "REC %u",
    "PROGRESSO", "Niv.%u", "%u min para Niv.%u", "EVOLUCAO", "Forma final",
    "Pronto a evoluir!", "Tudo a 40 para evoluir",
    "Evolui em %u niv.", "Descuidos: %u",
    "SOM ON", "SOM OFF",
    "EVOLUIR", "%s quer dizer-te algo...", "%s sente-se abandonado...",
    "Evoluir?", "Manter forma", "Despedir?", "Adeus", "Ficar juntos",
    "Escolhe o inicial",
    "Sem sprites", "Carrega-os no SD",
    "PS", "IV %u",
    "MENU", "AJUSTES", "FECHAR", "EQUIPA %u/6", "- vazio -", "%s junta-se a equipa!", "Equipa cheia: quem substituir?", "Deixa-lo ir",
    "STATS", "TREINO", "FORCA", "VELOCIDADE", "DEFESA", "Sobe sozinha se estiver bem",
    "GOLPES", "- vazio -", "Escolhe um golpe", "Toca para mudar", "POT %u", "ESTADO",
    "%s quer aprender", "Nao aprender",
    "%s usa %s", "Super eficaz!", "Pouco eficaz...", "Nao teve efeito...",
    "%s falhou!", "Acerto critico!", "%s desmaiou!", "Feriu-se!",
    "%s: %s", "PARALISIA", "QUEIMADURA", "VENENO", "A DORMIR", "CONGELADO", "CONFUSAO",
    "Ganhaste!", "Perdeste...",
    "%s envia %s", "Vai, %s!", "GINASIOS", "MEDALHAS %u/8", "TREINADOR", "VELOCIDADE +%u", "toca: mudar avatar", "%u no total", "NORMAL", "DIFICIL", "ESCOLHIDOS %u/%u", "LUTAR", "BLOQUEADO", "POKEMON", "%s derrotado!", "NOVA MEDALHA!", "VOL %u", "CAIXA %u/%u", "trocar com %s: escolhe", "CAIXA", "TRAZER", "so com um ovo", "COMBATE LAN", "CRIAR", "ENTRAR", "a procurar...", "pronto!", "versao diferente", "criar ou entrar", "rival: %u mons", "o rival saiu", "a esperar pelo rival...", "OUTRA VEZ", "FUGIR", "de que regiao vem o ovo", "%s +%u", "ja nao pode treinar mais", "ESCOLHE A REGIAO", "REFORMAR", "Reformar agora?", "o proximo evolui um dia depois", "evolui um dia depois",
   "FALTA PACK", "SOLTAR", "vai para sempre", "A EQUIPA", "nao entrara na equipa",
  },
  // ---------------- JA ----------------
  {
    "しんかした！", "おいしい！", "うれしそう！", "おなかすいた！", "おふろにいれて！",
    "つかれた...", "かなしい...", "ちょっとふとった...", "いろちがい！！", "ごきげん！",
    "ありがとう！ またね", "にげちゃった...", "バイバイ！",

    "たまご", "でんせつのたまご！？", "めずらしいたまご！", "たまごをタッチ...", "うごいた！", "もうすぐ！",

    "ずかん %u/%u",

    "%s%s Lv.%u",

    "%s とおわかれ？", "はい", "いいえ",

    "わざ %u", "ちから +%u", "しんきろく！", "きろく: %u", "れんだ！",

    "スコア: %u", "たのしい！", "+ごきげん",

    "じかんせってい", "じ", "ふん", "うえスワイプ: もどる", "ことば",

    "メダル！", "すごい！", "%u にちれんぞく！",

    "れんぞく %u  きろく %u", "なかよし", "きのみ ???", "あかいきのみ", "あおいきのみ", "みどりのきのみ",

    "%s   %luにち", "なまえをタッチ: へんこう",

    "バトル", "こうげき", "ぼうぎょ", "すばやさ", "たいじゅう", "ちからトレーニング",

    "メダル %d/%d", "タッチ: もどる",

    "なまえ:", "タッチでもどる",

    "ごはん", "ごきげん", "げんき", "きれい",

    "きろく %u",

    "せいちょう", "Lv.%u", "あと%uふんで Lv.%u", "しんか", "さいしゅうけい",

    "しんかできる！", "ぜんぶ40でしんか",

    "%u Lv.でしんか", "おせわミス: %u",

    "おと", "おと",

    "しんか", "%sがなにかいいたそう...", "%sがさびしがっている...",

    "しんかする？", "このまま", "おわかれする？", "さようなら", "いっしょにいる",

    "さいしょのポケモンをえらぶ",

    "スプライトなし", "SDにいれてください",
    // (sin traducir todavia: en ingles hasta que alguien los traduzca)
    "HP", "IV %u",
    "MENU", "SETTINGS", "CLOSE", "PARTY %u/6", "- empty -", "%s joined the party!", "Party full: pick who to replace", "Let it go",
    "STATS", "TRAINING", "STRENGTH", "SPEED", "DEFENCE", "Grows on its own when happy",
    "MOVES", "- empty -", "Choose a move", "Tap a slot to change", "PWR %u", "STATUS",
    "%s wants to learn", "Do not learn",
    "%s used %s", "Super effective!", "Not very effective...", "It had no effect...",
    "%s missed!", "Critical hit!", "%s fainted!", "It hurt itself!",
    "%s: %s", "PARALYSED", "BURNED", "POISONED", "ASLEEP", "FROZEN", "CONFUSED",
    "You win!", "You lost...",
    "%s sends out %s", "Go, %s!", "GYMS", "BADGES %u/8", "TRAINER",
 "SPEED +%u", "tap: change avatar", "%u earned in all", "EASY", "HARD", "CHOSEN %u/%u", "FIGHT", "LOCKED", "POKEMON", "%s defeated!", "NEW BADGE!", "VOL %u", "BOX %u/%u", "swap with %s: pick a slot", "BOX", "BRING BACK", "only while an egg waits", "LAN BATTLE", "HOST", "JOIN", "searching...", "ready!", "different version", "host or join", "rival: %u mons", "the rival left", "waiting for the rival...", "AGAIN", "RUN", "where this egg comes from", "%s +%u", "trained as far as it can go", "CHOOSE A REGION", "RETIRE", "Retire it early?", "the next one evolves a day later", "evolves a day later",
   "NEEDS PACK", "RELEASE", "gone for good", "TO PARTY", "it will not join your party",
  },
  // ---------------- KO ----------------
  // Los especificadores van en el MISMO ORDEN que en la fila inglesa aunque
  // el coreano natural los colocaria de otra manera (p. ej. S_NEXT_LVL_FMT):
  // T(id) se rellena con varargs en un orden fijo en cada llamada, asi que
  // reordenar %u/%s aqui sin tocar el punto de llamada intercambiaria en
  // silencio que valor cae en que hueco.
  {
    "진화 중!", "냠냠!", "마음에 들어해요!", "배고파요!", "목욕이 필요해요!",
    "지쳤어요...", "슬퍼해요...", "조금 통통해요...", "이로치예요!!", "행복해요",
    "고마워요! 안녕", "가출했어요...", "잘 가! 손을 흔들며...",
    "알", "전설의 알!?", "희귀한 알!", "알을 톡톡...", "움직여요!", "거의 다 됐어요!",
    "도감 %u/%u",
    "%s%s Lv.%u",
    "%s 놓아줄까요?", "예", "아니오",
    "%u회 명중", "힘 +%u", "신기록!", "최고: %u", "빠르게!",
    "점수: %u", "정말 즐거워요!", "+행복",
    "시간 설정", "시", "분", "위로 스와이프: 취소", "언어",
    "메달!", "최고예요!", "%u일 연속!",
    "연속 %u  최고 %u", "유대감", "열매 ???", "빨간 열매", "파란 열매", "초록 열매",
    "%s   나이 %lud", "이름 탭: 이름변경",
    "배틀", "공격", "방어", "속도", "무게", "힘 훈련",
    "메달 %d/%d", "탭: 뒤로",
    "이름:", "탭하면 뒤로",
    "포만", "기분", "기력", "청결",
    "최고 %u",
    "진행도", "Lv.%u", "%u분 후 Lv.%u", "진화", "최종 형태",
    "진화 준비 완료!", "진화하려면 전부 40 이상",
    "%u레벨 이후에 진화", "실수: %u",
    "소리 켬", "소리 끔",
    "진화!", "%s가 할 말이 있대요...", "%s가 버려진 기분이래요...",
    "진화할까요?", "형태 유지", "작별할까요?", "작별", "계속 함께",
    "스타터를 선택하세요",
    "스프라이트 없음", "SD 카드에 넣어주세요",
    // (sin traducir todavia: en ingles hasta que alguien los traduzca)
    "HP", "IV %u",
    "MENU", "SETTINGS", "CLOSE", "PARTY %u/6", "- empty -", "%s joined the party!", "Party full: pick who to replace", "Let it go",
    "STATS", "TRAINING", "STRENGTH", "SPEED", "DEFENCE", "Grows on its own when happy",
    "MOVES", "- empty -", "Choose a move", "Tap a slot to change", "PWR %u", "STATUS",
    "%s wants to learn", "Do not learn",
    "%s used %s", "Super effective!", "Not very effective...", "It had no effect...",
    "%s missed!", "Critical hit!", "%s fainted!", "It hurt itself!",
    "%s: %s", "PARALYSED", "BURNED", "POISONED", "ASLEEP", "FROZEN", "CONFUSED",
    "You win!", "You lost...",
    "%s sends out %s", "Go, %s!", "GYMS", "BADGES %u/8", "TRAINER",
 "SPEED +%u", "tap: change avatar", "%u earned in all", "EASY", "HARD", "CHOSEN %u/%u", "FIGHT", "LOCKED", "POKEMON", "%s defeated!", "NEW BADGE!", "VOL %u", "BOX %u/%u", "swap with %s: pick a slot", "BOX", "BRING BACK", "only while an egg waits", "LAN BATTLE", "HOST", "JOIN", "searching...", "ready!", "different version", "host or join", "rival: %u mons", "the rival left", "waiting for the rival...", "AGAIN", "RUN", "where this egg comes from", "%s +%u", "trained as far as it can go", "CHOOSE A REGION", "RETIRE", "Retire it early?", "the next one evolves a day later", "evolves a day later",
   "NEEDS PACK", "RELEASE", "gone for good", "TO PARTY", "it will not join your party",
  },
};

// Nombres de medalla en sus tres longitudes [idioma][medalla].
static const char *const MED_NAME[LANG_COUNT][MED_COUNT] = {
  { "Nv.10", "Nv.25", "Nv.50", "BAYA", "RACHA 7", "VINCULO", "FORMA TOPE", "EN FORMA" },
  { "Lv.10", "Lv.25", "Lv.50", "BERRY", "7 STREAK", "BOND", "TOP FORM", "IN SHAPE" },
  { "Niv.10", "Niv.25", "Niv.50", "BAIE", "SERIE 7", "LIEN", "FORME MAX", "EN FORME" },
  { "Lv.10", "Lv.25", "Lv.50", "BEERE", "7 SERIE", "BINDUNG", "ENDFORM", "FIT" },
  { "Lv.10", "Lv.25", "Lv.50", "BACCA", "SERIE 7", "LEGAME", "FORMA MAX", "IN FORMA" },
  { "Niv.10", "Niv.25", "Niv.50", "BAGA", "SEQ 7", "LACO", "FORMA MAX", "EM FORMA" },
  { "Lv.10", "Lv.25", "Lv.50", "きのみ", "7にち", "なかよし", "さいしゅう", "げんき" },
  { "Lv.10", "Lv.25", "Lv.50", "열매", "연속 7일", "유대", "최고 컨디션", "건강함" },
};
static const char *const MED_LBL[LANG_COUNT][MED_COUNT] = {
  { "Nv10", "Nv25", "Nv50", "BAYA", "7DIAS", "VINC", "TOPE", "SANO" },
  { "Lv10", "Lv25", "Lv50", "BERRY", "7DAYS", "BOND", "TOP", "FIT" },
  { "Niv10", "Niv25", "Niv50", "BAIE", "7JRS", "LIEN", "MAX", "FORME" },
  { "Lv10", "Lv25", "Lv50", "BEERE", "7TAGE", "BND", "END", "FIT" },
  { "Lv10", "Lv25", "Lv50", "BACCA", "7GG", "LEG", "MAX", "FIT" },
  { "Niv10", "Niv25", "Niv50", "BAGA", "7DIAS", "LACO", "MAX", "FIT" },
  { "Lv10", "Lv25", "Lv50", "きのみ", "7にち", "なかよし", "しんか", "げんき" },
  { "Lv10", "Lv25", "Lv50", "열매", "7일", "유대", "최고", "건강" },
};
static const char *const MED_DSC[LANG_COUNT][MED_COUNT] = {
  { "NIVEL 10", "NIVEL 25", "NIVEL 50", "BAYA HALLADA",
    "RACHA 7 DIAS", "VINCULO MAX", "FORMA FINAL", "EN FORMA" },
  { "LEVEL 10", "LEVEL 25", "LEVEL 50", "BERRY FOUND",
    "7 DAY STREAK", "MAX BOND", "FINAL FORM", "IN SHAPE" },
  { "NIVEAU 10", "NIVEAU 25", "NIVEAU 50", "BAIE TROUVEE",
    "SERIE 7 JOURS", "LIEN MAX", "FORME FINALE", "EN FORME" },
  { "LEVEL 10", "LEVEL 25", "LEVEL 50", "BEERE GEFUNDEN",
    "7 TAGE SERIE", "MAX BINDUNG", "ENDFORM", "FIT" },
  { "LIVELLO 10", "LIVELLO 25", "LIVELLO 50", "BACCA TROVATA",
    "SERIE 7 GIORNI", "LEGAME MAX", "FORMA FINALE", "IN FORMA" },
  { "NIVEL 10", "NIVEL 25", "NIVEL 50", "BAGA ACHADA",
    "SEQ 7 DIAS", "LACO MAX", "FORMA FINAL", "EM FORMA" },
  { "レベル 10", "レベル 25", "レベル 50", "きのみ はっけん",
    "7にち れんぞく", "なかよし MAX", "さいしゅうしんか", "げんきいっぱい" },
  { "레벨 10", "레벨 25", "레벨 50", "열매 발견",
    "연속 7일", "유대감 최대", "최종 진화", "건강함" },
};

const char *T(StrId id) { return STRINGS[gLang][id]; }
const char *medalName(int i)  { return MED_NAME[gLang][i]; }
const char *medalLabel(int i) { return MED_LBL[gLang][i]; }
const char *medalDesc(int i)  { return MED_DSC[gLang][i]; }

void loadLang() {
  Preferences p;
  p.begin("tamapoke", true);  // solo lectura
  uint8_t v = p.getUChar("lang", LANG_DEFAULT);
  p.end();
  gLang = (v < LANG_COUNT) ? (Lang)v : LANG_DEFAULT;
}

void setLang(Lang l) {
  if (l >= LANG_COUNT) return;
  gLang = l;
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("lang", (uint8_t)l);
  p.end();
}
