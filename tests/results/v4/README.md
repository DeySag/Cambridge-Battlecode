# v4 final validation — 22 October 2026

The exact frozen source in `bots/baseline/src` was rebuilt with official
**SDK/CLI 3.0.5, engine 1.0.3**, release mode. Four full comparisons covered
**49 repository maps × both sides = 98 games each**, or **392 games**.
The extracted submission ZIP completed 16 additional matches; a focused duelist
check completed another 12. All **420 final-candidate games** had zero native bot
faults and zero caught API errors on **either** team. Both gem accounts reconcile
in every game. Sources match [`VALIDATION.json`](../../../bots/baseline/VALIDATION.json)
and the source-only ZIP throughout; no source changed during a run.

## Full-pool results and same-runtime controls

| Opponent | Frozen v3 wins | v4 wins | Games per version |
| --- | ---: | ---: | ---: |
| Preserved v3 | — | 61 | 98 |
| Caster pressure | 72 | 84 | 98 |
| Corrected bruiser pressure | 58 | 73 | 98 |
| Control-heavy pressure | 60 | 71 | 98 |

The three comparison fixtures, map hashes, seeds, engine build hashes and
opponent source hashes match between v3 and v4 runs. V3 was rebuilt from its
preserved ZIP on **1.0.3**, not compared with its historical 1.0.2 results.
Reference evaluation completed another 294 full-pool games plus 12 duelist games.
All reference games were also fault/API-error free on both teams.

These are fixed-seed local source-fixture comparisons. They do not predict Elo,
ranked win rate or a replay counterfactual against unknown opponent source.
This is a combined revision comparison; it does not isolate every change's
contribution. A separate narrow route-recovery reproduction is archived below.

## Fuel, package and archetype checks

| Same 98-game fixture set | v3 fuel suspensions | v4 fuel suspensions |
| --- | ---: | ---: |
| Caster | 5,379 | 3,571 |
| Bruiser | 7,764 | 3,170 |
| Control-heavy | 7,949 | 3,428 |

Across these three matched sets, suspensions fell from **21,092 to 10,169**
(51.8% fewer). These are resumable pauses, not crashes. They remain significant;
no claim of fuel-exhaustion-free operation is made. The separate direct-v3 set
had 3,645 v4 suspensions, and the ZIP rebuild set had 416.

The ZIP CRC and all six source hashes were checked. It was extracted to a
separate bot directory and rebuilt. Its 16 completed matches reproduced the
corresponding full-pool **scores, map seeds and round counts exactly**, winning
11/16 against v3. Maps: `01big_world`, `01islands`, `00fragment`, `00contest`,
`01yellow_brick_road`, `01hooks`, `00lost_world`, `01boss_battle`, both sides.

The additional Vendetta/Vesper/Prism duelist fixture targets an opening seen in
the latest losses. V3 and v4 both won **9/12** on the same six-map, both-sided
set, with zero faults/API errors on either team. The exact source fixture,
provenance and both reports are retained. This is a focused check, not a fifth
full-pool comparison or evidence of improvement against that archetype.

## Reproduced failures and targeted trial evidence

- `v3-recovery-reference.json` and `recovery-fix-trial.json` record the
  uninstrumented narrow planning/recovery fix. On `01big_world` versus the same
  bruiser source, v3 lost both sides with 816/3,652 fuel events. The fix won both
  above 1,000 gems with 72/108. This isolates route-plan preservation and safe
  recall from later draft changes.
- `v3-control-focus.json` and `mobile-draft-trial.json` retain the independently
  frozen control-heavy fixture and the later eight-game mobile opening trial.
  V3 won 2/8 on those four compact maps; the trial won 7/8. The final full-pool
  results, not this selected set, are the main regression evidence.
- `recovery-replay-review.json` reconstructs eight uninstrumented final-candidate
  games on `01big_world` and `01islands` versus bruiser/control. Seven were wins,
  and every champion earned gold. Full-team spawn inactivity did not recur in
  those tests. However, individual island stationary runs still reached **319
  turns**; the old 1,000–1,800-turn failures are not a universal stall-freedom proof.
  Counts include valid channels, combat and capture waits, so activity requires
  interpretation. The full local reconstruction is retained under `matches/`.

## Coverage and archived evidence

All original 34 files were retained. Fifteen exact maps recovered from uploaded
replays were added, with complete file SHA-256 and size verified against public
organiser manifests. There are **49 checked-in maps**, including legacy files;
32 of the current separately published 40-map pool are represented by exact
hash. Eight current published maps were not present in these uploads. This
validation covers every repository map, not the entire external pool. Provenance
and the retrieved current manifest are archived alongside these results.

`matches.csv` has **420 final rows**. JSON reports retain scores, source manifests,
map/engine hashes, seeds, gem accounting, economy, damage/healing/shielding,
deaths and diagnostics. The 64 vendored SDK files and CMake file match a fresh
official CLI 3.0.5 project byte for byte. SDK hashes are archived here.

Full native summaries, runner logs and replay bundles remain under ignored
`matches/v4-d-full-*`, `matches/v4-bundle-verification`, `matches/v4-duelist-check`
and `matches/v4-reference-*`. The current source ZIP checksum is in
[`VALIDATION.json`](../../../bots/baseline/VALIDATION.json). The immutable v3
release manifest is now preserved at [`../v3/VALIDATION.json`](../v3/VALIDATION.json).

## Remaining losses and scope limits

- Against v3, both positions still lose `00gem_fields`, `00great_wall`,
  `00stampede`, `01gold_rush`, `interlock` and `rows`. Economic races and some
  layouts remain stronger for the old policy. Aggregate improvement does not
  mean every matchup improved.
- Against caster pressure, both sides still lose `swirl`.
- Against bruiser pressure, both sides lose `00lost_world`, `00monolith`,
  `01concentrated_wealth`, `contest` and `interlock`.
- Against control pressure, both sides lose `00elkring`, `00rows` and `01hooks`.
  The actual ranked hooks losses are not declared solved.
- Long individual island inactivity remains. Single-crossing transport planning,
  no-objective behavior at spawn and interactions with occupancy/claims require
  further instrumented reproduction. Extra diagnostic printing changes fuel use
  and can perturb outcomes; claims above use uninstrumented final-source games.
- Boss pressure is improved in some local cases, not universally solved. Hidden
  enemies, overlapping casts, safe carrier exits and late delivery still need
  new live evidence. Only each checked-in seed was evaluated.

## Reproduce

Use the [bot instructions](../../../bots/baseline/README.md#build-and-reproduce),
[fixture provenance](../../opponents/README.md) and the current evaluator. Every
output directory must be new. To rerun the controls, use the extracted v3 bot as
candidate against the same three opponents on the same SDK. Keep v3, fixture
sources and map seeds frozen while changing the candidate. Replays can be
reconstructed with `scripts/review_ranked_replays.mjs` and matching 1.0.3 official
visualiser assets; map extraction uses `scripts/recover_replay_maps.py` and the
saved published manifest.
