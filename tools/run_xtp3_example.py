"""Run the owned XTP3 example in a separate, temporary xs process."""
import argparse
from pathlib import Path
import tempfile
from test_extensions_runtime import ROOT, invoke


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    args = parser.parse_args()
    source = '#define XS_XTP3_EXAMPLE\n' + (ROOT / 'dev/xtp3/main.c').read_text(encoding='utf-8')
    with tempfile.TemporaryDirectory(prefix='xs-xtp3-example-') as directory:
        result = invoke(args.exe.resolve(), source, Path(directory), 'xtp3-example')
        print(result.stdout, end='')
        print(result.stderr, end='')
        if result.returncode or '[sdk] xtp3 codec+four-shapes+callback-ABI+RPC+PUSH+NOTIFY ok' not in result.stdout:
            return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
