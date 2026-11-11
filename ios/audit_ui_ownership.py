"""Inventory every generated function's UI calls without running the game.

The inventory is a structural audit, not a claim that every screen was viewed.
Run on the private Actions runner; generated game code stays private.
"""
from pathlib import Path
import json
import re
import sys

root=Path(sys.argv[1])
functions={}
for path in root.glob('*.c'):
    text=path.read_text()
    for match in re.finditer(r'void fn_([0-9A-F]+)\(CPU\* c\) \{(.*?)(?=\n/\* |\Z)',text,re.S):
        name,body=match.groups()
        if name in functions:raise SystemExit('Duplicate function '+name)
        functions[name]={
            'calls':sorted(set(re.findall(r'fn_([0-9A-F]+)\(c\)',body))),
            'indirect':body.count('call_indirect('),
            'native_scope':'fzgx_native_canvas_begin(' in body,
            'sprite_scope':'fzgx_ui_sprite_begin()' in body,
            'queue_record':'fzgx_ui_queue_record(' in body,
            'queue_restore':'fzgx_native_canvas_end(queueTag)' in body,
        }
if len(functions)<7000:raise SystemExit('Incomplete game function inventory')
def callers(api):return sorted(n for n,f in functions.items() if api in f['calls'])
# These are the actual original window construction APIs, not texture guesses.
apis={
    '8184D0A0':'text bounds begin',
    '8184D494':'framed text bounds begin',
    '8184D10C':'text bounds clip finish',
    '8184D2AC':'card window frame',
    '8184D4C8':'text-fitted window frame',
    '8184A0D8':'shared font',
    '8184F734':'ordinary/immediate sprite submission',
    '8184FC50':'alternate sprite queue',
    '8184FD64':'sorted UI queue consumption',
}
for name in ('8184D10C','8184D2AC','8184D4C8'):
    if not functions[name]['native_scope']:raise SystemExit('Untagged window finish '+name)
for name in ('8184F734','8184FC50'):
    if not functions[name]['sprite_scope'] or not functions[name]['queue_record']:
        raise SystemExit('Window ownership missing from sprite path '+name)
if not functions['8184FD64']['queue_restore']:raise SystemExit('Missing queue ownership restoration')
for name in ('818B6B28','818B8EDC','818BC310'):
    if not functions[name]['native_scope']:raise SystemExit('Missing actual card compositor '+name)
report={
    'functions':len(functions),
    'direct_call_edges':sum(len(f['calls']) for f in functions.values()),
    'functions_with_indirect_calls':sum(bool(f['indirect']) for f in functions.values()),
    'native_scope_functions':sorted(n for n,f in functions.items() if f['native_scope']),
    'ui_apis':{n:{'purpose':purpose,'direct_callers':callers(n)} for n,purpose in apis.items()},
    'limitations':['Indirect dispatch and state-dependent screen reachability require runtime evidence.','This inventory does not assert whole-game visual validation.'],
}
print(json.dumps(report,indent=2))
