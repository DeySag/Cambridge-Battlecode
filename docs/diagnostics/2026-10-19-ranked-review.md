# Sprint 1 ranked replay review — 19 October 2026

The main repeatable weakness is turning farmed resources into safe deposits under enemy pressure. Combat losses also prevent us from securing rewards at all on some maps. Improving only the deposit threshold will leave that second problem intact.

This review covers five **user-selected losing series**, containing 25 distinct games: five map wins and 20 map losses. It is a diagnostic sample, not an estimate of our overall win rate or of a future version's Elo. The user confirmed every game used the original Sprint 1 submission. No bot implementation changes were made during this review.

## Inputs and verification

| Bundle prefix | Our side | Verified series result |
| --- | --- | --- |
| ba5442c2 | A | 2–3 |
| 88a4baf1 | B | 1–4 |
| a4360c4b | B | 0–5 |
| 7dacb51c | B | 1–4 |
| 1e1ee763 | B | 1–4 |

Each outer ZIP contains five match ZIPs, each with a full `replay.repl26a` and our private stdout/debug/fuel files. Our side was identified from the private log filenames. The 25 replay hashes are unique. Embedded map bytes match the checked-in map pool.

Replays were advanced through the SDK 2.2.5 visualiser's engine, release `v0_0_3`, to their recorded terminal states. Champion bags, team scores, life transitions, actions and round snapshots were decoded using that visualiser's published snapshot layout. This does not rerun either bot or invent alternative opponent actions.

Every champion passed `collected = deposited + lost + carried_at_end`; every team's reconstructed deposits equal its final score. All five series results match the user's labels. Per-game values and replay checksums are in [the CSV](2026-10-19-ranked-matches.csv) and [JSON](2026-10-19-ranked-summary.json). Detailed event data and the analysis scripts are retained locally under ignored `matches/ranked-diagnostics/`.

Deaths in this report count individual champion bodies. Cassi and Issac die together but grant one kill reward, so deaths must not be equated with the other team's kill counter. Both occur in our two contest drafts. The other 23 games avoid that complication on our side. Gold income in the data files is reconstructed from positive wallet changes; no sell actions were recorded. Damage/shield totals were not reconstructed and are not claimed here.

## What repeats across the losses

| Metric, 20 losing games | Us | Opponents |
| --- | ---: | ---: |
| Gems collected | 16,632 | 20,948 |
| Gems deposited | 9,514 | 19,551 |
| Deposited / collected | **57.2%** | **93.3%** |
| Gems lost from bags | **5,584** | **841** |
| Gems carried when games ended | 1,534 | 556 |
| Individual champion deaths | 382 | 81 |

The deaths include the exceptional contest loss, where our six bodies died 134 times. Excluding that game leaves 248 of our deaths versus 77 enemy deaths across the 19 other losses. The gem totals do not change because neither team collected any gems in that contest game.

Four losses occurred despite collecting **more** gems than the opponent:

| Bundle / map | Collected, us : enemy | Deposited, us : enemy |
| --- | ---: | ---: |
| 88a4baf1 / packs | 1,126 : 1,048 | 850 : 1,026 |
| a4360c4b / packs | 1,136 : 1,124 | 856 : 1,008 |
| 88a4baf1 / interlock | 1,386 : 1,052 | 878 : 1,008 |
| 7dacb51c / pilgrimage | 1,512 : 1,020 | 900 : 1,020 |

These are strong candidates for improvements to delivery and survival. They do not establish that a particular change would have flipped the result: changing our actions also changes opponent responses.

Of the 5,584 gems lost in losing games, **4,610 (82.6%) were lost in bags below 100 gems**, and 974 were lost in seven deaths with bags of at least 100. A fixed trigger of 100 leaves substantial value exposed. There were 166 submission-start events and 143 completions in these games; most of the delivery deficit therefore occurs before starting a successful channel, though interrupted or unfinished channels also matter. Some starts can still be active when the game ends, so the difference is not a pure interruption count.

Across the 23 non-contest games, **255 of our 268 deaths followed enemy-champion actions**, including spells, and 13 followed camp actions. Enemy pressure is the dominant observed cause. This does not prove every death was avoidable; a safe escape requires position, turn order, cooldown and visibility analysis.

## Map patterns in this sample

