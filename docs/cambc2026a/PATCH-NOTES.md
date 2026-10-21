# Organiser patch update notes

Recorded on **21 October 2026** from organiser announcements supplied by the
user. Publication dates, patch identifiers and the matching SDK version were
not supplied. The announcements below retain the supplied order and wording;
the recording date is not an asserted release date.

These announcements supersede the affected balance values and ladder policy in
the older reference material. `GameRules.md` and `BotAPICpp.md` remain historical
exports. See the compatibility notes after the announcements before using these
values with the checked-in SDK.

## Supplied organiser announcements

### Heart of Ruin — emergency nerf

We have made an emergency nerf to Heart of Ruin due to near 100% usage rates in top ladder:

- Strength per shard (+Strength) : 5 → 4
- Focus per shard (+Focus) : 15 → 12

### Important Ladder Changes

- Previously, new patches that caused compilation errors would remove your bot from matchmaking and you could essentially freeze your elo
- This has now been changed such that your team will be marked as having an out of date submission ("invalid bot") and you will have a grace period for 24 hours to patch it before you will start auto losing ranked matches.
  - The site will alert you if you are in this situation and need to submit a new bot

### Stellar

- Health (Health) : 750 → 650
- Health per shard (+Health) : 75 → 65
- Focus (Focus) : 85 → 70
- Focus per shard (+Focus) : 9 → 7

### Heart of Ruin

- Strength (Strength) : 40 → 30
- Strength per shard (+Strength) : 4 → 3.5
- Dash landing damage (S2): 1.75 x Focus (Focus) → 1.5 x Focus (Focus)

### Ren

- Stun damage (S1): 1.25 x Focus (Focus) → 0.75 x Focus (Focus)

Focus champions have a lot of front loaded damage, which has been hard to play around. These champions also have extremely high pick rates all across the board and so we are lowering their output.

---

### Vesper Vale

- Health (Health) : 900 → 1000
- Strength (Strength) : 50 → 55

### Vendetta

- Health (Health) : 800 → 1000

### Tavi

- Health (Health) : 750 → 900
- Focus (Focus) : 100 → 110

Strength champions are struggling to endure the amount of burst in the game, so we are giving them a bit more health to help them through early game combats

---

### Prism

- Passive focus (Focus) per gem (Gems) : 1 → 1.5

Prism has not seen any usage in higher ranked play, hopefully these changes will encourage teams to build a specialist strategy around them

## Reference and SDK compatibility

- **Heart of Ruin:** the supplied Strength growth changes form the sequence
  **5 → 4 → 3.5**. The latest supplied values are base Strength **30**,
  Strength growth **3.5**, Focus growth **12**, and S2 dash landing damage
  **1.5 × Focus**. The checked-in SDK 2.2.5 has no champion named Heart of Ruin
  in its champion or draft enums. Its identity and full kit must be checked
  against the corresponding organiser SDK; do not map it to another champion.
- **Stellar:** the old handbook export lists Focus **100** and Focus growth
  **10**, whereas these announcements start from **85** and **9**. Retain
  those as separate historical snapshots; the latest supplied targets are
  Health **650**, Health growth **65**, Focus **70**, and Focus growth **7**.
- **Vesper Vale:** the old handbook export lists Strength **45**, whereas
  the announcement starts from **50**. The latest supplied targets are
  Health **1000** and Strength **55**.
- **Prism:** the old handbook and the checked-in SDK passive constant use
  **0.2 Focus per gem**, whereas the announcement starts from **1**. The latest
  supplied target is **1.5 Focus per gem**; this documentation update does not
  establish that the local engine implements it.
- **Ability numbering:** the organisers' S1 and S2 correspond to the existing
  API's zero-based slots 0 and 1. Ren's latest supplied S1 damage is
  **0.75 × Focus**; no other Ren change is stated here.

The project remains pinned to SDK **2.2.5**. Bot source, SDK code, submission
bundles and previous validation results are unchanged by this documentation
update. Previous benchmarks describe their recorded engine and balance values;
they do not validate performance on these patches. Before adapting or submitting
the bot for the patched ladder, obtain the matching SDK, check its metadata and
API compatibility, rebuild in release mode and rerun the match evaluations.
For patch-induced compilation failures, the announced ladder policy allows
**24 hours** to repair an invalid submission before automatic ranked losses;
the competition site alerts affected teams.
