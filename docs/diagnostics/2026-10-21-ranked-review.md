# Four recent ranked losses — 21 October 2026

The user reports a rating fall from **1603 to 1528**. The four selected series contain **20 games: 5 wins and 15 losses**. This is a diagnostic sample, not an overall ladder win rate.

## Reconstruction and provenance

The uploaded nested ZIPs contain private logs and recorded actions. Each replay was advanced with its matching organiser visualiser engine **v1_0_2**, bundled in CLI **3.0.4**. The offline analysis used the published snapshot layout with an unrestricted host observation perspective, not a champion’s partial view. Every champion passed `collected = deposited + lost + remaining`; reconstructed deposits equal both final team scores in all 20 games.

The original 17 repository maps did not include many of these layouts. All 17 distinct maps embedded in the replays were recovered with `scripts/recover_replay_maps.py`; their complete file SHA-256 values matched the organiser’s public map-pool manifest. They are added with the organiser’s `00`/`01` names; the original maps are preserved. This gives 34 checked-in maps. The published pool lists 35 maps, so these files do not claim to cover that entire expanded pool.

Full local event reconstruction is in ignored `matches/v3-ranked-analysis.json`. A compact, reproducible record is checked in alongside this review. `scripts/review_ranked_replays.mjs` accepts an extracted replay tree, an output JSON path and the matching visualiser assets directory. Replays can establish recorded events; they cannot simulate a responsive opponent against changed bot actions.

The logs and drafts are consistent with the checked-in v2 bot. The replay does not provide a compiled bot source checksum. Death counts below count individual bodies: Cassi and Issac die together and do not each award a separate kill.

## Match results

| Series | Map | Our side | Our score | Enemy score | Result | Rounds |
| --- | --- | --- | ---: | ---: | --- | ---: |
| 703e6f63 | 01shortcut | B | 127 | 929 | Loss | 2000 |
| 703e6f63 | 01islands | B | 572 | 1051 | Loss | 1827 |
| 703e6f63 | 01broken_curve | B | 971 | 1016 | Loss | 1383 |
| 703e6f63 | 01boss_battle | B | 0 | 0 | Loss | 2000 |
| 703e6f63 | 00contest | B | 1000 | 200 | Win | 1999 |
| 74ae282b | 01islands | B | 232 | 876 | Loss | 2000 |
| 74ae282b | 00rows | B | 1005 | 710 | Win | 1703 |
| 74ae282b | 01hooks | B | 242 | 946 | Loss | 2000 |
| 74ae282b | 01yellow_brick_road | B | 0 | 755 | Loss | 2000 |
| 74ae282b | 01broken_curve | B | 1001 | 285 | Win | 1338 |
| 85f66155 | 01concentrated_wealth | A | 482 | 1009 | Loss | 1468 |
| 85f66155 | 01the_farm | A | 298 | 1032 | Loss | 1140 |
| 85f66155 | 00swirl | A | 182 | 1004 | Loss | 1493 |
| 85f66155 | 00pilgrimage | A | 301 | 540 | Loss | 2000 |
| 85f66155 | 00reverse_spiral | A | 977 | 527 | Win | 2000 |
| e21af239 | 00elkring | A | 191 | 1001 | Loss | 720 |
| e21af239 | 00ambush | A | 857 | 655 | Win | 2000 |
| e21af239 | 01shortcut | A | 254 | 790 | Loss | 2000 |
| e21af239 | 00lanes | A | 124 | 1000 | Loss | 1088 |
| e21af239 | 01fracture | A | 743 | 1016 | Loss | 1812 |

## Repeated failures

| Metric across all 20 games | Us | Opponents |
| --- | ---: | ---: |
| Gems collected | 16,476 | 18,724 |
| Gems deposited | 9,559 | 15,342 |
| Gems lost | 5,842 | 2,708 |
| Gems carried at the end | 1,075 | 674 |
| Individual body deaths | 607 | 363 |
| Lifetime gold earned | 115,060 | 126,857 |
| Recall starts | 1,388 | 1,045 |
| Tribute starts | 356 | 609 |
| Tribute completions | 344 | 603 |
| Fuel suspensions | 542 | 1,070 |
| Recorded bot-error actions | 0 | 0 |

Our deposited/collected ratio was **58.0%**, versus **81.9%**. That ratio includes collected gems from champion kills, not only farm income.

### 1. Camp survival is a first-order problem

**305 of our 607 body deaths followed camp attacks; 302 followed champion actions.** Earlier replay samples were dominated by player damage. A bot that considers only its chosen camp group can miss damage from neighbouring awakened groups. Varg splash and ground spells can also wake multiple nearby groups. Survival must be evaluated against every visible awake attacker, and observations must account for groups just awakened by our own actions.

The bundled engine reports monolith attack range **5**, not the range **3** verified with the previous SDK. Range-four spell positions are no longer safe boss-farming positions. Clusters now have **120** health, and golems are available. The old balance assumptions change both farming efficiency and survival thresholds.

### 2. Navigation failures can remove an entire team from play

- On **01yellow_brick_road**, Cassi, Issac, Ryth and Tavi each remained on the same tile for **1,854 consecutive living end turns**, beginning at round 147. Varg remained stationary for 1,589 turns in a nearby position. We collected only 16 gems and scored zero. Stationary turns can be legitimate capture, channel or healing waits; these durations, low action counts and near-total loss of income establish a much longer stall.
- On **01boss_battle**, Ryth remained on one tile for **1,596** living end turns, starting at round 405. Varg later stood for 846 turns with a 131-gem bag. Neither team submitted gems; the opponent won the economy tiebreak.
- On **00lanes**, five of our six bodies had stationary runs between 248 and 712 turns; we lost 124–1000.

