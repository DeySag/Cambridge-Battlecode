# v3 — findings and next iteration

Updated 21 October 2026. This is a historical handoff note.

**22 October follow-up:** the v4 revision implements route-plan preservation
across fuel pauses, safe empty-bag recall recovery, defensive stun ordering and
targeted draft changes. See [the new diagnosis](2026-10-22-ranked-review.md) and
[validation](../../tests/results/v4/README.md). Residual island inactivity and
opponent-dependent losses remain; the original observations below are retained.

Current released source is commit `ed00af8` (`bot v3 update`), SDK/CLI 3.0.4,
engine 1.0.2. Preserve the v3 submission ZIP and its validation as the baseline.

## Results reviewed since release

- First reported ranked win, bundle `dade83d3`: **5–0**. User describes the
  opponent as comparatively easy. Deposited 5,031 / 5,349 collected gems (94.1%),
  but two champions became inactive on islands; the sweep concealed a failure.
- Unranked rematch, bundle `8dfc9150`: **3–2**. User reports losing 1–4 to this
  opponent previously. Won `00quadrants`, `00ambush`, `00corners`; lost
  `00fragment` and `01boss_battle`. This is encouraging, but different games
  do not establish a controlled estimate of v3's improvement.
- Unranked vs user-reported **1609** bot, bundle `b58426d2`: **2–3**. Won
  `01yellow_brick_road` and `00stampede`; lost `01big_world`, `01hooks`,
  `00fragment`. The series was close; several individual losses were decisive.

All 15 replays used engine v1_0_2. Individual gem accounts and final team scores
reconcile. No native bot faults or caught API errors were found on our side.
The user identifies the submission as v3; replays do not provide a source hash.
Unranked results do not establish an Elo gain or ranked win rate.

## Confirmed problems, in priority order

1. **Recovery still permits long inactivity.** On `01islands`, Tavi stayed on
   one tile for 1,266 living end turns and Marcella for 1,121; both collected
   zero gems. Each had completed one crystal teleport. Neither has a dash;
   field consumable purchases are prohibited even for Tavi. The source has no
   effective empty-bag/no-objective recall fallback. Crystal resupply and return
   planning are plausible missing paths, not yet a fully reproduced root cause.
2. **Healthy units can also stop at spawn.** On `01big_world`, Heart and Marcella
   stayed at spawn positions for 1,799 and 1,798 living end turns, starting at
   round 48. Tavi and Vesper had stationary runs of 1,618 and 1,486 turns. The
   team collected only 106 gems and lost **46–1,016**, with just two deaths.
   Repeated sampled positions show healthy units holding small bags on spawn.
   Adding recall alone cannot solve a unit already on spawn. Trace objective
   selection, navigation, congestion and planning-budget returns explicitly.
3. **Boss exposure and delivery can collapse together.** On `01boss_battle`
   in the rematch, we collected **937**, deposited **zero**, lost **850**, and
   ended with 87. There were 27 deaths, **21 following camp actions**, and only
   one tribute start, with no completion. Varg lost all 311 gems he collected.
   Prioritise safe approach/exit routes, monster exposure and actual delivery;
   changing the bag threshold alone does not establish a solution.
4. **Compact-map combat/economy is consistently weak.** `00fragment` lost
   **126–1,010** and **148–1,008** against different opponents. We had 31 deaths
   in each game versus five and three, and earned 2,440 / 2,450 gold versus
   12,434 / 10,432. On `01hooks`, we lost **22–1,001**, with 16 deaths, all
   following champion actions, versus one enemy death. This is resource denial
   and combat failure, not primarily dropping large gem bags. Test draft,
   equipment, spell timing, local support and disengagement separately.
5. **Fuel remains expensive, especially navigation.** First win: 400 fuel
   suspensions; rematch: 211; 1609 series: 818. `01big_world` alone had **759**,
   including 575 on Varg. Its stdout contains 633 completed spill messages,
   **619 tagged `navigate`**. Recorded events, not stdout, give the total;
   suspended frames may never finish printing. Other stalled units had far
   fewer suspensions, so fuel alone does not explain every inactive unit.

## Positive evidence and cautions

- `01yellow_brick_road` beat the 1609 bot **1,002–127 in 679 rounds**, with all
  collected gems deposited. Preserve this as a regression case for capture
  progress; its v3 fix does not cover every other form of stalling.
- Rematch wins on `00ambush` and `00corners` were **1,050–361** and **1,036–361**.
  Avoid weakening effective farming/delivery while changing compact-map combat.
- The 1609 `00stampede` win was **960–0 at round 2,000**, despite 43 of our
  deaths. Varg delivered all 960; the other four units delivered none. Another
  160 remained carried while Varg was still travelling at the deadline. This
  does not establish a channel-interruption bug. Assess useful pressure and
  carrier protection, not death counts alone; test endgame route timing.

## Next development pass

- Reproduce the island and big-world failures with instrumented stage, target,
  component, fuel and progress state. A watchdog must produce a viable action;
  merely resetting cached plans is insufficient. Distinguish stationary,
  contested capture, channel, healing and failed planning states.
- Profile navigation and component searches; reserve action fuel and amortise
  planning without repeatedly returning before the recovery logic can run.
- Test boss escape/delivery and compact-map combat fixes against independent
  opponents. Do not claim a root cause or successful fix before reproduction.
- Add exact newly encountered maps to the *next* regression pool. Five from the
  latest bundles are absent by SHA-256 from the 34-map release pool:
  `00quadrants`, `00corners`, `00fragment`, `00stampede`, `01big_world`.
  The previous first-win bundle additionally exposed `01elk_arena` and
  `01gold_rush`. Public names/full hashes were verified against the saved
  organiser manifest; similar legacy terrain does not prove identical files.
- Freeze the next candidate and rerun both sides on the expanded pool against
  v3 and pressure fixtures. Preserve failed trials and do not overwrite old
  validation claims with results from changed sources.

## Evidence locations

- Existing first-win review and compact JSON:
  `docs/diagnostics/2026-10-21-v3-first-win-*`.
- Full local analyses: `matches/v3-first-win-analysis.json` and
  `matches/v3-close-unranked-analysis.json`.
- Extracted inputs: `matches/v3-first-win-inputs/` and
  `matches/v3-close-unranked-inputs/`.
- Exact recovered maps: `matches/v3-first-win-maps/` and
  `matches/v3-close-unranked-maps/`.
- Reusable tools: `scripts/review_ranked_replays.mjs` and
  `scripts/recover_replay_maps.py`; manifest:
  `tests/results/v3/published-map-pool.json`.

The `matches/` evidence is intentionally ignored by Git and local to this
workspace. No bot source was changed during these post-release reviews.
