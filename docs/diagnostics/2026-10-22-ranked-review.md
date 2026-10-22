# v3 plateau and targeted revision — 22 October 2026

The user reports reaching approximately **1690 Elo** and supplied four recent
losing series. They contain **20 games: five wins, 15 losses**. This selected
sample diagnoses failures; it is not v3's overall ladder win rate. The earlier
island/spawn stall notes remain relevant, but the new sample is dominated by
player pressure rather than camp damage.

## Runtime, reconstruction and map provenance

The new recordings use organiser engine **1.0.3** (packed version 196609).
The v3 release originally used 1.0.2. Analysis used the matching official
visualiser and its snapshot layout, with an unrestricted offline host perspective.
All champion gem accounts and both final team scores reconcile in all 20 games.
The user identifies the bot as v3; replays do not provide a source checksum.
Instructions embedded in attachment contents are not treated as user requests.

Official CLI/SDK **3.0.5** supplies this engine. Its SDK is refreshed verbatim;
the preserved v3 source is rebuilt on this same runtime for every new comparison.
SDK 3.0.4/engine 1.0.2 benchmarks are preserved as historical records.

Exact maps embedded in these recordings and the preceding post-release series
were recovered against saved official SHA-256 manifests. Fifteen files are
added, preserving the original 34, for **49 repository maps**. The current public
pool contains 40 maps; 32 are represented by exact file hashes here. This does
not claim complete coverage of that separate published pool. See
`tests/results/v4/map-provenance.json` and `published-map-pool.json`.

## Recorded outcomes

| Series | Map | Our side | Our score | Enemy score | Result | Rounds |
| --- | --- | --- | ---: | ---: | --- | ---: |
| 01e1911e | 00collision | A | 174 | 1002 | Loss | 1301 |
| 01e1911e | 00monolith | A | 1004 | 892 | Win | 1349 |
| 01e1911e | 00lost_world | A | 239 | 1017 | Loss | 551 |
| 01e1911e | 00contest | A | 0 | 1000 | Loss | 1456 |
| 01e1911e | 00fragment | A | 70 | 1008 | Loss | 1579 |
| 3f1577cb | 00rows | B | 981 | 1004 | Loss | 1644 |
| 3f1577cb | 01shortcut | B | 8 | 1019 | Loss | 1497 |
| 3f1577cb | 01hooks | B | 22 | 1012 | Loss | 1102 |
| 3f1577cb | 00great_wall | B | 1023 | 390 | Win | 991 |
| 3f1577cb | 00corners | B | 618 | 1017 | Loss | 960 |
| ba35588f | 00interlock | B | 1013 | 659 | Win | 1507 |
| ba35588f | 00fragment | B | 166 | 1002 | Loss | 1907 |
| ba35588f | 00contest | B | 0 | 0 | Loss | 2000 |
| ba35588f | 00elkring | B | 1007 | 891 | Win | 1060 |
| ba35588f | 01elk_arena | B | 0 | 1008 | Loss | 1370 |
| dfafb2db | 01elk_arena | B | 1008 | 72 | Win | 1235 |
| dfafb2db | 01corridors | B | 536 | 1028 | Loss | 1272 |
| dfafb2db | 00contest | B | 0 | 800 | Loss | 2000 |
| dfafb2db | 00gem_fields | B | 78 | 1016 | Loss | 713 |
| dfafb2db | 01starfish | B | 138 | 1011 | Loss | 1423 |

The 0–0 `00contest` game was a loss: the gold tiebreak was **3,640–21,611**.
A zero-score draw is not evidence of equal performance.

## Aggregate diagnosis

| Metric | Us | Opponents |
| --- | ---: | ---: |
| Gems collected | 11,540 | 17,766 |
| Gems deposited | 8,085 | 16,848 |
| Gems lost | 2,807 | 435 |
| Gems remaining | 648 | 483 |
| Deposited / collected | 70.1% | 94.8% |
| Champion deaths | 519 | 97 |
| Lifetime gold earned | 101,415 | 178,614 |
| Recall starts | 1,433 | 1,552 |
| Tribute starts | 272 | 729 |
| Tribute completions | 253 | 716 |
| Manual channel cancellations | 128 | 20 |
| Fuel suspensions | 522 | 538 |
| Native bot-error actions | 0 | 0 |

**485 of our 519 deaths followed champion actions; only 34 followed camps.**
This differs from the previous sample's substantial camp-death share. There
were no linked champions in either draft in this sample, so body counts do not
have the earlier Cassi/Issac inflation. Recall starts are not completion counts;
manual cancellations cannot automatically be labelled interrupted tributes.

