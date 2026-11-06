"""End-to-end CLI check: real input, gathering, NPC quest, shop, save and reload."""
import os,subprocess,tempfile,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build'/('aetherwild.exe' if os.name=='nt' else 'aetherwild')
with tempfile.TemporaryDirectory(prefix='aw_smoke_') as folder:
    target=str(Path(folder)/'hero.awc')
    commands=['accept first_gather windchime_guild']+['walk 5 0']*17
    for node in ['twig0','twig1','stone0','stone1']:
        commands += ['gather '+node,'wait 2','harvest '+node]
    commands+=['walk -5 0']*17+['quest first_gather windchime_guild','buy potion 1 windchime_merchant','save '+target,'load '+target,'status','quit']
    result=subprocess.run([str(exe)],input='\n'.join(commands)+'\n',capture_output=True,text=True,encoding='utf-8',cwd=folder,check=True)
    if '操作未完成' in result.stdout or 'XP 80' not in result.stdout or '金幣 70' not in result.stdout or not Path(target).is_file():
        raise RuntimeError(result.stdout+result.stderr)
    print('CLI SMOKE OK: walk -> gather 4 -> quest -> buy potion -> save -> load')
