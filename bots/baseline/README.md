# v4 — targeted failure-case fixes

C++ source bot for organiser **SDK/CLI 3.0.5, engine 1.0.3**. This is a focused
revision of v3 after reviewing 20 recent games supplied at approximately 1690
Elo and the preceding post-release failure notes. See the
[diagnosis](../../docs/diagnostics/2026-10-22-ranked-review.md) and
[measured results](../../tests/results/v4/README.md).

## Changes from v3

- **Preserve route planning across fuel pauses.** A completed distance search
  may be useful even when its frame resumes in another turn. Banking preserves
  that plan and defers the move, keeping the round/life/fuel/action guards.
  It no longer repeatedly discards its route because planning crossed a turn.
- **Recover stranded farmers.** After the progress watchdog fires, a unit away
  from spawn can safely recall without needing to be banking or carry enough
  shopping gold. Current and predicted visible damage must both be absent.
- **Stun before retreating.** Against a small number of pursuers, consider a
  legal, ready targeted stun before spending movement and disabling an exclusive
  ranged spell. Remaining defenses and movement still obey the one-cast limit.
- **Use mobile economy in three-pick teams.** Ordinary three-pick maps choose
  Varg, Vesper Vale and Heart of Ruin. Vesper adds a dash, attack reset and bonus
  gold. Four/five-pick ordinary drafts retain the v3 roster and density ordering.
- **Use a control frontline on boss-only gem maps.** Choose Varg, Heart,
  Marcella, Ryth and Vesper as picks permit. This replaces the earlier support
  opening that repeatedly lost its economy under player pressure.

The existing risk-adjusted camp scoring, route caches, eight-byte task
reservations, camp obstruction clearing, bounded capture waits, legal transport,
shield timing, carrier banking, altar-lock handling and combat filters remain.
Item families and banking thresholds are those of v3. The SDK refresh is official organiser code, recorded separately.

## Build and reproduce

Use CLI 3.0.5 and the checked-in SDK:

```sh
cambc build baseline --release
mkdir -p bots/v3 bots/pressure-caster bots/bruiser-pressure bots/control-pressure
unzip submissions/v3-patched-sdk-bot.zip -d bots/v3
unzip tests/opponents/pressure-caster.zip -d bots/pressure-caster
unzip tests/opponents/v3/bruiser-pressure.zip -d bots/bruiser-pressure
unzip tests/opponents/v4/control-pressure.zip -d bots/control-pressure
python3 scripts/evaluate_bot.py --candidate baseline --opponent v3 --output matches/new-v4-v3
python3 scripts/evaluate_bot.py --candidate baseline --opponent pressure-caster --output matches/new-v4-caster
python3 scripts/evaluate_bot.py --candidate baseline --opponent bruiser-pressure --output matches/new-v4-bruiser
python3 scripts/evaluate_bot.py --candidate baseline --opponent control-pressure --output matches/new-v4-control
```

Every output directory must be new. The evaluator defaults to all **49 repository
maps from both sides**, validates completed statistics and gem balances, and
checks both teams for native faults and caught API errors. It records both source
hashes, runtime/build hash, map hash, seed, economy and fuel suspensions.
This covers the repo, not the entire separately published current map pool.

## Validation and remaining limits

[`VALIDATION.json`](VALIDATION.json) ties the exact frozen source to report and
submission ZIP checksums. The ZIP was extracted, rebuilt and tested separately;
all corresponding scores, seeds and match lengths matched. Archived results
and losses live in [`tests/results/v4`](../../tests/results/v4/README.md).

Local fixed-seed fixture comparisons do not predict ranked Elo or win rate.
Some boss matchups, economic races and control pressure remain weak. Replay
checks still show individual island stationary runs up to 319 turns; universal
stall freedom is not established. Multiple walking components and repeated
transport planning remain priorities. Fuel pauses are substantially reduced
in the comparisons but are still present. Preserve the immutable v3 ZIP and its
[historical validation](../../tests/results/v3/VALIDATION.json) for further tests.
