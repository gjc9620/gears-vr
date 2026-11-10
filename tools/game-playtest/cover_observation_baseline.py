"""Exact file restoration for one idle, normally closed cover observation.

The caller proves runtime ownership and cleanup. This module never launches,
queries or terminates a process, and never restores public or user profile files.
"""
from pathlib import Path
import hashlib
import json
import os


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def regular(path):
    path = Path(path)
    if path.is_symlink() or path.is_junction() or (path.exists() and not path.is_file()):
        raise ValueError('Not an ordinary file: ' + str(path))
    # Reject a redirected ancestor as well as a redirected leaf.
    for parent in path.parents:
        if parent.is_symlink() or parent.is_junction():
            raise ValueError('Redirected file parent: ' + str(parent))
    return path


def save_json(path, value):
    with Path(path).open('x', encoding='utf-8') as stream:
        json.dump(value, stream, indent=2)
        stream.flush()
        os.fsync(stream.fileno())


def capture(specs, private_directories, store, root):
    root, store = Path(root).resolve(), Path(store).resolve()
    if not store.is_relative_to(root) or store.exists():
        raise ValueError('A new workspace backup directory is required')
    directories = [Path(p).resolve() for p in private_directories]
    if any(not p.is_relative_to(root) or not p.is_dir() for p in directories):
        raise ValueError('Private directories must already exist in the workspace')
    entries = {str(regular(p).resolve()): bool(read_only) for p, read_only in specs}
    trees = {}
    for directory in directories:
        regular(directory / '__boundary_check__')
        files = [regular(p) for p in directory.iterdir()]
        trees[str(directory)] = sorted(p.name for p in files)
        for path in files:
            entries[str(path.resolve())] = False
    if any(not read_only and not Path(p).is_relative_to(root) for p, read_only in entries.items()):
        raise ValueError('Writable baseline path is outside workspace')
    store.mkdir()
    rows = []
    for index, (name, read_only) in enumerate(entries.items()):
        path = regular(name)
        present = path.exists()
        row = dict(path=name, exists=present, read_only=read_only, sha256=None, saved=None)
        if present:
            raw = path.read_bytes()
            saved = store / (str(index) + '.raw')
            with saved.open('xb') as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())
            row.update(sha256=digest(saved), saved=str(saved))
            if digest(path) != row['sha256']:
                raise RuntimeError('File changed during baseline capture')
        rows.append(row)
    result = dict(root=str(root), store=str(store), files=rows, private_file_sets=trees)
    save_json(store / 'baseline.json', result)
    verify(result, current=True)
    return result


def verify(baseline, *, current):
    root, store = Path(baseline['root']).resolve(), Path(baseline['store']).resolve()
    if not store.is_relative_to(root):
        raise ValueError('Backup escaped workspace')
    seen = set()
    for row in baseline['files']:
        path = regular(row['path']).resolve()
        if str(path) in seen or type(row['exists']) is not bool or type(row['read_only']) is not bool:
            raise ValueError('Invalid or duplicate baseline row')
        seen.add(str(path))
        if not row['read_only'] and not path.is_relative_to(root):
            raise ValueError('Write escaped workspace')
        if row['exists']:
            saved = regular(row['saved']).resolve()
            if saved.parent != store or digest(saved) != row['sha256']:
                raise ValueError('Backup identity changed')
        elif row['sha256'] is not None or row['saved'] is not None:
            raise ValueError('Absent file has invented backup')
        if current or row['read_only']:
            if path.exists() != row['exists'] or (path.exists() and digest(path) != row['sha256']):
                raise RuntimeError('Protected baseline differs: ' + str(path))
    for name, files in baseline['private_file_sets'].items():
        directory = Path(name)
        if not directory.resolve().is_relative_to(root) or not directory.is_dir():
            raise ValueError('Private directory identity changed')
        regular(directory / '__boundary_check__')
        present = sorted(regular(p).name for p in directory.iterdir())
        if current and present != files:
            raise RuntimeError('Private file set changed')


def restore(baseline):
    # Validate every backup, all external read-only files, and all removable
    # leaves before the first mutation. Caller separately proves fresh idle.
    verify(baseline, current=False)
    extras = []
    root = Path(baseline['root']).resolve()
    listed = {Path(row['path']).resolve() for row in baseline['files']}
    for name, original in baseline['private_file_sets'].items():
        directory = Path(name).resolve()
        for path in directory.iterdir():
            path = regular(path)
            if path.name not in original:
                if path.resolve().parent != directory or not path.resolve().is_relative_to(root):
                    raise ValueError('New file escaped exact private directory')
                if path.resolve() not in listed:
                    extras.append(path)
    for row in baseline['files']:
        if row['read_only']:
            continue
        path = Path(row['path'])
        if row['exists']:
            if not path.exists() or digest(path) != row['sha256']:
                path.write_bytes(Path(row['saved']).read_bytes())
        elif path.exists():
            # Absent non-tree files are individually named by the entry plan.
            path.unlink()
    for path in extras:
        path.unlink()
    verify(baseline, current=True)
    return dict(restored_files=len(baseline['files']), removed_new_private_files=len(extras),
                all_bytes_and_absences_match=True, private_file_sets_match=True,
                public_and_user_files_written=False)
