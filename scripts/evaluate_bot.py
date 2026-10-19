#!/usr/bin/env python3
"""Run full release-mode matches on both sides and check replay diagnostics.

Example: python3 scripts/evaluate_bot.py --candidate baseline --opponent starter-cpp
Outputs go under ignored matches/. Win rate is against this opponent and map pool only.
"""
import argparse
import concurrent.futures
import json
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', default='baseline')
    parser.add_argument('--opponent', default='starter-cpp')
    parser.add_argument('--maps', nargs='+', help='Map names; default: every checked-in map')
    parser.add_argument('--jobs', type=int, default=min(4, os.cpu_count() or 1))
    parser.add_argument('--timeout', type=int, default=180)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    cli = shutil.which('cambc')
    if not cli:
        parser.error('cambc is not on PATH; activate the environment first')
    output = (args.output or ROOT / 'matches' / time.strftime('evaluation-%Y%m%d-%H%M%S')).resolve()
    output.mkdir(parents=True, exist_ok=False)
    candidate_root = Path(args.candidate)
    if not candidate_root.is_absolute():
        candidate_root = ROOT / 'bots' / candidate_root
    source_files = sorted((candidate_root / 'src').rglob('*'))
    source_hashes = {str(p.relative_to(candidate_root)): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in source_files if p.is_file()}
    maps = args.maps or sorted(p.stem for p in (ROOT / 'maps').glob('*.map26a'))
    if not maps or args.jobs < 1:
        parser.error('at least one map and one worker are required')
    for bot in dict.fromkeys([args.candidate, args.opponent]):
        subprocess.run([cli, 'build', bot, '--release'], cwd=ROOT, check=True)

    def match(pair):
        map_name, side = pair
        key = f'{map_name}-{side}'
        statistics = output / f'{key}.json'
        teams = [args.candidate, args.opponent]
        if side == 'B':
            teams.reverse()
        command = [cli, 'run', *teams, '--map', map_name, '--release', '--json',
                   '--statistics', str(statistics), '-o', str(output)]
        result = {'map': map_name, 'side': side, 'win': False, 'valid': False}
        try:
            completed = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT, timeout=args.timeout)
            (output / f'{key}.log').write_text(completed.stdout)
            result['exit_code'] = completed.returncode
            if completed.returncode or not statistics.is_file():
                result['error'] = 'runner failed or did not produce a completed match summary'
                return result
            summary = json.loads(statistics.read_text())
            team = 'teamA' if side == 'A' else 'teamB'
            other = 'teamB' if side == 'A' else 'teamA'
            result.update(
                win=summary['match']['outcome'] == team,
                outcome=summary['match']['outcome'],
                rounds=summary['match']['roundsStarted'],
                seed=summary['match']['seed'],
                gems={x['team']: int(x['submittedGems']) for x in summary['teams']},
                crashes=summary['diagnostics']['botFaults'][team],
                deaths=sum(int(x['metrics']['deaths']) for x in summary['champions'] if x['team'] == team),
            )
            for label, owner in [('our', team), ('enemy', other)]:
                champions = [x['metrics'] for x in summary['champions'] if x['team'] == owner]
                totals = {key: sum(int(c[key]) for c in champions)
                          for key in ['collectedGems', 'submittedGems', 'gems', 'deaths', 'kills']}
                totals['lostGems'] = sum(int(entry['value']) for event in summary['events']
                                        if event['kind'] == 'gemLoss' and event['owner'].get('team') == owner
                                        for entry in event['entries'])
                totals['conversion'] = (totals['submittedGems'] / totals['collectedGems']
                                        if totals['collectedGems'] else None)
                result[label] = totals
                if totals['collectedGems'] != totals['submittedGems'] + totals['gems'] + totals['lostGems']:
                    result['error'] = f'{label} gem accounting does not reconcile'
                    return result
            replay = re.search(r'replay bundle: (.+)', completed.stdout)
            if not replay:
                result['error'] = 'completed run has no replay bundle'
                return result
            with zipfile.ZipFile(replay[1].strip()) as archive:
                log = archive.read('team-a.stdout' if side == 'A' else 'team-b.stdout').decode(errors='replace')
            result['caught_errors'] = re.findall(r'bot error[^\n\x00]*', log)
            result['fuel_suspensions'] = sum(len(event['entries']) for event in summary['events']
                                             if event['kind'] == 'fuelExhausted' and event['owner'].get('team') == team)
            result['valid'] = not result['crashes'] and not result['caught_errors']
            print(f"{key}: {'WIN' if result['win'] else 'LOSS'} "
                  f"{result['gems'][team]}:{result['gems'][other]} "
                  f"round {result['rounds']} errors={len(result['caught_errors'])} "
                  f"crashes={result['crashes']}", flush=True)
        except (subprocess.TimeoutExpired, OSError, KeyError, ValueError, zipfile.BadZipFile) as error:
            result['error'] = str(error)
        return result

    pairs = [(name, side) for name in maps for side in ['A', 'B']]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        results = list(executor.map(match, pairs))
    report = {'candidate': args.candidate, 'opponent': args.opponent,
              'sdk': (ROOT / 'cambc.toml').read_text(), 'candidate_sources_sha256': source_hashes,
              'wins': sum(x['win'] for x in results), 'games': len(results),
              'invalid': sum(not x['valid'] for x in results), 'results': results}
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"{report['wins']}/{report['games']} wins; {report['invalid']} invalid checks. Report: {output / 'report.json'}")
    return int(report['invalid'] != 0)


if __name__ == '__main__':
    sys.exit(main())