A development replay on the same map revealed another independently reproduced
failure: the camp was already cleared, but an opponent occupied another member
of its long connected capture region. Four bodies waited over 1,800 turns.
Instrumentation showed `state=reward available` and the progress timestamp
being refreshed every turn. A finite capture wait followed by a patrol is
necessary; a watchdog that treats waiting as progress cannot detect this.
This reproduction establishes the control-flow bug, not the exact hidden camp
state of every historical stall listed above.

The old path masks also treat occupied monster tiles as permanent obstacles. A monster sealing a corridor can make the chosen destination unreachable even though fighting through it is productive. Repeatedly selecting that same destination, without a progress watchdog or transport plan, does not resolve the blockage. This is a code-level failure consistent with the recorded stalls; an exact counterfactual outcome cannot be inferred from the replay.

### 3. Linked units multiply the cost of isolated routes

Cassi and Issac each died **142 times**, together accounting for **284/607** body deaths. Issac lost **1,569** carried gems and deposited 1,141 of 2,780 collected gems. His separate farming or retreat route can kill a healthy Cassi elsewhere, and vice versa. An extra body is not free capacity when the two bodies independently enter danger. Roster alternatives must be evaluated, rather than assuming either the linked pair or a particular replacement is always stronger.

### 4. Resource collection often fails to become score

On **01shortcut** in series `703e6f63`, we collected **1,792** gems to the opponent’s **1,197**, but deposited **127** to their **929** and lost **1,526** gems. Only one of our three tribute starts completed. Banking must compare altar routes, locks, exposed bags and the chance of surviving the channel; collection alone is insufficient.

On **01the_farm**, we collected 802 gems but deposited 298 and lost 504. On **01fracture**, 262 gems remained carried when we lost 743–1016. The final few turns and winning-bag calculations matter alongside ordinary delivery thresholds.

### 5. Recall frequency and cancellations consume productive time

We started **1,388 recalls** across the sample. This includes useful healing and interrupted attempts; it is not a count of completed recalls. Death, recall travel and repeated interruptions all reduce farming uptime. There were **168 manual channel cancellations** against seven by opponents; the action does not by itself distinguish recall from tribute, so it must not be described as 168 interrupted deposits. Current channels should be reassessed using remaining duration and whether damage actually interrupts that channel.

### 6. Combat must compare damage and position, not health totals alone

Raw health plus Focus is not a reliable engagement score. It can overvalue a large Focus pool on a kit that cannot immediately apply it, ignore burst availability or dash landing damage, and count support that cannot reach the fight. Healing on enemy spawn is an unproductive target. Pursuit should depend on reachable damage, cooldowns, stuns, nearby support, carried gems and retreat options.

A separate implementation hazard is choosing one spell target outside basic range, then attacking a camp because that specific target cannot be basic-attacked. Other enemies may already be in basic range. Basic-attack and spell target selection need separate range checks; combat should not awaken unrelated camps during an unfavourable duel.

### 7. Attack resets and spawn recovery can waste an entire fight

The old attack-reset scorer requires a prior basic attack. Ranged movement also
consumes the basic-attack allowance, so a ranged unit can walk into position,
reset, then attack; that useful sequence was excluded. Healing on allied spawn
also returned before combat. Waiting through turn end is required for recovery,
but it need not disable available attacks and spells. Enemy chip damage could
otherwise keep a damaged champion repeatedly waiting instead of defending.
These are code-level findings from the old control flow, not counts reconstructed
from the uploaded games.

### 8. Fuel and action boundaries need explicit handling

The replay engine records **542** of our fuel suspensions. Stdout can contain fewer completed spill messages, because a paused frame may never finish. Fuel exhaustion is a resumable pause, not inherently a crash. Expensive full-map searches, repeated occupancy masks and per-group geometry should be cached or amortised. After any calculation that can span a pause, the bot must recheck the round, life state, channel and action allowance immediately before acting.

## Replacement and measured validation

The replacement uses SDK/CLI **3.0.4** and organiser engine **1.0.2**. It drafts
independent durable bodies, establishes Jungle/Durability upgrades, handles
monster corridor blockages and legal island transport, bounds contested capture
waits, shields before retaliation, chooses reachable basic targets without
bypassing engagement filters, restores ranged attacks after movement when a
reset allows it, defends while healing on spawn, and banks smaller exposed bags.
Actual camp/ability metadata replace historical balance assumptions.

The same frozen source completed both sides of all **34 checked-in maps**
against three independent opponents: **55/68** wins against preserved v2,
**53/68** against caster pressure and **38/68** against corrected bruiser
pressure. On the same runtime and map seeds, v2 scored **47/68** and **27/68**
against those pressure fixtures. A source-ZIP extraction/rebuild completed
another 12 games. All **216 final games** had zero faults or caught API errors
on either team, with both gem accounts reconciled. Fuel suspensions remain a
measured limitation, especially on large maps.

See [the strategy](../../bots/baseline/README.md),
[full results and remaining losses](../../tests/results/v3/README.md), and
[the source bundle](../../submissions/v3-patched-sdk-bot.zip). The validation
manifest ties those reports and the ZIP to exact source hashes. Intermediate
trials and an early erroneous fixture were excluded from the final claims.
Local fixed-map comparisons identify improvements and regressions; they do not
predict a ranked win rate or Elo gain.
