#!/usr/bin/env node
// Offline analysis using the organiser's version-matched visualiser engines.
// Usage: node scripts/review_ranked_replays.mjs INPUT_DIR OUTPUT.json ASSETS_DIR
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {WASI} from 'node:wasi';
const [input,output,assets]=process.argv.slice(2);
if(!input||!output||!assets)throw Error('Provide input directory, output JSON and visualiser assets directory');
const layout=await import(path.resolve(assets,fs.readdirSync(assets).find(x=>/^PanelChrome-.*\.js$/.test(x))));
const championNames=layout.R, actions=layout.L;
const json=x=>JSON.stringify(x,(_,v)=>typeof v==='bigint'?Number(v):v,2);
const gap=(a,b)=>a&&b?Math.max(Math.abs(a.x-b.x),Math.abs(a.y-b.y)):999;
function files(dir){return fs.readdirSync(dir,{withFileTypes:true}).flatMap(e=>e.isDirectory()?files(path.join(dir,e.name)):[path.join(dir,e.name)]);}
const modules=new Map();
async function engine(version){
 let module=modules.get(version);
 if(!module){for(const f of fs.readdirSync(assets).filter(x=>x.endsWith('.wasm'))){const m=await WebAssembly.compile(fs.readFileSync(path.join(assets,f)));const wasi=new WASI({version:'preview1',args:[],env:{}});const {exports:ex}=await WebAssembly.instantiate(m,{...wasi.getImportObject(),deep_wasm:{on_assert_fail_impl(){throw Error('Engine assertion');}}});wasi.initialize({exports:ex});if(Number(ex.Version())===version){module=m;modules.set(version,module);break;}}}
 if(!module)throw Error(`No engine for packed version ${version}`);
 const wasi=new WASI({version:'preview1',args:[],env:{}});const instance=await WebAssembly.instantiate(module,{...wasi.getImportObject(),deep_wasm:{on_assert_fail_impl(){throw Error('Engine assertion');}}});wasi.initialize(instance);const ex=instance.exports;
 const out=ex.Engine_Malloc(128),persp=ex.Engine_Malloc(16),snap=ex.Engine_Malloc(ex.Game_Visualiser_SnapshotSize()),timelineSnap=ex.Engine_Malloc(ex.Game_Visualiser_TimelineSnapshotSize());
 const dv=()=>new DataView(ex.memory.buffer);dv().setUint32(persp,2,true);dv().setUint32(persp+4,0xffffffff,true);
 function check(c){if(c){const ptr=ex.GetLastErrorMsg();const bytes=new Uint8Array(ex.memory.buffer);let end=ptr;while(bytes[end])end++;throw Error(`${c}: ${new TextDecoder().decode(bytes.subarray(ptr,end))}`);}}
 function load(bytes,replay){const p=ex.Engine_Malloc(bytes.length);new Uint8Array(ex.memory.buffer,p,bytes.length).set(bytes);check(replay?ex.Game_FromReplayFileBytes(p,bytes.length,out):ex.Game_FromMapFileBytes(p,bytes.length,out,0));const h=dv().getBigUint64(out,true);ex.Engine_Free(p);return h;}
 function full(h){check(ex.Game_Visualiser_ReadState(h,persp,snap));return layout.F(dv(),snap);}
 function state(h){check(ex.Game_Visualiser_ReadTimelineState(h,timelineSnap));return layout.I(dv(),timelineSnap);}
 function fingerprint(h){check(ex.Game_GetBitboardMetadata(h,out));const v=dv(),width=v.getInt32(out,true),height=v.getInt32(out+4,true),size=v.getUint32(out+12,true)*v.getUint32(out+16,true)*8;const hash=crypto.createHash('sha256');hash.update(`${width},${height}`);for(const f of ['Game_GetWallBuffer','Game_GetHoleBuffer','Game_GetBushBuffer','Game_GetAltarBuffer']){check(ex[f](h,out));hash.update(new Uint8Array(ex.memory.buffer,dv().getUint32(out,true),size));}return {width,height,hash:hash.digest('hex')};}
 return {ex,out,check,load,full,state,fingerprint,dv};
}
const reports=[];
for(const file of files(input).filter(x=>x.endsWith('.repl26a')).sort()){
 const bytes=fs.readFileSync(file),version=bytes.readUInt32LE(8),e=await engine(version),{ex,out,dv,check}=e,h=e.load(bytes,true),map=e.fingerprint(h);
 let mapName='unknown';for(const p of files('maps').filter(x=>x.endsWith('.map26a'))){const mh=e.load(fs.readFileSync(p),false);if(e.fingerprint(mh).hash===map.hash)mapName=path.basename(p,'.map26a');check(ex.Game_Destroy(mh));}
 check(ex.Game_Visualiser_Timeline_GetMain(h,out));const tl=dv().getBigUint64(out,true);check(ex.Game_Visualiser_Timeline_Info(h,tl,out,out+8,out+16));const size=dv().getUint32(out,true);
 check(ex.Game_Visualiser_Timeline_Seek(h,tl,size,0));const final=e.full(h);check(ex.Game_Visualiser_Timeline_Seek(h,tl,0,0));
 const sideLogs=['team-a.stdout','team-b.stdout'].map(name=>fs.existsSync(path.join(path.dirname(file),name)));
 if(sideLogs[0]===sideLogs[1])throw Error(`Exactly one private team stdout is required to identify our side: ${file}`);
 const own=sideLogs[0]?0:1;
 const stats=new Map(),events=[],deaths=[],turns=[],rounds=[],deathCauses={},samples=[];
 let prev=e.state(h),steps=0,lastSample=-100;
 const fresh=c=>({id:c.entity.index,name:championNames[c.type],team:c.team,collected:0,deposited:0,lost:0,deaths:0,recalls:0,submissions:0,completedSubmissions:0,cancelledChannels:0,fuel:0,faults:0,attacks:0,casts:0,moves:0,blocked:0,aliveTurns:0,idleTurns:0,maxIdleStreak:0,idleStreak:0,maxCarried:0,actions:{},lastTurnRound:-1,lastTurnActions:0,positions:[],lastAlivePosition:null});
 while(prev.phase!==11){
  check(ex.Game_Visualiser_Timeline_ReplayNext(h,1));const now=e.state(h);steps++;
  if(steps>2000000)throw Error('Replay failed to terminate');
  const old=new Map(prev.champions.map(c=>[c.entity.index,c]));
  for(const c of now.champions){const id=c.entity.index;let s=stats.get(id);if(!s){s=fresh(c);stats.set(id,s);}const p=old.get(id);if(!p)continue;
   if(c.gems>p.gems)s.collected+=c.gems-p.gems;
   if(c.gems<p.gems){const deposited=now.submittedGems[c.team]>prev.submittedGems[c.team];if(deposited){s.deposited+=p.gems-c.gems;s.completedSubmissions++;events.push({kind:'deposit',round:now.round,id,name:s.name,gems:p.gems-c.gems});}else{s.lost+=p.gems-c.gems;}}
   s.maxCarried=Math.max(s.maxCarried,c.gems);
   if(c.alive&&c.position)s.lastAlivePosition=c.position;
   if(p.alive&&!c.alive){s.deaths++;const killer=old.get(now.step.actor.index);const cause=now.step.actor.kind===1?(killer?.team===c.team?'linked/self':'champion'):now.step.actor.kind===2?'camp':now.step.actor.kind===4?'spell':'other';deathCauses[cause]=(deathCauses[cause]??0)+1;
    const foes=prev.champions.filter(o=>o.team!==c.team&&o.alive&&gap(o.position,p.position)<=6),allies=prev.champions.filter(o=>o.team===c.team&&o.entity.index!==id&&o.alive&&gap(o.position,p.position)<=6);
    deaths.push({round:now.round,id,name:s.name,team:c.team,gems:p.gems,position:p.position,healthBefore:p.health,maxHealth:p.maxHealth,cause,action:actions[now.step.kind],actor:now.step.actor,allies:allies.map(o=>championNames[o.type]),enemies:foes.map(o=>({name:championNames[o.type],gap:gap(o.position,p.position),health:o.health}))});
   }
  }
  if(now.step.actor.kind===1){const id=now.step.actor.index,s=stats.get(id),c=now.champions.find(x=>x.entity.index===id),k=now.step.kind;
   if(s){s.actions[actions[k]]=(s.actions[actions[k]]??0)+1;if(k===4)s.moves++;if(k===5)s.attacks++;if(k===6)s.casts++;if(k===11)s.recalls++;if(k===12)s.submissions++;if(k===16)s.cancelledChannels++;if(k===23)s.blocked++;if(k===26)s.fuel++;if(k===27)s.faults++;
    if([4,5,6,11,12,15,20,21,25].includes(k))s.lastTurnActions++;
    if([8,26,27].includes(k)&&now.step.substep===0&&c?.alive){s.aliveTurns++;const active=s.lastTurnActions>0;s.lastTurnActions=0;const channel=prev.champions.find(x=>x.entity.index===id);if(!active){s.idleTurns++;s.idleStreak++;s.maxIdleStreak=Math.max(s.maxIdleStreak,s.idleStreak);}else s.idleStreak=0;
     const p=s.positions.at(-1);if(p&&p.x===c.position?.x&&p.y===c.position?.y)p.n++;else s.positions.push({...c.position,n:1,round:now.round,gems:c.gems});
    }
   }
  }
  if(now.round>=lastSample+50&&now.phase!==12){lastSample=now.round;rounds.push({round:now.round,score:now.submittedGems,champions:now.champions.map(c=>({id:c.entity.index,name:championNames[c.type],team:c.team,alive:c.alive,hp:Math.round(c.health),maxhp:c.maxHealth,gold:c.gold,gems:c.gems,position:c.position}))});}
  if(now.inputPosition===prev.inputPosition&&now.step.substep===prev.step.substep&&now.phase===prev.phase)throw Error('No replay progress');
  prev=now;
 }
 for(const c of final.champions){const s=stats.get(c.entity.index);if(!s)continue;s.remaining=c.gems;s.earnedGold=c.earnedGold;s.kills=c.kda[0];s.finalItems=c.items;s.longestStands=s.positions.filter(p=>p.n>=15).sort((a,b)=>b.n-a.n).slice(0,10);delete s.positions;delete s.lastTurnActions;delete s.lastTurnRound;s.reconciled=s.collected===s.deposited+s.lost+s.remaining;}
 const teamTotals=[0,1].map(team=>Object.fromEntries(['collected','deposited','lost','remaining','deaths','fuel','faults','earnedGold','attacks','casts','recalls','submissions','completedSubmissions','cancelledChannels'].map(k=>[k,[...stats.values()].filter(s=>s.team===team).reduce((n,s)=>n+(s[k]??0),0)])));
 const report={bundle:path.basename(path.dirname(path.dirname(file))),match:path.basename(path.dirname(file)),replaySHA256:crypto.createHash('sha256').update(bytes).digest('hex'),version,engine:`v${version&255}_${(version>>>8)&255}_${version>>>16}`,map:mapName,geometry:map,ourSide:own,rounds:final.round,outcome:final.outcome,win:final.outcome===own+1,scores:final.submittedGems,inputs:size,steps,drafts:final.drafts.map(d=>d.map(x=>layout.z[x])),totals:teamTotals,champions:[...stats.values()],deaths,events,samples:rounds};
 if(!report.champions.every(s=>s.reconciled))throw Error(`Gem reconciliation failed ${file}`);
 for(const t of [0,1])if(teamTotals[t].deposited!==Number(final.submittedGems[t]))throw Error('Score reconciliation failed');
 reports.push(report);fs.writeFileSync(output,json(reports)+'\n');console.log(mapName,own===0?'A':'B',report.win?'WIN':'LOSS',final.submittedGems.map(Number).join(':'),'round',final.round,'own',teamTotals[own]);check(ex.Game_Destroy(h));
}
