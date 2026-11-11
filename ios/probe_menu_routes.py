"""Private, repeatable menu routes through the real generated game code.

Captures contain decoded game inputs and must be encrypted before upload.
This script supplies ordinary controller events; it does not patch guest state.
"""
from pathlib import Path
import os
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor

root=Path.cwd()
boot="1440:START:5,"
routes={
    "practice":"2200:DOWN:5,2400:A:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3600:A:5,3800:A:1200",
    "timeattack":"2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3600:A:5,3800:A:1200",
    "gp":"2200:A:5,2400:A:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3700:A:5,3800:A:1600",
    "records":"2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:DOWN:5,3000:A:5,3200:A:5,3400:A:5",
    "records-right":"2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:RIGHT:5,3000:A:5,3200:A:5,3400:A:5",
    "records-sort":"2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:DOWN:5,3000:A:5,3200:A:5,3400:A:5,4000:X:5",
    "records-pilot":"2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:DOWN:5,3000:A:5,3200:A:5,3400:A:5,4000:Z:5",
    "returnlogo":"2200:B:5",
    "options":"2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:A:5",
    "story":"2200:DOWN:5,2400:RIGHT:5,2600:A:5,2800:A:5",
    "replay":"2200:DOWN:5,2400:RIGHT:5,2600:RIGHT:5,2800:A:5",
    "profile":"2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:RIGHT:5,3000:A:5,3400:A:5",
    "profile-x":"2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:RIGHT:5,3000:A:5,3400:A:5,4000:X:5",
    "profile-y":"2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:RIGHT:5,3000:A:5,3400:A:5,4000:Y:5",
    "profile-z":"2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:RIGHT:5,3000:A:5,3400:A:5,4000:Z:5",
    "customize":"2200:DOWN:5,2400:DOWN:5,2600:A:5,3000:A:5",
    "shop":"2200:DOWN:5,2400:DOWN:5,2600:A:5,3000:RIGHT:5,3200:RIGHT:5,3400:A:5,3800:A:5",
}
# Cover mode/subscreen families with ordinary inputs. Route names describe
# intended input, not proof of the resulting state: audit owner/phase metadata.
base_options=routes["options"]
for name,extra in {
    "controller":"3200:A:5,3600:DOWN:5,4000:A:5",
    "memory":"3200:DOWN:5,3400:A:5,4000:A:5",
    "screen":"3200:DOWN:5,3400:DOWN:5,3600:A:5",
    "wide":"3200:DOWN:5,3400:DOWN:5,3600:A:5,4000:RIGHT:5,4200:A:5,4500:B:5,4800:B:5",
}.items():routes["options-"+name]=base_options+","+extra
routes["records-clear"]=routes["records"]+",4000:Y:5,4300:B:5"
routes["records-page"]=routes["records"]+",4000:RIGHT:5,4300:DOWN:5,4600:DOWN:5,4900:LEFT:5"
start="2200:RIGHT:5,2400:A:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3600:A:5,3800:A:1800"
for players in (2,3,4):routes[f"versus-{players}"]=start
for mode in ("practice","timeattack","gp"):
    routes[mode+"-pause"]=routes[mode]+",5000:START:5,6200:START:5"
    routes[mode+"-retire"]=routes[mode]+",5000:START:5,5300:DOWN:5,5500:DOWN:5,5700:A:5,6000:LEFT:5,6200:A:5,6700:A:5"
# Alternative cup/course selection, machine settings and course-intro variants.
for n in range(1,5):
    events="2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,"
    events+=",".join(f"{3400+j*70}:RIGHT:5" for j in range(n))
    events+=",3800:A:5,4000:A:5,4200:A:1800"
    routes[f"timeattack-course-{n}"]=events
routes["story-play"]=routes["story"]+",3400:START:5,3800:A:5,4200:A:5,4600:A:5,5000:START:5,5400:A:5,6000:A:1200"
routes["boot-card"]=""
routes["boot-card-empty"]=""
routes["replay-card"]=routes["replay"]+",3400:A:5,3800:DOWN:5,4200:A:5,4600:B:5"
for i in range(4):
    extra=",".join(f"{3200+j*100}:RIGHT:5" for j in range(i))
    routes[f"garage-{i}"]="2200:DOWN:5,2400:DOWN:5,2600:A:5,"+extra+("," if extra else "")+"3800:A:5,4200:A:5,4600:RIGHT:5,5000:A:5"
