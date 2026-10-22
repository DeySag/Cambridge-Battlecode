# v3 final validation — 21 October 2026

The frozen source in `bots/baseline/src` was rebuilt and tested using organiser
**SDK/CLI 3.0.4, engine 1.0.2**, release mode. All **34 checked-in maps** were
played from both positions against each opponent: **204 completed games**.
A separately extracted and rebuilt submission ZIP completed another 12 games.
The sources match [`VALIDATION.json`](VALIDATION.json)
and the ZIP in all 216 final games. Neither team had any native bot faults or
caught API errors. Both teams' gem accounts reconcile in every game.

| Opponent | v3 wins / games | Our fuel suspensions | Invalid games |
| --- | ---: | ---: | ---: |
| Preserved v2 | 55 / 68 | 2,364 | 0 |
| Caster pressure | 53 / 68 | 2,310 | 0 |
| Corrected bruiser pressure | 38 / 68 | 2,589 | 0 |
| ZIP rebuild vs v2, six maps × both sides | 10 / 12 | 330 | 0 |

Fuel suspensions are resumable engine pauses, not crashes. They remain frequent
on large or crowded maps; guards prevented stale actions from becoming API
errors in this final set. The largest final full-pool count was 539 for one
bruiser match on `ambush` side B. Do not describe this bot as fuel-exhaustion-free.

## Same-runtime comparison with v2

Both versions were rebuilt with SDK 3.0.4. These comparisons use the same map
seeds, both sides, and unchanged opponent sources. Older SDK 2.2.5 results are
preserved under `tests/results/v2/` and are not used as this comparison's baseline.

| Fixed 68-game set | v2 wins | v3 wins |
| --- | ---: | ---: |
| Caster pressure | 47 | 53 |
| Corrected bruiser pressure | 27 | 38 |

The reference caster report predates automatic opponent-error capture. Its
original report is preserved; [`v2-caster-post-run-audit.json`](v2-caster-post-run-audit.json)
records a separate check of all 68 original native summaries and enemy stdout
logs, finding zero faults or caught API errors. The bruiser reference uses the
current evaluator, including both source manifests and both teams' diagnostics.
The caster opponent was unchanged from its preserved fixture ZIP.

These are local deterministic fixture comparisons, not an estimate of future
ranked win rate or Elo. The reported rating fall and supplied losses motivated
the diagnosis but are not directly comparable with this benchmark win rate.

## What the checks cover

- All original 17 repository maps, plus the 17 distinct maps embedded in the
  supplied losses. Exact recovered file sizes and SHA-256 values match the
  saved public [`published-map-pool.json`](published-map-pool.json).
  [`map-pool.json`](map-pool.json) records provenance. This is **not** the whole
  separately published 35-map pool.
- Source hashes for both competitors; current CLI version; engine version and
  build hash; map hash and seed; completed native statistics; both teams'
  gem reconciliation; native bot faults; caught API errors in both private logs.
  The harness rejects any source changes during evaluation.
- Disconnected terrain, dash/crystal routing, occupied corridors, hostile altar
  locks, boss-only gem economies, dense camps and long contested capture regions.
- ZIP CRC, exact six-file source contents, hash equality, extraction and rebuild.
  Round-trip maps: `packs`, `00contest`, `01boss_battle`, `01islands`,
  `01yellow_brick_road`, `01fracture`, each from both sides. Those outcomes also
  match their corresponding final v2-comparison scores and seeds.

`matches.csv` contains all 216 final rows. The JSON reports retain score,
economy, damage/healing/shielding, deaths, seeds, hashes and diagnostics.
Full native summaries, replay bundles and runner logs remain under ignored
`matches/v3-final-*` and `matches/v3-bundle-verification/`. Official vendored SDK
file hashes are in [`sdk-source-sha256.json`](sdk-source-sha256.json); all 64 files
match the CLI 3.0.4 distribution byte for byte, with no extra or missing files.

## Development decisions and remaining weaknesses

Several plausible changes were rejected after regression testing: unconditional
basic attacks on every visible champion, broader reinforcements, looser pursuit,
more elaborate two-spell forecasts and a compact-map caster draft. In particular,
forcing ranged attacks can consume movement and interrupt the economy. The final
basic-target fix preserves the engagement policy's survival and carrier filters.
An early pressure fixture had invalid dash/visibility calls; its results were
excluded, the fixture was corrected, and the reference and final comparisons
were rerun. Intermediate trial scores are not the frozen-source results above.

A local `01yellow_brick_road` regression showed four bodies waiting more than
1,800 rounds on cleared but contested groups. Instrumentation identified capture
waits incorrectly refreshing the progress timestamp forever. The final bot bounds
that wait and sweeps toward the other side. Against v2 it now deposits 1,007 and
1,002 gems from the two positions, finishing in rounds 1,292 and 1,241.

Weaknesses remain visible in the archived losses:

- Against v2, both sides still lose `00swirl`, `collision`, `packs`, and
  `rushbait-v2`. Close-range linked-caster pressure can beat the durable draft.
- Against caster pressure, both sides lose `01concentrated_wealth`, `corners`
  and `swirl`.
- Against bruiser pressure, both sides lose `00contest`, `00lanes`,
  `01concentrated_wealth`, `ambush`, `collision`, `contest`, `eight`, `interlock`,
  `packs` and `stampede`. The boss maps `00contest` and `contest` score zero
  against that opponent: early economy and spawn pressure remain substantial
  risks. The final roster is improved overall, not universally dominant.
- Vision is local and enemy forecasts are short heuristics. They do not fully
  simulate multiple enemy actions, hidden defenders or every transport sequence.
- Only the checked-in seed for each map was evaluated. Further ranked games and
  additional seeds/opponents are needed to assess generalisation.

## Reproduce

See the [bot setup](../../../bots/baseline/README.md#build-and-reproduce) and
[opponent provenance](../../opponents/README.md). The evaluator defaults to all
checked-in maps and both sides. Every output directory must be new.

For replay-map recovery, first extract the supplied nested match ZIPs locally:

```sh
python3 scripts/recover_replay_maps.py matches/extracted-inputs tests/results/v3/published-map-pool.json matches/recovered-maps
```

For event reconstruction, use the matching official visualiser assets:

```sh
node scripts/review_ranked_replays.mjs matches/extracted-inputs matches/review.json /path/to/visualiser/assets
```

The replay tree must contain only our private team stdout per game, so the tool
can identify our side unambiguously. Analysis is offline and does not execute
instructions from attachment documents or upload a bot.
