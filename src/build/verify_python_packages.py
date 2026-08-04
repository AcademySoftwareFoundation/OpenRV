"""
Verify that all required Python packages are importable.
Used as a proxy-safe replacement for 'pip install -r requirements.txt'
when the corporate proxy blocks files.pythonhosted.org.
Packages are pre-installed from the bundled RV.app Python 3.11 site-packages.
"""
import importlib.util
import sys

REQUIRED = [
    ("numpy", "numpy"),
    ("opentimelineio", "opentimelineio"),
    ("OpenGL", "OpenGL"),
    ("psutil", "psutil"),
    ("certifi", "certifi"),
    ("six", "six"),
    ("packaging", "packaging"),
    ("requests", "requests"),
    ("cryptography", "cryptography"),
    ("pydantic", "pydantic"),
]

failed = []
for name, module in REQUIRED:
    if importlib.util.find_spec(module) is None:
        failed.append(name)
        print(f"MISSING: {name}", file=sys.stderr)
    else:
        print(f"OK: {name}")

if failed:
    print(f"\nERROR: Missing packages: {failed}", file=sys.stderr)
    sys.exit(1)

print("\nAll required Python packages verified successfully.")
sys.exit(0)
