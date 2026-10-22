# First v3 ranked win — 21 October 2026

The user identifies this as the first winning series on v3 and describes the
opponent as comparatively easy. The supplied series is a verified **5–0** sweep,
with our team on side B throughout. The replay does not carry a compiled source
checksum; source attribution relies on the user's statement and the matching
draft, not a claim of binary identification. One opponent is not evidence of a
new ladder win rate or Elo expectation.

All five replays were advanced with matching organiser engine **v1_0_2** using
an unrestricted offline host perspective. Individual gem balances and team
scores reconcile in every game. The compact companion JSON retains replay
hashes, verified map names/hashes, drafts, champion actions, stationary runs and
team aggregates. Full local reconstruction is `matches/v3-first-win-analysis.json`.

| Map | Our score | Enemy score | Rounds |
| --- | ---: | ---: | ---: |
| 01islands | 1,032 | 886 | 1,384 |
| 00pilgrimage | 1,008 | 504 | 1,989 |
| 01elk_arena | 1,008 | 0 | 1,299 |
| 01gold_rush | 957 | 165 | 2,000 |
| 01concentrated_wealth | 1,026 | 200 | 1,139 |

Map files embedded in the recordings match exact SHA-256 entries in the saved
public organiser manifest. `01elk_arena` and `01gold_rush` were absent from the
34-map v3 release test pool. They were recovered locally under
`matches/v3-first-win-maps/` for subsequent regression tests; this observation
does not retroactively expand the published release validation. The terrain-only
fingerprint identified `00pilgrimage` as the legacy `pilgrimage` layout; its
complete embedded-file hash establishes the public `00pilgrimage` name here.

## What worked

Across the series we collected **5,349** gems and deposited **5,031 (94.1%)**,
losing 163 and ending with 155 carried. Opponents collected 2,074, deposited
1,755 (84.6%), lost 112 and ended with 207. Our lifetime earned gold was
**51,226 versus 19,470**. All 209 completed tribute channels are accounted for.
There were zero native faults and zero caught API errors in our private logs.
These are encouraging economy and delivery outcomes against this opponent;
they cannot isolate v3's contribution from opponent strength or map selection.

The opposing roster was Varg, Cassi/Issac, Stellar, Ryth and Ren where five picks
were available; it used the first three on `01gold_rush`. We used the expected
Varg, Tavi, Heart, Marcella, Vesper roster, reduced to the first three on that
map. We won both small-team and large-team layouts. The opponent collected zero
gems on `01elk_arena`, so that particular result is weak evidence of robustness
against a functioning, stronger economy. `01islands` was much closer at 1,032–886.

## Failures the result conceals

### Two champions stopped contributing on 01islands

Tavi remained at **(25,47)** for **1,266 consecutive living end turns**, beginning
at round 119. Marcella remained at **(25,48)** for **1,121**, beginning at round
264. Both collected and deposited **zero gems**, earned only 80 gold, and
survived the game. Their maximum runs without counted productive actions were
1,265 and 1,123 turns, respectively. This is not ordinary tribute, capture or
healing downtime. Varg, Heart and Vesper delivered all 1,032 gems.

Each stalled unit had completed one teleport-crystal channel. Tavi held 10 gold
and Marcella 40 while stationary. Neither has a dash; field consumable purchases
are prohibited even for Tavi. The current source skips disconnected targets
without transport, has no recall fallback when objective selection yields no
target and the bag is empty, and only recalls explicitly from the watchdog when
banking. Its ordinary purchase recall also requires much more gold and excludes
Tavi. This is a concrete missing recovery path consistent with the recording.
The exact last objective/component state still needs a controlled instrumented
reproduction before claiming a complete root cause or verified fix.

The stalled units had only two and one fuel suspensions, respectively. High
fuel use elsewhere does not explain these long inactive runs by itself.
Priority for the next revision: meaningful-progress tracking, no-objective
recovery, crystal resupply and a return/escape plan for disconnected terrain.
This is distinct from the contested-capture wait fixed during v3 development.

### Fuel overhead is still high

Our side had **400 resumable fuel suspensions**, versus six for the opponent.
Counts by map were 72, 141, 120, 8 and 59 in the table's order. The private log
contains 388 completed spill messages; it is not the authoritative total because
a paused frame may never finish. Native replay events establish 400. These are
not crashes, but can consume productive turns. Large-map observation, route
search and repeated component planning remain profiling targets.

### Survival and throughput still need work

We had 34 individual body deaths: **26 followed champion actions and eight
followed camp actions**. Opponents had 91 body deaths, including linked Cassi/
Issac deaths; body counts must not be presented as kill counts. On
`00pilgrimage`, we lost 83 carried gems and needed 1,989 rounds to finish. Tavi
lost 30 gems at round 1,734 with three enemies within four tiles, and lost
15-gem bags in two other deaths. Those events warrant checking delivery routes
and retreat feasibility under concentrated pressure. They do not establish
that a lower blanket banking threshold would improve results.

`01gold_rush` went the full 2,000 rounds despite the large lead and deposited
957, below the immediate-win threshold. Throughput on that layout deserves a
separate test; the win alone does not prove fast or optimal objective execution.

## Next test priorities

1. Reproduce the two island stalls and verify recovery with empty bags, no
   crystal, insufficient shopping gold, healthy champions and disconnected
   objectives. Test the return journey as well as the first crossing.
2. Include the two newly encountered maps in the next revision's regression pool.
3. Profile large-map fuel overhead and retain the current life/round/action
   guards while reducing repeated planning work.
4. Test the same scenarios against stronger pressure fixtures and new ranked
   opponents. Preserve the current source/ZIP as the comparison baseline.

This review records evidence and next targets. It does not claim that these
remaining issues were patched or that a new submission was validated.
