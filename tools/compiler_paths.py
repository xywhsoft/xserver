"""Resolve TCC's libc directory from the compiler's actual Linux header layout."""
from __future__ import annotations
import os
from pathlib import PurePosixPath
import re
import subprocess

TRIPLET = re.compile(r"[A-Za-z0-9_+.-]+\Z")

def linux_tcc_triplet(compiler: str) -> str:
    multi = subprocess.run([compiler, "-print-multiarch"], capture_output=True, text=True)
    value = multi.stdout.strip()
    if multi.returncode == 0 and value and TRIPLET.fullmatch(value):
        return value
    target = subprocess.check_output([compiler, "-dumpmachine"], text=True).strip()
    if not target or not TRIPLET.fullmatch(target):
        raise ValueError("compiler returned an invalid target triplet")
    # Clang's target can contain a vendor (x86_64-pc-linux-gnu) that is absent
    # from Debian's libc directory. Ask its preprocessor instead of guessing
    # or silently changing the selected compiler/target.
    search = subprocess.run([compiler, "-E", "-x", "c", "-", "-v"], input="",
                            capture_output=True, text=True, env={**os.environ, "LC_ALL": "C"})
    active = False
    if search.returncode == 0:
        for line in search.stderr.splitlines():
            text = line.strip()
            if text == '#include <...> search starts here:':
                active = True
                continue
            if active and text == 'End of search list.':
                break
            path = PurePosixPath(text)
            if active and path.is_absolute() and path.parent.name == 'include' and '-linux-' in path.name and TRIPLET.fullmatch(path.name):
                return path.name
    return target
