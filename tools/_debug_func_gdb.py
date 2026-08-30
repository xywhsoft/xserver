"""Temporary local crash reproducer; removed after diagnosis."""

from pathlib import Path
import subprocess
import tempfile

import func_test


root = Path(__file__).resolve().parent.parent
func_test.EXE = (root / "release" / "xs_dbg").resolve()
tempfile.tempdir = str((root / "release").resolve())
original_popen = subprocess.Popen


def debugger_popen(arguments, *args, **kwargs):
    command = list(arguments)
    command[1] = str((root / "release" / command[1]).resolve())
    kwargs["cwd"] = "/home/ubuntu"
    return original_popen(command, *args, **kwargs)


func_test.subprocess.Popen = debugger_popen
func_test.behavior_matrix()
print(func_test.failures)
