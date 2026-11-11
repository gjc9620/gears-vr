"""Use existing paired full-span and interval-bounded contact review."""
from pathlib import Path
HERE=Path(__file__).resolve().parent
source=(HERE.parent/'guard-witness-r137/review_r138.py').read_text(encoding='utf-8')
for old,new in (('runtime-r138','runtime-r164'),('cover-1111-r138-cloak-guard','cover-1111-r164-cloak-timing'),
    ('contacts=contacts,encode_exit=encode.returncode)))','contacts=len(contacts),encode_exit=encode.returncode)))')):
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(__file__),'exec'),globals())
