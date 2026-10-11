import hashlib
import json
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).parent / "python-libs"))
import pefile
path = pathlib.Path(sys.argv[1])
pe = pefile.PE(str(path))
result = {
    "path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    "machine": hex(pe.FILE_HEADER.Machine), "image_base": hex(pe.OPTIONAL_HEADER.ImageBase),
    "imports": {e.dll.decode(): [i.name.decode() if i.name else f"ordinal:{i.ordinal}" for i in e.imports]
                for e in getattr(pe, "DIRECTORY_ENTRY_IMPORT", [])},
    "sections": [{"name": s.Name.rstrip(b"\0").decode(), "rva": hex(s.VirtualAddress),
                  "virtual_size": s.Misc_VirtualSize} for s in pe.sections],
}
pathlib.Path(sys.argv[2]).write_text(json.dumps(result, indent=2), encoding="utf-8")
print(json.dumps({k: v for k, v in result.items() if k != "imports" and k != "sections"}))
print("Graphics imports:", {k: v for k, v in result["imports"].items() if any(t in k.lower() for t in ["dxgi", "d3d", "vulkan", "openxr"] )})
