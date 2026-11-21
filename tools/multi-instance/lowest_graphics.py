"""Low-load graphics for fresh private instances; never shared user settings."""
import re

SYSTEM = dict(MotionBlur='False', DepthOfField='False', Bloom='False',
              QualityBloom='False', LensFlares='False', DynamicDecals='False',
              DynamicShadows='False', DYNAMICSHADOWSPLITSCREENCAMPAIGN='False',
              LightEnvironmentShadows='False', MaxAnisotropy='1',
              SSAOQualityLevel='0', Cacao='False', CACAOQualityLevel='0',
              CACAOQualityLevelSplitScreen='0', SSRQUALITY='0', ENABLESSR='False',
              ENABLEPCSS='False', LIGHTSHAFTQUALITY='0', AllowHDR='False',
              FORCEMOTIONBLUR='False', FOGVOLUMES='False', ALLOWHBAO='False',
              DETAILMODE='0', MINSHADOWRESOLUTION='32', MINPRESHADOWRESOLUTION='8',
              MAXSHADOWRESOLUTION='256', MAXWHOLESCENEDOMINANTSHADOWRESOLUTION='256',
              SkeletalMEshLODBias='1', ParticleLODBias='2', DLSS='False',
              FSR2='False', FSR3='False')


def transform(raw, desktop_size=(800,600)):
    encoding = 'utf-16' if raw.startswith(b'\xff\xfe') else 'utf-8-sig' if raw.startswith(b'\xef\xbb\xbf') else 'utf-8'
    text = raw.decode(encoding)
    section = None
    changed = []
    output = []
    for line in text.splitlines(keepends=True):
        header = re.match(r'^\[([^]]+)\]', line)
        if header:
            section = header[1].casefold()
        match = re.match(r'^([^=;\r\n]+)=([^\r\n]*)(\r?\n)?$', line)
        if match:
            key, old, ending = match[1], match[2], match[3] or ''
            new = old
            if section == 'systemsettings':
                choices = {k.casefold(): v for k, v in SYSTEM.items()}
                new = choices.get(key.casefold(), old)
                if key.upper().startswith('TEXTUREGROUP_') and not any(x in key.upper() for x in ('RENDERTARGET', 'UI', 'COLORLOOKUPTABLE')):
                    new = re.sub(r'MaxLODSize=(\d+)', lambda m: 'MaxLODSize='+str(min(256, int(m[1]))), new)
                    new = re.sub(r'LODBias=\d+', 'LODBias=2', new)
                    new = re.sub(r'MaxAnisotropy=\d+', 'MaxAnisotropy=1', new)
            elif section == 'clientsettings' and re.fullmatch(r'(?:Hi|Low|Rec)?StartupResolution[XY]', key):
                new = str(desktop_size[0] if key.endswith('X') else desktop_size[1])
            if new != old:
                changed.append(dict(section=section, key=key, before=old, after=new))
                line = key+'='+new+ending
        output.append(line)
    if not any(re.search(r'^'+re.escape(key)+r'=', text, re.M | re.I) for key in SYSTEM):
        raise ValueError('No recognized graphics settings exist')
    return ''.join(output).encode(encoding), changed
