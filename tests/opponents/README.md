# Reproducible pressure opponents

These source ZIPs are test fixtures derived from the preserved Sprint 1 bot.
Use SDK/CLI 2.2.5 and the checked-in maps. They are not ranked opponents, and
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
