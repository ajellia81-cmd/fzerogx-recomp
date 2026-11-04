"""Private, repeatable menu routes through the real generated game code.

Captures contain decoded game inputs and must be encrypted before upload.
This script supplies ordinary controller events; it does not patch guest state.
"""
from pathlib import Path
import os
import subprocess

root=Path.cwd()
boot="500:A:5,1300:A:5,1340:A:5,1380:A:5,1440:START:5,1500:A:5,1580:LEFT:5,1620:A:5,1800:A:5,2000:START:5,"
routes={
    "replay":"2200:DOWN:5,2400:RIGHT:5,2600:RIGHT:5,2800:A:5",
    "profile":"2200:DOWN:5,2400:DOWN:5,2600:RIGHT:5,2800:RIGHT:5,3000:A:5,3400:A:5",
    "customize":"2200:DOWN:5,2400:DOWN:5,2600:A:5,3000:A:5",
    "shop":"2200:DOWN:5,2400:DOWN:5,2600:A:5,3000:RIGHT:5,3200:RIGHT:5,3400:A:5,3800:A:5",
}
for name,events in routes.items():
    directory=root/("build-probe-menu-"+name)
    directory.mkdir(exist_ok=True)
    env=os.environ.copy()
    env.update(FZGX_PRIVATE_CAPTURE="1",FZGX_MENU_PROBE="1",FZGX_CAPTURE_FRAMES="2700,2801,2810,2850,2900,3100,3300,3500,3700,4000,4001,4002,4003,4500",GCN_INPUT=boot+events)
    with (root/"build-private-logs"/("menu-"+name+".txt")).open("w") as log:
        subprocess.run([str(root/"build-host/fzerogx_bench"),"--seconds=65","--warmup=0",str(root/"rom/game.iso")],cwd=directory,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
    print("Completed private menu route:",name,flush=True)
