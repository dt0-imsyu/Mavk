import json
from pathlib import Path

folder = Path('/root/.local/share/droiddeck-compat')
state = json.loads((folder / 'state.json').read_text())
tool = next(t for t in state['tools'] if t['dir'] == 'Proton Experimental (ARM64)' and t['valve'])
path = folder / 'games.json'
backup = folder / 'games.json.before-muck-test'
if not backup.exists():
    backup.write_text(path.read_text() if path.exists() else '{"version":1,"games":{}}\n')
body = json.loads(path.read_text()) if path.exists() else {'version': 1, 'games': {}}
assert body['version'] == 1 and isinstance(body['games'], dict)
body['games']['1625450'] = {'valve': tool['valve'], 'dir': tool['dir']}
staged = folder / 'games.json.staged'
staged.write_text(json.dumps(body, indent=2) + '\n')
staged.replace(path)
print(json.dumps({'appId': 1625450, 'tool': tool['name'], 'rollback': str(backup)}))
