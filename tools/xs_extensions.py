"""Single registry for optional native objects, VFS headers and TCC imports."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REGISTRY = Path(__file__).with_name("extensions.json")
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")
LIBRARY_NAME = re.compile(r"[a-z][a-z0-9_-]*\Z")


def source_path(relative: str, root: Path = ROOT) -> Path:
    """Manifest paths are project-relative; never traverse outside the project."""
    if not isinstance(relative, str) or not relative:
        raise ValueError("extension source paths must be nonempty strings")
    path = (root / relative).resolve()
    if not path.is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f"missing or out-of-project extension source: {relative}")
    return path


def load_registry(path: Path = REGISTRY) -> dict:
    registry = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(registry, dict):
        raise ValueError("extensions.json must contain an object")
    macros: set[str] = set()
    for name, entry in registry.items():
        if not LIBRARY_NAME.fullmatch(name) or not isinstance(entry, dict):
            raise ValueError(f"invalid extension entry: {name}")
        for field in ("macro", "symbol_macro"):
            if field not in entry:
                continue
            value = entry.get(field)
            if not isinstance(value, str) or not IDENTIFIER.fullmatch(value):
                raise ValueError(f"{name}: invalid {field}")
        if "feature_macro" in entry:
            value = entry["feature_macro"]
            values = [value] if isinstance(value, str) else value
            if not values or not all(isinstance(v, str) and IDENTIFIER.fullmatch(v) for v in values):
                raise ValueError(f"{name}: invalid feature_macro")
        unknown = set(entry) - {"macro", "headers", "sources", "symbols",
                               "symbol_macro", "link_flags", "requires",
                               "host_includes", "feature_macro"}
        if unknown:
            raise ValueError(f"{name}: unknown field(s): {', '.join(sorted(unknown))}")
        if entry["macro"] in macros:
            raise ValueError(f"{name}: duplicate feature macro {entry['macro']}")
        macros.add(entry["macro"])
        requires = entry.get("requires", [])
        if not isinstance(requires, list) or not all(isinstance(v, str) for v in requires):
            raise ValueError(f"{name}: requires must be a string list")
        for dependency in requires:
            if dependency == name:
                raise ValueError(f"{name}: requires cannot reference itself")
            if dependency not in registry:
                raise ValueError(f"{name}: requires unknown extension {dependency}")
        host_includes = entry.get("host_includes", [])
        if not isinstance(host_includes, list) or not all(isinstance(v, str) and v for v in host_includes):
            raise ValueError(f"{name}: host_includes must be nonempty strings")
        if not isinstance(entry.get("headers"), dict) or not entry["headers"]:
            raise ValueError(f"{name}: headers must be a nonempty object")
        for virtual in entry["headers"]:
            parts = virtual.replace("\\", "/").split("/")
            if virtual.startswith("/") or any(p in ("", ".", "..") for p in parts):
                raise ValueError(f"{name}: invalid VFS header name {virtual}")
        if not isinstance(entry.get("sources"), list) or not entry["sources"]:
            raise ValueError(f"{name}: sources must be a nonempty list")
        for source in entry["sources"]:
            if not isinstance(source, dict) or not isinstance(source.get("path"), str):
                raise ValueError(f"{name}: each source needs a path")
            if set(source) - {"path", "flags", "defines"}:
                raise ValueError(f"{name}: source supports path, flags and defines only")
            for field in ("flags", "defines"):
                values = source.get(field, [])
                if not isinstance(values, list) or not all(isinstance(v, str) for v in values):
                    raise ValueError(f"{name}: source {field} must be a string list")
        if not isinstance(entry.get("symbols"), str):
            raise ValueError(f"{name}: symbols needs an import file path")
        links = entry.get("link_flags", {})
        if not isinstance(links, dict) or set(links) - {"windows", "linux"}:
            raise ValueError(f"{name}: link_flags supports windows/linux")
        for flags in links.values():
            if not isinstance(flags, list) or not all(isinstance(v, str) for v in flags):
                raise ValueError(f"{name}: link flags must be string lists")
    return registry


def select_extensions(names: list[str], registry: dict | None = None,
                      root: Path = ROOT) -> dict:
    registry = load_registry() if registry is None else registry
    requested = {name.lower() for name in names}
    # "all" 是保留名：展开为清单中的全部可选库（可与其它名字混用，结果一致）。
    if "all" in requested:
        requested |= set(registry.keys())
        requested.discard("all")
    unknown = requested - registry.keys()
    if unknown:
        raise ValueError(f"unknown extension(s): {', '.join(sorted(unknown))}; "
                         f"available: all, {', '.join(registry)}")
    # 依赖闭包：显式选择或被已选条目 requires 的条目都入选（DFS，含环检测）。
    expanded: set[str] = set()

    def requires_of(name: str) -> list[str]:
        requires = registry[name].get("requires", [])
        if not isinstance(requires, list) or not all(isinstance(v, str) for v in requires):
            raise ValueError(f"{name}: requires must be a string list")
        for dependency in requires:
            if dependency == name:
                raise ValueError(f"{name}: requires cannot reference itself")
            if dependency not in registry:
                raise ValueError(f"{name}: requires unknown extension {dependency}")
        return requires

    def visit(name: str, path: list[str]) -> None:
        if name in expanded:
            return
        if name in path:
            raise ValueError("extension requires cycle detected: "
                             + " -> ".join(path + [name]))
        for dependency in requires_of(name):
            visit(dependency, path + [name])
        expanded.add(name)

    for name in requested:
        visit(name, [])
    # Canonical registry order makes "xtp sqlite" and "sqlite xtp" identical.
    selected = {name: entry for name, entry in registry.items() if name in expanded}
    seen: dict[str, str] = {}
    for name, entry in selected.items():
        for virtual, source in entry["headers"].items():
            source_path(source, root)
            key = virtual.replace("\\", "/").lower()
            if key in seen:
                raise ValueError(f"VFS header collision: {virtual} ({seen[key]}, {name})")
            seen[key] = name
        for source in entry["sources"]:
            source_path(source["path"], root)
        source_path(entry["symbols"], root)
    return selected


def host_header(selected: dict) -> str:
    """Included after XS_TccAddSymbols; no per-library branches in the host."""
    lines = [
        "/* Generated from tools/extensions.json; do not edit. */",
        "#ifndef XS_BUILD_EXTENSIONS_H",
        "#define XS_BUILD_EXTENSIONS_H",
        "",
    ]
    for entry in selected.values():
        lines += [f'#include "{source}"' for source in entry["headers"].values()]
    lines += ["", "static const XS_TccSymbol g_XS_ExtensionSymbols[] = {"]
    for entry in selected.values():
        macro = entry["symbol_macro"]
        lines += [
            f"#define {macro}(name) {{ #name, (const void*)(name) }},",
            f'#include "{entry["symbols"]}"',
            f"#undef {macro}",
        ]
    lines += [
        "\t{ NULL, NULL }",
        "};",
        "",
        "static void XS_TccAddExtensions(TCCState* pTcc)",
        "{",
    ]
    for entry in selected.values():
        lines.append(f'\ttcc_define_symbol(pTcc, "{entry["macro"]}", "1");')
        features = entry.get("feature_macro", [])
        features = [features] if isinstance(features, str) else features
        for feature in features:
            lines.append(f'\ttcc_define_symbol(pTcc, "{feature}", "1");')
    lines += [
        "\tXS_TccAddSymbols(pTcc, g_XS_ExtensionSymbols,",
        "\t\tsizeof(g_XS_ExtensionSymbols) / sizeof(g_XS_ExtensionSymbols[0]) - 1);",
        "}",
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)
