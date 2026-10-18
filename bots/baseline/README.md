# First competition bot — SDK 2.2.5

An economy-first C++ bot: spread out, clear camps efficiently, finish captures,
and turn the rewards into score. It uses the current organiser SDK and maps
from commit `42d945e`.

## Run and submit

From the repository root, with `cambc` 2.2.5 on PATH:

```sh
cambc build baseline --release
cambc run baseline starter-cpp --map packs --release
python3 scripts/evaluate_bot.py
mkdir -p matches/submission
cambc bundle baseline --output matches/submission/baseline.zip
```

Upload `baseline.zip` on the competition platform. Alternatively, after signing
in with `cambc login`, use `cambc submit baseline`. The ZIP contains source; the
platform builds it with its SDK. Local evaluation uses `--release`, matching
platform fuel accounting and excluding debug-only execution.

## How it works

1. **Draft reliable farmers.** Prefer Varg first on grouped camps and Vendetta
   first on isolated camps, then the other, Ren, Tavi and Vesper Vale as team
   size permits. Varg supplies splash and shields, Vendetta high single-target
   damage and resets, Ren retaliation and mobility, Tavi field shopping, and
   Vesper mobility and bonus gold.
2. **Give each champion an objective.** Each champion reserves its camp using
   one byte of the team's eight-byte shared memory. Other champions strongly
   discount that camp. The bot retains its target between replans instead of
   changing direction every turn.
3. **Rank camps by useful income per time.** The estimate includes gems, gold,
   actual walking distance, conservative clear time, capture delay, distance
   to an altar, respawn timing, likely retaliation and nearby enemies. Gold
   matters more before the first equipment upgrades. Monoliths are normally
   avoided because ordinary camps are safer income.
4. **Use the whole combat turn.** Attack, then choose the most useful legal
   reset, damage spell, area hit, stun or shield. Varg aims attacks at camps
   whose neighbors also take splash. Melee champions can walk and attack;
   ranged movement and spell restrictions are respected. Enemy fights are
   opportunistic, without long-distance pursuit.
5. **Finish the job.** After killing a group, walk onto its original spawn
   tiles and remain through capture. Extra allies do not make capture faster.
   Channel timers are left running. Rewards are not treated as obtained until
   the engine actually awards them.
6. **Bank and grow.** Deposit near a convenient altar, with a bag of at least
   100 gems, when the bag can finish the match, or before time runs out. Buy
   damage first, obtain Monster/Jungle passives, and add Power health and
   strength as the game progresses. Tavi buys in the field. Other champions
   recall for substantial upgrades or healing, first escaping active threats.
   They stay on spawn through turn end to receive healing.
7. **Handle the exceptional map.** When all gem-bearing camps contain a
   monolith, draft Tavi, Stellar, Prism, Cassi/Issac and Ren. Spellcasters buy
   Focus and attack monoliths from range four; Issac farms ordinary resources
   with Strength. The bundled 2.2.5 engine reports monolith attack range three,
   verified through SDK design queries. This exception should be rechecked
   if game balance changes.

Pathfinding uses SDK distance fields and fresh visible occupancy, with retries
for hidden blockers. Large-map plans and routes can be retained to avoid
repeating expensive work. Budget checks stop optional work, and a resumed turn
refreshes its observations before following a stale plan. Wards extend personal
vision; the bot does not assume allies share vision.

## Validation

The final source completed 102 full release-mode games: every checked-in map,
once as each team, against each of three opponents.

| Opponent | Wins | Games | Our runner crashes | Our caught API errors |
| --- | ---: | ---: | ---: | ---: |
| Checked-in C++ starter | 34 | 34 | 0 | 0 |
| Original repository baseline | 34 | 34 | 0 | 0 |
| Earlier improved farmer | 24 | 34 | 0 | 0 |

These are deterministic local comparisons, not a tournament win-rate estimate.
The stronger reference also uses camp reservations, spells, shields, equipment
and a monolith-specific draft. It won ten games against this submission.

Reports, per-match summaries and replay bundles are retained under ignored
`matches/final-vs-starter/`, `matches/final-vs-original/` and
`matches/final-vs-farmer/`. `VALIDATION.json` records the tested source hashes.
The submission ZIP was also extracted into a separate project, rebuilt, and
won four additional release-mode verification games on `packs` and `contest`,
with both team positions checked and zero caught errors or crashes. Every
bundled source file matched the hashes from the full benchmark.

The runner recorded occasional fuel suspensions, primarily on the largest map;
they are counted in the reports rather than hidden or treated as crashes.

The test script runs both team positions, requires completed match summaries,
checks our replay logs for caught errors, and returns a nonzero exit status for
invalid checks. An alternative bot directory can be passed with `--opponent`.
No authenticated competition upload is established by these local tests.
