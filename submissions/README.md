# Submission bundles

## v4 — failure-case update

[`v4-failure-case-update.zip`](v4-failure-case-update.zip) is the current source-only
bot for **SDK/CLI 3.0.5, engine 1.0.3**. It preserves route plans across fuel
pauses, adds safe stranded-unit recovery and stun-before-retreat handling, and
adjusts three-pick and boss drafts. The frozen source completed 392 full-pool
games, 16 ZIP extraction/rebuild games and 12 focused archetype games, with zero
faults or caught API errors on either team. See [results and remaining limits](../tests/results/v4/README.md).

SHA-256: `084d3ee511ed8502b157e3b679d763e719d9e4835f4569c4409abefed1656da9`

The ZIP contains the six source files listed in the validation manifest.
It is prepared for submission; an arena upload is a separate action.

## v3 — patched SDK

[`v3-patched-sdk-bot.zip`](v3-patched-sdk-bot.zip) contains the current source-only
bot for **SDK/CLI 3.0.4, engine 1.0.2**. All 34 checked-in maps were tested from
both positions against three opponents (204 games). Extraction, rebuild and
12 further matches verified this exact ZIP, with no faults or caught API errors
on either team. See [the measured results and limitations](../tests/results/v3/README.md).

SHA-256: `f6ff205e76ac4de8ea3bc623661a395633de9abe6232a9c19662014d02d609de`

The archive contains exactly `src/bot.cpp`, `src/bot.h`, `src/draft.cpp`,
`src/draft.h`, `src/main.cpp` and `src/random.h`. Download the ZIP as a raw file
for submission; the prepared bundle does not establish an arena upload.

## v2 — Sprint 2

`v2-sprint-2-bot-update.zip` contains the v2 source submission for SDK 2.2.5.
It was extracted, rebuilt and tested separately. The source hashes match all
106 final validation games. See [the results](../tests/results/v2/README.md).

SHA-256: `a25ec953026ac034463c65efe3895f3d5752d1f8ab618f4dad7453c5d3daa1a4`

This bundle has been prepared locally; an authenticated competition upload
is a separate action.

## v1 — Sprint 1 (preserved)

`v1-sprint-1-bot-update.zip` contains the verified source submission for commit `53bf041`, using SDK 2.2.5.

Download the ZIP directly using GitHub’s **Download raw file** button, then upload it to the competition platform.

SHA-256: `0c00900c5abe1f59c0c95f2feb539ae90b26d4313be8fc1f2c159193ea75d47c`