| Map | Wins / games | Main evidence |
| --- | ---: | --- |
| packs | 0 / 4 | Two losses despite collecting more gems; delivery is an immediate target. |
| interlock | 0 / 3 | 320–1018, 878–1008, 568–1010; bag losses of 446, 436 and 392. |
| pilgrimage | 0 / 2 | 900–1020 and 63–1007; collected 1,512 and 772, respectively. |
| elkring | 0 / 2 | 270–1026 and 342–1026; only 39.5% aggregate gem conversion. |
| corners | 0 / 2 | 306–1050 and 552–1036; survival and delivery both fail. |
| monolith | 0 / 2 | 644–1032 and 746–1018; collection totals are relatively close. |
| swirl | 0 / 2 | 132–1086 and 420–1061; far fewer gems secured, not just undelivered bags. |
| stampede | 1 / 2 | 1080–180 win, 360–1080 loss; major change in objective control. |
| contest | 1 / 2 | 800–200 win at the time limit; 0–0 loss with 134 body deaths. |
| fragment | 0 / 1 | 355–1007. |
| rushbait-v2 | 1 / 1 | 1084–806, despite losing 384 carried gems. |
| eight | 1 / 1 | 1101–209; two deaths and 44 gems lost. |
| ambush | 1 / 1 | 1106–0; one death, zero gems lost; the opponent collected zero. |

These small, selected samples identify regression targets, not permanent rankings of maps or champions. The ambush win in particular does not demonstrate strength against an effective opponent on that map.

## Replay examples to retain

### Pilgrimage: farm acquired, delivery failed

Bundle `88a4baf1`, match `d71b45e3-81f3-422f-a796-aedffc65d516`: loss **63–1007** at round 634.

- Our team collected 772 gems, lost 497, and ended carrying 212.
- Vendetta made our only completed deposit: 63 gems at round 285.
- Varg died carrying **185** gems at round **306**, then ended the match carrying another 144 without any deposit.
- Ren died carrying **183** gems at round **471** and never deposited.
- Both large-bag deaths followed enemy actions. This gives concrete positions and timing for examining safe routes, channel choices and escape behavior.

### Stampede: depositing perfectly is insufficient

Bundle `a4360c4b`, match `124a67a9-0ac5-454b-bb04-77be28fa274b`: loss **360–1080** at round 1322.

- We collected exactly 360 gems and deposited all of them; bag loss was zero.
- We suffered **44 deaths**. Varg, Vendetta, Ren and Tavi collected zero gems; Vesper Vale secured the team's only gem reward and deposited 360 at round 1070.
- The opponent collected 1,440 gems.
- This is an objective-access and survival failure. A lower banking threshold cannot repair it by itself. The comparison stampede win in `88a4baf1` is 1080–180, with seven of our deaths and 33 enemy body deaths.

### Contest: combat collapse and neglected tiebreak economy

Bundle `7dacb51c`, match `7bcc946e-2210-4394-b81c-2a14aec8bb71`: **0–0**, Team A wins at round 2000.

- Neither team collected or deposited gems.
- Our draft produced Tavi, Stellar, Prism, Cassi, Issac and Ren. They suffered 134 individual body deaths versus four enemy body deaths. Cassi and Issac each died 25 times; their linked deaths explain part of the difference between death and kill counts.
- Our first death was Prism at round 23, with several enemies within one tile. The enemy's economy then grew far ahead: observed lifetime wallet increases were 14,165 versus our 955. With gems tied, the official rules compare lifetime gold before kills.
- Our other contest game won 800–200 at round 2000, but still lost two 200-gem bags and suffered 25 body deaths. The current boss plan can work against some opponents; it is not robust under active contesting.

### Packs: closest clean delivery regression

Bundle `a4360c4b`, match `37b1ad0e-5b46-4ee9-924e-3891c0f508d6`: loss **856–1008** at round 583.

We collected 1,136 gems to their 1,124, lost 184 and ended carrying 96. This is a useful small-map regression alongside the 850–1026 packs loss in `88a4baf1`.

## Code findings consistent with the replays

These are verified properties of Sprint 1, rather than inferred opponent behavior:

