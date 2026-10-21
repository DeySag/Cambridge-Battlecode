# v3 — durable economy and contested objectives

C++ bot for organiser **SDK/CLI 3.0.4, engine 1.0.2**. This replaces v2 after
reviewing all 20 games in the four supplied losing series. See the
[replay diagnosis](../../docs/diagnostics/2026-10-21-ranked-review.md) and
[final measured results](../../tests/results/v3/README.md). The organiser's
[patch announcements](../../docs/cambc2026a/PATCH-NOTES.md) and earlier v1/v2
source ZIPs remain preserved.

## Strategy

- **Durable draft.** Ordinary maps open Varg then Tavi on dense camp layouts,
  reversing those two on sparse layouts; add Heart of Ruin, Marcella and Vesper
  as the pick limit permits. Boss-only gem maps choose Varg, Tavi, Willow,
  Heart and Marcella. This avoids linked deaths and builds a shielded frontline
  with ranged support. Monolith range is queried from the current engine;
  range-four spells no longer give safe boss farming.
- **Build an economy before expensive fights.** Complete five Jungle damage
  shards, then five Durability Health, then five Monster damage; later complete
  Power damage blocks interleaved with Bruiser Health. Heart buys Focus, basic
  attackers buy Strength. Tavi shops in the field. Jungle and Durability passives
  reduce incoming damage; their benefits are not assumed to stack repeatedly.
  Recall for healing or a meaningful purchase rather than each small reward.
- **Farm and move with a purpose.** Rank camps by rewards, terrain distance,
  clearing time, health cost, delivery cost, claims and remembered deaths.
  Reserve groups in one shared byte per body. Cache routes and spread expensive
  objective scoring across turns. Static routes can approach a corridor monster;
  a blocked step can switch to clearing its group. Temporary unit blockages
  permit detours. Disconnected components require a demonstrated legal dash or
  a range-three teleport crystal crossing; portal planning is nonrecursive.
- **Bound capture waits.** Waiting on a cleared camp is productive only briefly.
  An unseen enemy anywhere in a long connected group can prevent capture. After
  the capture window plus a margin, sweep toward the far side and fight its
  defender. A separate watchdog discards stale objectives and route plans after
  16 rounds without movement, gem change or combat progress.
- **Take profitable nearby fights.** Compare visible friendly and hostile
  health and damage before short pursuits. Use damage, stuns, splash, shields,
  attack resets, heals and damaging dash landings. Basic attacks can select a
  reachable enemy when the preferred spell target is outside basic range, while
  retaining the engagement policy's carrier and survival filters. Exclude enemy
  spawn targets. Ranged units may reset their attack allowance after walking.
- **Survive retaliation and burst.** Include neighbouring visible awake camps,
  not just the selected group. Mark groups awakened by attacks and splash.
  Use shields before retaliation, refresh health after attacks, and evaluate
  escape tiles against camp damage, enemy abilities, stuns and cooldowns.
  Stay on allied spawn for recovery while still using available attacks/spells.
- **Turn gems into score.** Usually bank at 75, or at 30 near an altar; exposed
  smaller bags, a winning bag and the time limit trigger earlier delivery.
  Compare actual altar routes, visible danger, locks and recent death sites.
  Remove hostile locks, use attack resets where useful, and finish survivable
  tribute channels because ordinary damage does not interrupt them.
- **Respect fuel and visibility.** Live state is read only for visible units.
  Life, round identity, action allowance and fuel are checked before actions.
  A resumed computation cannot silently assume it is still the same turn.

## Build and reproduce

Install organiser CLI 3.0.4 and use the checked-in SDK:

```sh
cambc build baseline --release
mkdir -p bots/v2 bots/pressure-caster bots/bruiser-pressure
unzip submissions/v2-sprint-2-bot-update.zip -d bots/v2
unzip tests/opponents/pressure-caster.zip -d bots/pressure-caster
unzip tests/opponents/v3/bruiser-pressure.zip -d bots/bruiser-pressure
python3 scripts/evaluate_bot.py --candidate baseline --opponent v2 --output matches/new-v3-v2
python3 scripts/evaluate_bot.py --candidate baseline --opponent pressure-caster --output matches/new-v3-caster
python3 scripts/evaluate_bot.py --candidate baseline --opponent bruiser-pressure --output matches/new-v3-bruiser
```

Each output directory must be new. By default the evaluator runs both positions
on all **34 checked-in maps**: the original 17 plus 17 recovered from the uploaded
replays and verified against official SHA-256 values. This covers every repo map,
not the entire separately published 35-map pool. The evaluator rejects incomplete
runs and API errors/crashes on either team, reconciles both gem accounts, and
records source hashes, runtime/map hashes, seed, economy, deaths and fuel events.

## Validation and limits

[`VALIDATION.json`](VALIDATION.json) ties the final source hashes to benchmark
reports and the source-only ZIP. Archived reports and CSVs live under
[`tests/results/v3`](../../tests/results/v3/README.md); full local replays and
statistics are under ignored `matches/v3-final-*`. ZIP extraction, rebuild and
separate match checks verify the deliverable rather than only the working tree.

These are deterministic local comparisons against independent hand-written
fixtures. They do not predict ranked win rate or Elo. Enemy assessment is a
short heuristic with partial vision, not a full combat search. Hidden defenders,
multiple available enemy spells over several turns, failed transport, long
contests and some close-range matchups remain risks. Fuel suspensions remain
measured separately from bot faults. New arena replays should guide further
changes; final validation reports document the losses as well as wins.
