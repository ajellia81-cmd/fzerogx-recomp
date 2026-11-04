"""Private, repeatable menu routes through the real generated game code.

Captures contain decoded game inputs and must be encrypted before upload.
This script supplies ordinary controller events; it does not patch guest state.
"""
from pathlib import Path
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor

root=Path.cwd()
boot="500:A:5,1300:A:5,1340:A:5,1380:A:5,1440:START:5,1500:A:5,1580:LEFT:5,1620:A:5,1800:A:5,2000:START:5,"
routes={
    "practice":"2200:DOWN:5,2400:A:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3600:A:5,3800:A:1200",
    "timeattack":"2200:RIGHT:5,2400:RIGHT:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3600:A:5,3800:A:1200",
    "gp":"2200:A:5,2400:A:5,2600:A:5,2800:A:5,3000:A:5,3200:A:5,3400:A:5,3700:A:5,3800:A:1600",
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
def run_route(item):
    name,events=item
    directory=root/("build-probe-menu-"+name)
    directory.mkdir(exist_ok=True)
    env=os.environ.copy()
    env.update(FZGX_PRIVATE_CAPTURE="1",FZGX_MENU_PROBE="1",FZGX_CAPTURE_FRAMES="2300,2500,2700,2801,2810,2850,2900,3100,3300,3500,3700,3900,4000,4001,4002,4003,4200,4500,4800,5000,5200,5400",GCN_INPUT=boot+events)
    if name=="replay":env["FZGX_CAPTURE_FRAMES"]="30,60,120,180,240,360,480,600,1000,1900,"+env["FZGX_CAPTURE_FRAMES"]
    with (root/"build-private-logs"/("menu-"+name+".txt")).open("w") as log:
        subprocess.run([str(root/"build-host/fzerogx_bench"),"--seconds=65","--warmup=0",str(root/"rom/game.iso")],cwd=directory,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
    print("Completed private menu route:",name,flush=True)

with ThreadPoolExecutor(max_workers=2) as pool:
    list(pool.map(run_route,routes.items()))