1. **Threat estimation understates spells and crowd control.** `incoming()` uses `enemy.strength * 1.3 + 15`, within basic attack range plus one. It does not use enemy Focus, ability damage, cooldowns or stun chains. A base Stellar spell deals 200 before upgrades, while this formula estimates 60.5 for that enemy. Melee champions with range-four spells are only included within two tiles. See [bot.cpp](../../bots/baseline/src/bot.cpp#L160).
2. **Normal objective movement ignores danger zones.** Farming and capture navigation use the default `avoid_danger=false`. Danger-aware navigation can explicitly retry with danger avoidance disabled when its route is unreachable. This is especially risky while carrying gems. See [navigation](../../bots/baseline/src/bot.cpp#L170) and [turn decisions](../../bots/baseline/src/bot.cpp#L586).
3. **Retreat reacts late and has limited defensive behavior.** It generally triggers below 28% health or around two estimated incoming hits. Escape does not deliberately use shields or crowd control to survive; its active ability handling is movement-oriented. See [retreat](../../bots/baseline/src/bot.cpp#L380) and [thresholds](../../bots/baseline/src/bot.cpp#L551).
4. **Existing channels prevent fresh observation.** The turn returns immediately while recalling/submitting/teleporting, before observing nearby enemies. There is no threat-based decision to cancel or finish a channel. Any replacement must account for remaining channel time: damage alone does not interrupt a gem submission, and finishing a nearly complete deposit can be correct. See [channel handling](../../bots/baseline/src/bot.cpp#L543).
5. **Banking is largely a fixed bag-size/distance rule.** It triggers at 100 gems, within three walking steps of an altar, for a winning bag, or near the time limit. It does not explicitly price death risk, route exposure or likely enemy arrival during the channel. Altars share one navigation goal set rather than being compared by delivery safety. See [banking](../../bots/baseline/src/bot.cpp#L337) and [trigger](../../bots/baseline/src/bot.cpp#L565).
6. **Local enemy avoidance has no lasting danger memory.** Target scoring has a visible-enemy penalty and an early home-side preference, but a champion can return to a dangerous area after respawning or after enemies leave vision. The draft is also fixed by camp density/boss availability, rather than the enemy composition. See [scoring](../../bots/baseline/src/bot.cpp#L258) and [draft.cpp](../../bots/baseline/src/draft.cpp).

## Recommended Sprint 2 order

### 1. Make survival decisions aware of enemy spells and numbers

Keep the design simple: forecast a short burst of damage using enemy champion type, Focus, basic attacks, spell range and available cooldown information. Add nearby ally/enemy support. Use that estimate before engaging or moving to an objective. Defensive stun/shield/escape use should take priority when necessary. Avoid a blanket increase in retreat thresholds: abandoning every almost-cleared camp can reduce income.

For gem carriers, do not silently switch from a route chosen to avoid danger to one through the same danger. Give the bot a deliberate fallback: a different altar, a safe waiting position, a defended route, or recall while retaining the bag. These alternatives require comparing expected delivery time and exposure.

### 2. Make banking depend on risk and channel duration

Test a lower or adaptive threshold, incorporating bag value, health, enemy presence, walking distance and submission duration. Pick a suitable altar explicitly. Preserve efficient large deposits in safe regions rather than forcing every small bag across a long map. Reassess active channels against remaining time and danger.

Measure deposits per collected gem, bags lost and time spent delivering together. Conversion alone can rise while actual score per round falls.

### 3. Handle repeatedly contested objectives and boss maps

Remember recent deaths/threats by area, prefer safer productive objectives, and allow limited reinforcement when a valuable camp genuinely requires it. Preserve reservation-based distribution for ordinary farming. Do not march isolated units repeatedly back into a lethal hotspot.

Treat contest separately: obtain an initial gold economy, protect spell farmers, and maintain a gold-tiebreak plan if boss capture is unrealistic. Stampede needs a plan for reaching and securing its valuable rewards under pressure. A general-purpose banking patch is insufficient for either example.

### 4. Test draft and equipment alternatives after survival behavior

Compare one controlled roster change at a time, including a spellcaster or Cassi/Issac alternative to the fixed ordinary roster, and compare Strength/Focus choices for champions whose spells matter. The linked pair creates an extra unit but requires keeping both alive.

Champion totals here are confounded by different map appearances, draft order and role. They do not justify automatically removing Ren or declaring another champion superior.

### 5. Keep execution fixes proportionate

Across all 25 games, our replays record **zero bot-fault events**, our logs contain **zero caught API errors**, and there are **seven fuel-exhaustion events**, all in the ambush win. Stdout contains six completed fuel-spill messages; event counts are the appropriate exhaustion total. Fuel suspensions still deserve regression checks, but they are not the dominant explanation for these losses.

## Validation needed before a new submission

- Preserve the original Sprint 1 ZIP and source checksums as the comparison baseline.
- Add an aggressive local opponent with spell pressure, pursuit, altar interference and objective contesting. Winning against the starter and earlier farming opponents does not adequately validate these behaviors.
- Evaluate candidate and Sprint 1 on both sides across the same map/seed set. Include packs, interlock, pilgrimage, swirl, stampede and contest, plus previous strong maps to catch regressions.
- Track score rate, win outcome, deposit conversion, bag loss, deaths, captured rewards, recall/delivery overhead, API errors and fuel. Separate boss maps from the ordinary aggregate.
- Change one major behavior at a time. Inspect close losses and failures even if the aggregate local win count improves.
- Follow with new arena games against multiple opponents. These replay files contain recorded opponent actions, not their bot source; replaying those inputs cannot provide a valid responsive counterfactual match against an updated bot.

The highest-priority change is **surviving enemy contact while securing and delivering rewards**. Reliable banking is one part of that behavior; contested-resource access and the boss-map economy are the other parts exposed by this sample.
