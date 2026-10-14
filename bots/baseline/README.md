# Baseline bot (C++, SDK 2.1.5)

Build and run from the repository root:

```sh
cambc build baseline
cambc run baseline starter-cpp --map packs
cambc view
```

The fixed draft picks Tavi, Vesper Vale, Stellar, Prism, then Varg, stopping at
this map's team size. Each champion scores camps using reachable walking
distance, remaining monster health, gold and gems, and estimated retaliation.
Monoliths are excluded. Nearby visible allies reduce a camp's desirability.
There are no team-wide assignments or shared-memory coordination yet.

The bot uses basic attacks, attack resets, damage/stun spells, area damage and
Varg's shield. It stands on a cleared region until capture and banks every haul.
It buys five Jungle Health shards first, then Monster Strength shards (Focus
for Stellar and Prism), with additional Health later. It shops while dead and
uses Tavi's field purchasing. It retreats before recalling when threatened and
waits on spawn for end-of-turn healing. It recalls with 150 unspent gold or
below 45% health. Enemy fights are opportunistic; no chasing is implemented.

Channels are left running. Pathfinding uses SDK distance fields and retries
alternative shortest steps after hidden obstructions. Main catches and logs
SDK exceptions so an invalid action does not terminate the process.

## Initial validation

The final version compiled and completed three full matches against starter-cpp
as Team A, with zero runner-reported crashes:

| Map | Baseline gems | Starter gems | Baseline deaths | Result |
| --- | ---: | ---: | ---: | --- |
| lanes | 282 | 1004 | 0 | Loss |
| packs | 740 | 1020 | 4 | Loss |
| corners | 66 | 1010 | 2 | Loss |

These single matches are smoke tests, not win-rate estimates. The bot currently
loses to the starter: camp selection, route persistence and team coordination
need improvement. It does not use wards, dashes, consumables or draft planning.
