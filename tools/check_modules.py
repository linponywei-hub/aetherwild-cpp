"""Check module boundaries, local imports, entry points and cleanup invariants."""
from pathlib import Path
import json
import re
import subprocess

root = Path(__file__).resolve().parents[1]
web = root / 'web'
checks = 0
for source in web.rglob('*.js'):
    subprocess.run(['node', '--check', str(source)], check=True, capture_output=True)
    for spec in re.findall(r'''(?:from\s*|import\s*\(\s*|import\s*)['"]([^'"]+)['"]''', source.read_text()):
        if not spec.startswith(('.', '/')):
            continue
        clean = spec.split('?')[0]
        target = web / clean.lstrip('/') if clean.startswith('/') else source.parent / clean
        assert target.resolve().is_relative_to(web.resolve()), (source, spec, 'outside web')
        assert target.is_file(), (source, spec, 'missing module')
        checks += 1
for source in web.glob('*.html'):
    for spec in re.findall(r'''(?:src|href)=["']([^"']+\.(?:js|css)(?:\?[^"']*)?)["']''', source.read_text()):
        clean = spec.split('?')[0]
        target = web / clean.lstrip('/') if clean.startswith('/') else source.parent / clean
        assert target.is_file(), (source, spec, 'missing page dependency')
        checks += 1
for source in [*(root / 'src').rglob('*.cpp'), *(root / 'src').rglob('*.hpp'), *(root / 'include').rglob('*.hpp')]:
    for name in re.findall(r'#include\s+"([^"]+)"', source.read_text()):
        target = root / 'include' / name if name.startswith('aetherwild/') else source.parent / name
        assert target.is_file(), (source, name, 'missing header')
        checks += 1
assert not list((root / 'src').glob('*.cpp')), 'Native code must live in a feature module or generated/'
assert not list(web.glob('*.js')), 'Browser code must live in modules/ or vendor/'
assert not list(root.glob('*.glb')), 'Root models must use canonical runtime or source-asset locations'
cleanup = json.loads((root / 'docs/architecture/cleanup.json').read_text())
for entry in cleanup['removed']:
    assert not (root / entry['path']).exists(), entry['path']
    if entry.get('canonicalRetired'):
        assert not (root / entry['canonical']).exists(), entry['canonical']
    elif 'canonical' in entry:
        assert (root / entry['canonical']).is_file(), entry['canonical']
print(f'MODULE CHECK OK: {checks} dependency paths; {len(cleanup["removed"])} cleaned files')