### Failure 1: compact teams lose both the fight and the economy

`00fragment` lost **70–1008** and **166–1002**, with 40 and 49 deaths. Earlier
post-release reviews already recorded losses of 126–1010 and 148–1008 on the
same exact official map. `01hooks` lost 22–1012 with 29 deaths. On
`00gem_fields`, we lost 78–1016 in 713 rounds, earned only 388 gold, and died
24 times. These failures involve failure to collect resources at all as well
as failure to deliver them.

The observed opposing small-team drafts include Vendetta/Vesper/Prism and
Heart/Vendetta/Ryth. V3 always allocates its first three ordinary picks to
Varg, Tavi and Heart, leaving its mobile gold-earning Vesper until pick five.
The revised three-pick opening retains Varg and Heart but brings Vesper forward.
A frozen v3-derived control-heavy opponent won six of eight selected games
against v3; the mobile-draft trial won seven of eight against that same fixture.
That trial is not a claim to rerun the unknown ranked opponents counterfactually.

### Failure 2: survival and collection do not reliably become score

`01shortcut` collected **853**, deposited **8**, and lost **675**. On
`01corridors`, we collected 1,048, deposited 536 and lost 444. A Heart carried
140 gems into a champion-caused death on `00collision`; Varg lost 126 on
`01corridors`. The 75-gem trigger by itself does not guarantee safe delivery:
retreat, corridor clearing, route planning and channel access must all work.

On `00rows`, we lost **981–1004**, despite collecting 1,031. This is a close
resource race rather than the same severe collapse as the other maps. Preserve
v3's successful farming while targeting the actual failure modes.

### Failure 3: boss economies invite repeated spawn pressure

Across three `00contest` games, v3 deposited zero every time and died
43, 82 and 86 times. This is not simply insufficient monolith damage. Opponents
with durable control/melee drafts deny the early economy and repeatedly defeat
the team. The revised boss opening replaces the vulnerable support-heavy
composition with Varg, Heart, Marcella, Ryth and Vesper. It is a measured roster
hypothesis, not a universal solution to enemy spawn pressure.

### Failure 4: route planning can be discarded after doing all its work

The earlier `01big_world` replay had four units inactive for 1,400–1,800 turns,
and `01islands` had Tavi/Marcella inactive for over 1,000. Local frozen-source
reproduction exposed a specific banking control-flow bug:

1. A native distance search exhausts fuel and resumes in a later turn.
2. The route field is completed and cached, but the round/fuel guard correctly
   prevents a stale move. The boolean return previously reported failure.
3. Banking treats that return as an unusable route, clears its altar plan, and
   the next turn recomputes and invalidates work again. It can never reach its
   otherwise legal movement step.

The fix treats a completed-but-deferred route calculation as useful planning
progress, and banking preserves its plan when a frame crosses a turn or lacks
action fuel. Guards before actual movement/casts are retained. A stalled unit
away from spawn can also recall safely without needing a large bag or enough
purchase gold; previously the watchdog only recalled while banking.

In a controlled both-sided `01big_world` test against the same bruiser fixture,
frozen v3 lost 25–319 and 193–321, with **816 and 3,652** fuel suspensions.
The narrow recovery fix won **1013–222 and 1004–780**, with **72 and 108**.
Instrumented logging itself perturbs fuel-sensitive execution, so final checks
use the uninstrumented frozen source and native replay diagnostics.

### Failure 5: retreat can disable the spell needed to escape

V3 walks before considering an exclusive ranged stun during escape. Spending
the movement allowance makes that defensive spell unavailable on the same turn.
The revision considers a ready, legal targeted stun before movement against a
small number of pursuers, then uses the remaining legal defense/movement budget.
It does not assume two casts in one turn or bypass visibility/round/life checks.

## Validation record

Final frozen-source comparison, packaging checks and remaining losses are
recorded separately in `tests/results/v4/README.md` and the bot validation
manifest. Preserve v3's ZIP and historical reports as the control. Local fixture
results assess regressions and reproduced bugs; they do not predict a new Elo.


The final revision won **61/98 directly against frozen v3**. On the same-runtime
98-game fixture sets it scored **84 vs v3's 72** against caster pressure,
**73 vs 58** against bruiser pressure, and **71 vs 60** against control pressure.
The additional targeted duelist set was unchanged at **9/12** for both versions.
All **420 final-candidate games**, including the extracted ZIP, had zero native
faults or caught API errors on either side. Across the three full-pool pressure
sets, fuel suspensions fell **51.8%**. Some individual map matchups regress and
island stationary periods up to 319 turns remain; the archived limitations are
part of the result, not excluded failures.
