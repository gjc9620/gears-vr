"""Reuse the existing full-span paired-image review and encode for this run."""
from pathlib import Path
HERE=Path(__file__).resolve().parent
source=(HERE.parent/'guard-witness-r137/review_r138.py').read_text(encoding='utf-8')
for old,new in (('runtime-r138','runtime-r142'),
                ('cover-1111-r138-cloak-guard','cover-1111-r142-cloak-discovery')):
    assert source.count(old)==1
    source=source.replace(old,new)
exec(compile(source,str(__file__),'exec'),globals())
