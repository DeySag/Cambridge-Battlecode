# v4 follow-up priorities — 22 October 2026

The current source is the frozen v4 failure-case update. Before changing it,
preserve its source ZIP/validation and compare on SDK 3.0.5/engine 1.0.3.
See the full [diagnosis](2026-10-22-ranked-review.md) and
[validation](../../tests/results/v4/README.md).

Implemented and measured: route plans survive turn-crossing computations;
banking does not invalidate completed work on a budget pause; stalled units
away from spawn can recall without a bag; defensive targeted stuns precede
movement; three-pick and boss drafts are adjusted. The release beats v3 61/98
and improves all three full-pool pressure fixture totals. Focused duelist
performance is unchanged, 9/12. Do not overwrite old v3 validation claims.

Next priorities:

1. Investigate residual island inactivity. Eight final recovery replays had
   all units earning gold, but a Heart still stood at full-health spawn
   positions for 306/319 living end turns in one control-pressure game.
   This is not automatically the same bank invalidation bug. Trace the true
   engine round and stored frame round, target, route/component, shared claims,
   transport availability and fuel. Printing itself perturbs fuel-sensitive runs.
2. `cross_obstacle` searches a crossing from the current walking component into
   the destination component. It does not plan a chain of intermediate components.
   `choose_group` assumes a dash is enough to consider disconnected targets.
   Reproduce multi-hop/no-affordable-crystal cases before selecting a graph
   planner or destination-feasibility penalty. The SDK uses north = (0,+1);
   do not introduce a false y-sign fix to `shift_up`.
3. Investigate control denial on `01hooks` and economic races on `00rows`,
   `00lost_world`, `01gold_rush`, `00gem_fields`, `00great_wall`. Preserve v4's
   gains elsewhere; its direct-v3 losses include quiet resource races.
4. Boss pressure and carrier exits remain opponent-dependent. Reproduce the
   937-collected/zero-deposited boss failure and late winning bags before
   applying generic smaller banking thresholds. Ready spell damage, stun
   timing and enemy kit-specific kill forecasts deserve separate tests.
5. Add more seeds and responsive opponents, then gather new arena replays.
   The repo covers 49 files, not all 40 current public-pool files by hash;
   eight external pool maps were absent from the supplied replays.

Evidence: `tests/results/v4/recovery-replay-review.json` and full local
`matches/v4-recovery-replay-review.json`, `matches/v4-ranked-analysis.json`,
`matches/v4-d-full-*`. Local matches remain Git-ignored. Fixture ZIPs and exact
source manifests are checked in under `tests/opponents/v4/` for reproducibility.