# Every pilot-selection index is exercised. The size issue remains closed;
# these routes inspect shared text, icons and clipping around the original art.
for pilot in range(30):
    events="2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:RIGHT:5,3000:A:5,"
    events+=",".join(f"{3100+j*50}:RIGHT:5" for j in range(pilot))
    events+=("," if pilot else "")+"4800:A:5,5200:X:5"
    routes[f"pilot-index-{pilot:02}"]=events

selected=os.environ.get("FZGX_MENU_ROUTES")
if not selected:
    request=root/"ios/private-build-request.txt"
    if request.exists():
        selected=next((line[len("routes="):] for line in request.read_text().splitlines() if line.startswith("routes=")),None)
if selected:
    names=selected.split(",")
    unknown=set(names)-routes.keys()
    if unknown:raise SystemExit(f"Unknown private menu routes: {sorted(unknown)}")
    routes={name:routes[name] for name in names}

def run_route(item):
    name,events=item
    directory=root/("build-probe-menu-"+name)
    directory.mkdir(exist_ok=True)
    seed = root / 'rom/memcard_a.raw'
    if seed.is_file() and name!="boot-card-empty":
        saves = directory / 'saves'
        saves.mkdir(exist_ok=True)
        shutil.copyfile(seed, saves / 'memcard_a.raw')
    # The original menu reads stick navigation; include it with each intended
    # direction instead of assuming D-pad presses reached the target screen.
    prefix="500:A:5,1300:A:5,1340:A:5,1380:A:5,1440:START:5,1500:A:5,1580:LEFT:5,1620:A:5,1800:A:5,2000:START:5," if name=="gp-pause" else boot
    navigation=prefix+events
    for direction,stick in (("LEFT","SL"),("RIGHT","SR"),("UP","SU"),("DOWN","SD")):
        navigation=navigation.replace(":"+direction+":",":"+direction+"+"+stick+":")
    env=os.environ.copy()
    env["FZGX_TRACE_UI_QUEUES"]="1"
    env["FZGX_TRACE_UI_TEXT"]="1"
    env.update(FZGX_PRIVATE_CAPTURE="1",FZGX_MENU_PROBE="1",FZGX_CAPTURE_METADATA="1",FZGX_PROBE_END_FRAME="6500",FZGX_CAPTURE_FRAMES="2300,2500,2700,2801,2810,2850,2900,3100,3300,3500,3700,3900,4000,4001,4002,4003,4200,4500,4800,5000,5200,5400",GCN_INPUT=navigation)
    if name.startswith("boot-card"):
        env["FZGX_CAPTURE_FRAMES"]=",".join(map(str,range(30,2001,30)))+","+env["FZGX_CAPTURE_FRAMES"]
    if name=="profile":
        early=",".join(map(str,[3020,3040,3050,3060,3065,3070,3080,3090,3100,3110,3120,3130,3140,3150,3160,3180,3200,3220,3240]))
        env["FZGX_CAPTURE_FRAMES"]=early+","+env["FZGX_CAPTURE_FRAMES"]
    if name.startswith("pilot-index-"):
        env["FZGX_CAPTURE_FRAMES"]="3100,4500,4850,4900,5100,5250,5400,5700,6000,6400"
    if name.startswith("versus-"):
        env["FZGX_PROBE_PLAYERS"]=name.rsplit("-",1)[1]
    if name.startswith("records"):
        env["FZGX_CAPTURE_FRAMES"]="2900,3100,3300,3500,3700,3900,4000,4001,4002,4003,4020,4050,4100,4200,4500,4800,5000,5200,5400"
    if name=="replay":env["FZGX_CAPTURE_FRAMES"]="30,60,120,180,240,360,480,600,1000,1900,"+env["FZGX_CAPTURE_FRAMES"]
    if name.endswith("-pause") or name.endswith("-retire") or name=="story-play":
        env["FZGX_CAPTURE_FRAMES"]="4800,4999,5000,5001,5002,5020,5100,5300,5500,5700,5900,6100,6199,6200,6201,6202,6300,6500,6800,7000,7200,7400"
        env["FZGX_PROBE_END_FRAME"]="7500"
    with (root/"build-private-logs"/("menu-"+name+".txt")).open("w") as log:
        subprocess.run([str(root/"build-host/fzerogx_bench"),"--seconds=140","--warmup=0",str(root/"rom/game.iso")],cwd=directory,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
    print("Completed private game UI route:",name,flush=True)

with ThreadPoolExecutor(max_workers=2) as pool:
    list(pool.map(run_route,routes.items()))



