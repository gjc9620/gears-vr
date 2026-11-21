"""Build a signed native Quest APK using isolated, pinned local Android tools."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile

HERE = Path(__file__).resolve().parent


def build(dependencies, output):
    output.mkdir(parents=True, exist_ok=True)
    ndk = dependencies/'ndk/android-ndk-r27c'
    tools = dependencies/'build-tools/android-15'
    android = dependencies/'platform/android-35/android.jar'
    aar = dependencies/'openxr'
    cmake = Path('E:/CMake/bin/cmake.exe')
    commands = []

    def run(name, args):
        commands.append(dict(stage=name, arguments=[str(x) for x in args]))
        with (output/(name+'.log')).open('w', encoding='utf-8') as log:
            result = subprocess.run([str(x) for x in args], stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError(f'{name} failed ({result.returncode}); see {output/(name+".log")}')

    run('configure', [cmake, '-S', HERE, '-B', output/'native', '-G', 'Ninja',
                      '-DCMAKE_MAKE_PROGRAM=E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe',
                      '-DCMAKE_TOOLCHAIN_FILE='+str(ndk/'build/cmake/android.toolchain.cmake'),
                      '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-29', '-DCMAKE_BUILD_TYPE=Release',
                      '-DOPENXR_AAR='+str(aar)])
    run('compile', [cmake, '--build', output/'native', '--parallel', '2'])
    if not (HERE/'assets/Lancer.glb').exists():
        raise FileNotFoundError('Run prepare_assets.py with the existing numpy/Pillow interpreter first')
    unsigned = output/'unsigned.apk'
    run('resources', [tools/'aapt2.exe', 'link', '-I', android, '--manifest', HERE/'AndroidManifest.xml',
                      '-A', HERE/'assets', '-o', unsigned])
    with zipfile.ZipFile(unsigned, 'a', compression=zipfile.ZIP_STORED) as archive:
        for name, source in (('libquest_workbench.so', output/'native/libquest_workbench.so'),
                             ('libopenxr_loader.so', aar/'jni/arm64-v8a/libopenxr_loader.so')):
            archive.write(source, 'lib/arm64-v8a/'+name)
    aligned = output/'aligned.apk'
    run('align', [tools/'zipalign.exe', '-P', '16', '-f', '4', unsigned, aligned])
    key = dependencies/'workbench-development.jks'
    if not key.exists():
        run('development-key', [Path('C:/Program Files (x86)/bin/keytool.exe'), '-genkeypair',
                               '-keystore', key, '-storepass', 'android', '-keypass', 'android',
                               '-alias', 'workbench', '-dname', 'CN=GearsVR Workbench Development',
                               '-validity', '10000', '-keyalg', 'RSA', '-keysize', '2048'])
    apk = output/'GearsVR-Quest-Workbench.apk'
    signer = tools/'lib/apksigner.jar'
    run('sign', ['java', '-jar', signer, 'sign', '--ks', key, '--ks-key-alias', 'workbench',
                 '--ks-pass', 'pass:android', '--key-pass', 'pass:android', '--out', apk, aligned])
    run('verify-signature', ['java', '-jar', signer, 'verify', '--verbose', apk])
    run('verify-alignment', [tools/'zipalign.exe', '-c', '-P', '16', '4', apk])
    run('verify-manifest', [tools/'aapt2.exe', 'dump', 'badging', apk])
    report = dict(apk=str(apk), sha256=hashlib.sha256(apk.read_bytes()).hexdigest(), bytes=apk.stat().st_size,
                  build=True, signature_verified=True, zip_alignment_16k=True,
                  physical_device_validation=False, commands=commands)
    (output/'build-receipt.json').write_text(json.dumps(report, indent=2), 'utf-8')
    print(json.dumps({key:value for key,value in report.items() if key!='commands'}))
    return apk


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dependencies', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build(args.dependencies.resolve(), args.output.resolve())
