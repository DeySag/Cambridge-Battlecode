# v2 — farming with local combat support

C++ submission for organiser SDK 2.2.5. The first ranked replays showed that
Sprint 1 lost too many fights and carried gems, even when its farming was
competitive. v2 combines farming with spell damage, nearby ally support and
safer delivery. See the [ranked review](../../docs/diagnostics/2026-10-19-ranked-review.md).

## Strategy

1. **Draft farming and combat together.** On ordinary maps choose Varg,
   Stellar, Cassi/Issac, Ryth and Tavi in that order, up to the map's pick limit.
   The linked pair provides two separately controlled units for one pick, with
   the risk that both die if either dies. On boss-only gem maps retain the
   Tavi, Stellar, Prism, Cassi/Issac, Ren draft.
2. **Farm separate objectives.** Reserve camp groups with one byte per unit.
   Rank rewards by walking, clearing, capture and delivery time, health risk,
   visible enemies and recent deaths. Hold a safe capture until it completes.
3. **Take useful nearby fights.** Compare visible friendly and enemy health,
   damage and Focus before pursuing a visible opponent within seven tiles.
   Prioritize vulnerable targets and gem carriers. Keep farming when no
   productive engagement is available; do not attack enemies healing on spawn.
4. **Use spells and defenses.** Estimate enemy bursts from basic attacks,
   live ability ranges, damage, cooldowns and stuns. Shields, escape dashes and
   defensive attacks help recover from danger. Ground spells can target
   enemy champions as well as camps. This is a short heuristic, not a full
   combat simulator or an assumption that allied vision is shared.
5. **Deliver safely.** Prefer an altar using actual terrain path distance,
   visible danger, locks and remembered death locations. Deposit convenient
   bags, ordinarily bank at 100 gems, and bank smaller exposed bags when
   health or nearby pressure makes that worthwhile. Reassess long channels;
   damage alone does not interrupt tribute, so survivable channels can finish.
   A failed route chosen to avoid danger never silently retries through it.
6. **Upgrade for both jobs.** Buy Power Strength for basic attackers and
   Power Focus for spellcasters, interleaving Power Health on ordinary maps.
   Tavi buys in the field. Recall for healing or substantial unspent gold,
   and stay on spawn through the healing turn. Buy and use altar locks.
7. **Remember danger.** Each unit discounts camps near its recent death
   locations for up to 180 rounds, with the penalty fading over time.

Monolith farming still uses range-four spells outside the attack range of
three reported by this SDK's runtime. It must be rechecked after balance/SDK
changes. Issac farms ordinary resources on that map rather than attempting
the monolith with his basic attacks.

Pathfinding uses fresh occupancy, cached large-map routes and budget checks.
Action boundaries recheck round identity and availability so a calculation
resumed after a fuel suspension does not issue stale movement or casts.

## Build and evaluate

```sh
cambc build baseline --release
python3 scripts/evaluate_bot.py --candidate baseline --opponent /path/to/sprint1
python3 scripts/evaluate_bot.py --candidate baseline --opponent /path/to/pressure-caster
```

Use organiser SDK/CLI 2.2.5 and release mode, matching competition fuel rules.
The evaluator runs each checked-in map from both sides, requires completed
statistics, reconciles gem totals and checks our replay logs for API errors.
Reports include score, collected/deposited/lost gems, deaths and fuel events.

The original submission remains in
[`submissions/v1-sprint-1-bot-update.zip`](../../submissions/v1-sprint-1-bot-update.zip).
The two reproducible pressure opponents are documented under
[`tests/opponents`](../../tests/opponents/README.md).

## Validation and submission

`VALIDATION.json` records final source hashes, benchmark outcomes, fuel counts
and the source ZIP checksum. Local full reports and replays are retained under
ignored `matches/v2-final-*`. The archived benchmark summaries are under
`tests/results/v2/`. These local opponents are regression tools, not a ranked
win-rate or Elo prediction.

The v2 source bundle is
[`submissions/v2-sprint-2-bot-update.zip`](../../submissions/v2-sprint-2-bot-update.zip).
It contains source files only. Its round-trip verification extracts the ZIP,
rebuilds it as a separate bot and runs both sides on ordinary and boss maps.
Build/test/package work does not establish a competition upload.
