# v2 final validation — 19 October 2026

The same frozen v2 source completed the full 17-map pool from both team
positions against three opponents in release mode, using SDK/CLI 2.2.5.
All source checksums match `bots/baseline/VALIDATION.json` and the submission ZIP.

| Opponent | Wins | Games | Our crashes | Our caught API errors | Our fuel suspensions |
| --- | ---: | ---: | ---: | ---: | ---: |
| Preserved Sprint 1 submission | 30 | 34 | 0 | 0 | 29 |
| Caster pressure | 26 | 34 | 0 | 0 | 31 |
| Melee pressure | 24 | 34 | 0 | 0 | 26 |
| ZIP extraction/rebuild vs Sprint 1 | 4 | 4 | 0 | 0 | 0 |

The opponents also had zero crashes and zero caught API errors in the 102
full-pool games. Fuel suspensions are normal engine pauses, counted separately
from crashes; they remain a performance limitation. Raw statistics, runner logs
and replays are retained under ignored `matches/v2-final-*` and
`matches/v2-bundle-verification/`.

`matches.csv` contains one row for each of the 106 final games. The JSON reports
retain per-game scores, seeds, gem conversion/loss, deaths, diagnostics and
candidate source hashes. Collected gems reconcile with deposited, lost and
remaining bags for both teams in every game.

## Pressure comparison with Sprint 1

The initial comparison set used collision, packs, interlock, pilgrimage, swirl,
stampede, contest and eight, each from both sides. Both pressure opponents and
Sprint 1 were frozen before comparing candidates.

| Fixed 16-game set | Sprint 1 wins | Final v2 wins |
| --- | ---: | ---: |
| Caster pressure | 0 / 16 | 14 / 16 |
| Melee pressure | 0 / 16 | 11 / 16 |

The Sprint 1 reports for those sets are archived alongside the final reports.
They use the same map seeds and release-mode fuel rules. This is a combined
strategy comparison: v2 changes the draft, equipment, engagements, delivery and
retreat behavior. It does not isolate one change's contribution.

## What changed and why

The initial safety-only trial improved delivered score against pressure but
still lost every game in that test set and regressed against Sprint 1. The
selected approach pairs safer delivery with ranged damage and local ally support.
Power equipment contributes to both farming and player combat. The ordinary
draft now adds Stellar and the Cassi/Issac pair alongside Varg; larger teams
also select Ryth and Tavi.

During development, tests exposed repeated attacks on enemies healing on spawn,
especially on rushbait-v2. The final version excludes those targets. A separate
test exposed an escape frame resuming after a fuel pause with its champion dead;
movement/cast boundaries now check the round and current action availability.
The affected scenarios and the full pool were rerun on the final frozen source.

## Limits and remaining weaknesses

- These are local, deterministic comparisons against hand-written fixtures,
  not a ranked win-rate or Elo forecast. Only the checked-in map seed for each
  map was exercised; the seed is recorded in every row.
- v2 still lost both positions on **expanse** and **rows** against Sprint 1.
  Quiet farming races remain weaker than its performance under pressure.
- Stampede lost both sides against the melee-pressure fixture, despite winning
  both sides against Sprint 1 and caster pressure. Contest and some larger
  maps also remain opponent-dependent.
- Cassi/Issac create an extra unit but share a death, so their safety remains
  coupled. Local support is based only on units visible to the acting champion.
- Pressure fixtures can overcommit and attack enemies on spawn. They expose
  useful weaknesses but are not replacements for new arena matches.

## Reproduce

From a project using SDK 2.2.5, extract the preserved source ZIPs into bot
directories. See [opponent setup](../../opponents/README.md). Then run:

```sh
python3 scripts/evaluate_bot.py --candidate baseline --opponent sprint1 --output matches/new-v2-v1
python3 scripts/evaluate_bot.py --candidate baseline --opponent pressure-caster --output matches/new-v2-caster
python3 scripts/evaluate_bot.py --candidate baseline --opponent pressure-melee --output matches/new-v2-melee
```

Each output directory must be new. The harness builds release binaries, checks
completion and native statistics, reconciles gem balances, inspects our replay
logs and returns a nonzero exit status for invalid checks. Losses are recorded
as valid completed games rather than hidden as test failures.
