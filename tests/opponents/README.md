# Reproducible pressure opponents

These source ZIPs are test fixtures derived from the preserved Sprint 1 bot.
The v2 results used SDK/CLI 2.2.5. For current v3 comparisons, rebuild these
unchanged sources with SDK/CLI 3.0.4. They are not ranked opponents, and
performance against them does not predict tournament Elo.

- `pressure-caster.zip`: drafts Stellar, Cassi/Issac, Varg, Ryth and Tavi;
  buys Power damage equipment; pursues nearby visible farmers and gem carriers
  while healthy. Ground spells target champions. It farms when no suitable
  enemy is visible and attempts delivery at larger bag sizes.
- `pressure-melee.zip`: drafts Varg, Ren, Tavi, Stellar and Vesper Vale;
  uses Focus on Ren/Tavi and a lower health threshold for pursuing opponents.
  This tests a different mixture of close combat, retaliation and stuns.

Both retain the original boss draft. Their simple pursuit can overcommit,
including attacking an enemy healing on spawn. v2 explicitly avoids that
failure. These fixtures complement the passive farmers used during Sprint 1.

To reconstruct a fixture inside an existing cambc project:

```sh
mkdir -p bots/pressure-caster
unzip tests/opponents/pressure-caster.zip -d bots/pressure-caster
cambc build pressure-caster --release
python3 scripts/evaluate_bot.py --candidate baseline --opponent pressure-caster
```

Extract `submissions/v1-sprint-1-bot-update.zip` into `bots/sprint1` to reproduce
the preserved baseline as another opponent. The evaluator output directory
must be new for each run. Reports record candidate source hashes, map seeds
and diagnostics; archived final reports live under `tests/results/v2/`.


## Patched-runtime bruiser fixture

[`v3/bruiser-pressure.zip`](v3/bruiser-pressure.zip) is an independent Sprint 1
derivative with a durable physical draft, Jungle opening, Bruiser Health,
aggressive pursuit and Heart of Ruin dash damage. The complete source hashes
and changes are in [`v3/PROVENANCE.json`](v3/PROVENANCE.json). Build it with
SDK/CLI 3.0.4. It retains deliberately simple strategy, can overcommit and does
not represent a ranked opponent. It complements the ranged pressure fixture.

Use the same extraction commands above with `v3/bruiser-pressure.zip` and a new
`bots/bruiser-pressure` directory. Extract the frozen v2 comparison opponent
from `submissions/v2-sprint-2-bot-update.zip`. Current final reports and exact
source checksums are under `tests/results/v3/`. Both teams' caught API errors
and native bot faults are checked by the current evaluator. An early bruiser
fixture contained invalid dash/visibility calls; it was fixed and those earlier
comparisons were excluded.


## v4 control-pressure fixture

[`v4/control-pressure.zip`](v4/control-pressure.zip) is derived from the frozen
v3 submission, independently of the developing v4 source. It drafts Heart,
Vendetta, Ryth, Vesper and Ren and buys Power damage from the opening with three
Bruiser Health per five damage shards. The other v3 behavior remains frozen,
including its limitations. It exercises the control-heavy physical openings seen
in the new losses; it does not reconstruct an unknown ranked opponent.
[`v4/PROVENANCE.json`](v4/PROVENANCE.json) records the exact source and ZIP hashes.
Use SDK/CLI 3.0.5. Reference v3 and final v4 both completed the full 49-map pool
from both sides against the same fixture; see `tests/results/v4/`.


[`v4/duelist-pressure.zip`](v4/duelist-pressure.zip) is the corrected bruiser
fixture with a Vendetta/Vesper/Prism/Ryth/Varg opening and Focus equipment on
Prism. Its exact hashes and limited 12-game check scope are in
[`v4/duelist-PROVENANCE.json`](v4/duelist-PROVENANCE.json). Other corrected
bruiser behavior is retained. Both v3 and v4 scored 9/12 in that focused check.
