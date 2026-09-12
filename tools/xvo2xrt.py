#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""xvo* -> xrtValue* 调用形态机械翻译器（xadmin 平移原型验证用）

处理形态：
  P0 纯改名：xvoUnref/xvoAddRef/xvoType/...
  P1 键视图改写：丢键长参数，字面量键 -> XRT_STR_LITERAL，运行时键 -> xrtStrViewN
  P2 类型 getter 语句块改写：`T x = xvoTableGetText(o,k,n);` -> 两步调用块
  P3 setter 内嵌 getter：SetText(o,k,n, GetText(o2,k2,n2), ...) -> Set(o,K, ObjectGet(o2,K2))
不认识的形态标 /* XVO-MANUAL */ 留人工。
"""
import re
import sys

# ---------- 基础工具 ----------

def split_args(s):
    """按顶层逗号切分参数串（括号/引号感知）"""
    args, depth, cur, i, n = [], 0, '', 0, len(s)
    in_str = in_chr = False
    while i < n:
        c = s[i]
        if in_str:
            cur += c
            if c == '\\' and i + 1 < n:
                cur += s[i + 1]; i += 2; continue
            if c == '"': in_str = False
        elif in_chr:
            cur += c
            if c == '\\':
                cur += s[i + 1]; i += 2; continue
            if c == "'": in_chr = False
        elif c == '"': in_str = True; cur += c
        elif c == "'": in_chr = True; cur += c
        elif c in '([{': depth += 1; cur += c
        elif c in ')]}': depth -= 1; cur += c
        elif c == ',' and depth == 0:
            args.append(cur); cur = ''
        else:
            cur += c
        i += 1
    if cur.strip() != '' or args:
        args.append(cur)
    return [a.strip() for a in args]

def match_paren(s, start):
    """s[start] == '('，返回匹配右括号位置"""
    depth, i, n = 0, start, len(s)
    in_str = in_chr = False
    while i < n:
        c = s[i]
        if in_str:
            if c == '\\': i += 2; continue
            if c == '"': in_str = False
        elif in_chr:
            if c == '\\': i += 2; continue
            if c == "'": in_chr = False
        elif c == '"': in_str = True
        elif c == "'": in_chr = True
        elif c == '(': depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0: return i
        i += 1
    return -1

LIT_RE = re.compile(r'^"(?:[^"\\]|\\.)*"$')

def key_view(k):
    """键参数 -> 视图表达式"""
    k = k.strip()
    if LIT_RE.match(k):
        return 'XRT_STR_LITERAL(%s)' % k
    return k  # 运行时键由调用方包 xrtStrViewN（见规则表）

# ---------- 翻译规则 ----------

stats = {'ok': 0, 'manual': 0}
manual_log = []

def keyview_n(k, n):
    """运行时键：xrtStrViewN(k, n)"""
    k = k.strip()
    if LIT_RE.match(k):
        return 'XRT_STR_LITERAL(%s)' % k
    if n.strip() in ('0',):
        return 'xrtStrView(%s)' % k
    return 'xrtStrViewN(%s, (size_t)(%s))' % (k, n)

def translate_call(name, args):
    """单次 xvo 调用 -> 新调用文本；返回 None 表示需人工"""
    na = len(args)
    A = lambda i: args[i] if i < na else ''

    # —— 原生 xrt 漂移形态 ——
    sp = translate_special(name, args)
    if sp is not None:
        return sp
    if name == 'xrtDictCreate':
        return 'xrtMapCreate(%s)' % A(0)
    if name == 'xrtDictGet':
        return 'xrtMapGet(%s, %s)' % (A(0), bview(A(1), A(2)))
    if name == 'xrtDictSet':
        return 'xrtMapGetOrAdd(%s, %s, %s)' % (A(0), bview(A(1), A(2)), A(3) if A(3) else 'NULL')
    if name == 'xrtDictRemove':
        return 'xrtMapRemove(%s, %s)' % (A(0), bview(A(1), A(2)))
    if name == 'xrtPathJoin':
        # 老变参计数形态 -> 二元右折叠
        if na >= 1 and A(0).strip().isdigit():
            segs = [a for a in args[1:] if a.strip() != '']
            if len(segs) == 2:
                return 'xrtPathJoin(%s, %s)' % (segs[0], segs[1])
            if len(segs) > 2:
                inner = 'xrtPathJoin(%s, %s)' % (segs[-2], segs[-1])
                for s in reversed(segs[:-2]):
                    inner = 'xrtPathJoin(%s, %s)' % (s, inner)
                return inner
        return None

    if name == 'xvoUnref':        return 'xrtValueRelease(%s)' % A(0)
    if name == 'xvoAddRef':       return 'xrtValueRetain(%s)' % A(0)
    if name == 'xvoDeepCopy':     return 'xrtValueClone(%s)' % A(0)
    if name == 'xvoCopy':         return 'xrtValueRetain(%s)' % A(0)
    if name == 'xvoType':         return 'xrtValueType(%s)' % A(0)
    if name == 'xvoCreateTable':  return 'xrtValueObject()'
    if name == 'xvoCreateArray':  return 'xrtValueArray()'
    if name == 'xvoCreateList':   return 'xrtValueArray()'
    if name == 'xvoCreateText':   return 'xrtValueString(%s)' % ('xrtStrView(%s)' % A(0) if A(1).strip() in ('', '0') else 'xrtStrViewN(%s, (size_t)(%s))' % (A(0), A(1)))
    if name == 'xvoCreateInt':    return 'xrtValueInt(%s)' % A(0)
    if name == 'xvoCreateBool':   return 'xrtValueBool(%s)' % A(0)
    if name == 'xvoCreateFloat':  return 'xrtValueFloat(%s)' % A(0)
    if name == 'xvoCreateNull':   return 'xrtValueNull()'

    if name in ('xvoTableItemCount', 'xvoArrayItemCount', 'xvoListItemCount'):
        return 'xrtValueCount(%s)' % A(0)

    if name == 'xvoTableExists':
        return 'xrtValueObjectHas(%s, %s)' % (A(0), keyview_n(A(1), A(2)))

    if name == 'xvoTableSetValue':
        kv = keyview_n(A(1), A(2))
        if A(4).strip() in ('TRUE', '1', 'true'):
            return 'xrtValueObjectSetNew(%s, %s, %s)' % (A(0), kv, A(3))
        return 'xrtValueObjectSet(%s, %s, %s)' % (A(0), kv, A(3))

    if name == 'xvoTableGetValue':
        return 'xrtValueObjectGet(%s, %s)' % (A(0), keyview_n(A(1), A(2)))

    if name == 'xvoArrayGetValue' or name == 'xvoListGetValue':
        return 'xrtValueArrayGet(%s, (size_t)(%s))' % (A(0), A(1))

    if name == 'xvoArrayAppendValue':
        if A(2).strip() in ('TRUE', '1', 'true'):
            return 'xrtValueArrayAppendNew(%s, %s)' % (A(0), A(1))
        return '(xrtValueArrayAppend(%s, %s) ? 0 : 0)' % (A(0), A(1)) if False else 'xrtValueArrayAppend(%s, %s)' % (A(0), A(1))

    if name in ('xvoArrayAppendText',):
        tv = 'xrtStrView(%s)' % A(1) if A(2).strip() in ('', '0') else 'xrtStrViewN(%s, (size_t)(%s))' % (A(1), A(2))
        return 'xrtValueArrayAppendNew(%s, xrtValueString(%s))' % (A(0), tv)

    if name == 'xvoArrayAppendInt':
        return 'xrtValueArrayAppendNew(%s, xrtValueInt(%s))' % (A(0), A(1))

    return None  # typed getter / setter / 其它 -> 上下文处理

TYPE_GETTERS = {
    'xvoTableGetInt':   ('xrtValueObjectGet',  'xrtValueGetInt',    'int64'),
    'xvoTableGetBool':  ('xrtValueObjectGet',  'xrtValueGetBool',   'bool'),
    'xvoTableGetFloat': ('xrtValueObjectGet',  'xrtValueGetFloat',  'double'),
    'xvoArrayGetInt':   (None,                 'xrtValueGetInt',    'int64'),
    'xvoGetInt':        (None,                 'xrtValueGetInt',    'int64'),
    'xvoGetBool':       (None,                 'xrtValueGetBool',   'bool'),
    'xvoGetFloat':      (None,                 'xrtValueGetFloat',  'double'),
}

SETTERS = {
    'xvoTableSetText':  'text',
    'xvoTableSetInt':   'int',
    'xvoTableSetBool':  'bool',
    'xvoTableSetFloat': 'float',
}

def getter_args(name, args):
    if name.startswith('xvoTable'):
        return keyview_n(args[1], args[2]), args[0]
    return None, args[0]  # array/generic

# ---------- 语句级改写 ----------

ASSIGN_RE = re.compile(
    r'^(?P<indent>\s*)(?P<decl>(?:static\s+|const\s+|unsigned\s+)*[A-Za-z_]\w*\s*\**\s+)?'
    r'(?P<var>[A-Za-z_]\w*)\s*=\s*(?P<call>xvo(?:Table|Array|List)?Get(?:Text|Int|Bool|Float)\s*\()',
)

RETURN_RE = re.compile(
    r'^(?P<indent>\s*)return\s+(?P<call>xvo(?:Table|Array|List)?Get(?:Text|Int|Bool|Float)\s*\()',
)

def translate_line_stmt(line):
    """处理 `T x = xvo...Get...(...);` / `x = ...;` / `return ...;` 语句"""
    m = ASSIGN_RE.match(line)
    if not m or not line.rstrip().endswith(';'):
        m = None
    if m is None:
        m2 = RETURN_RE.match(line)
        if m2 and line.rstrip().endswith(';'):
            start = line.index(m2.group('call'))
            p = match_paren(line, start + len(m2.group('call')) - 1)
            if p >= 0 and line[p + 1:].strip() == ';':
                name = m2.group('call').strip()[:-1].strip()
                args = split_args(line[start + len(m2.group('call')):p])
                ind = m2.group('indent')
                if name == 'xvoTableGetText':
                    return (
                        '%s{ xstrview rv = {0}; (void)xrtValueGetString(xrtValueObjectGet(%s, %s), &rv);'
                        ' return (str)rv.Data; }'
                        % (ind, args[0], keyview_n(args[1], args[2]))
                    )
                if name in TYPE_GETTERS:
                    _, getfn, ty = TYPE_GETTERS[name]
                    if name.startswith('xvoTable'):
                        src = 'xrtValueObjectGet(%s, %s)' % (args[0], keyview_n(args[1], args[2]))
                    elif name.startswith('xvoArray'):
                        src = 'xrtValueArrayGet(%s, (size_t)(%s))' % (args[0], args[1])
                    else:
                        src = args[0]
                    return (
                        '%s{ %s rv = 0; (void)%s(%s, &rv); return rv; }'
                        % (ind, ty, getfn, src)
                    )
        return None
    start = line.index(m.group('call'))
    p = match_paren(line, start + len(m.group('call')) - 1)
    if p < 0 or line[p + 1:].strip() != ';':
        return None
    name = m.group('call').strip()[:-1].strip()
    args = split_args(line[start + len(m.group('call')):p])
    var = m.group('var')
    ind = m.group('indent')
    if name in ('xvoTableGetText',):
        kv = keyview_n(args[1], args[2])
        if m.group('decl'):
            decl = m.group('decl').strip()
            cast = decl.split()[-1]
            return (
                '%s%s %s = 0;\n'
                '%sxstrview %s_v = {0};\n'
                '%s(void)xrtValueGetString(xrtValueObjectGet(%s, %s), &%s_v);\n'
                '%s%s = (%s)%s_v.Data;'
                % (ind, decl, var, ind, var, ind, args[0], kv, var, ind, var, cast, var)
            )
        return (
            '%sxstrview %s_v = {0};\n'
            '%s(void)xrtValueGetString(xrtValueObjectGet(%s, %s), &%s_v);\n'
            '%s%s = %s_v.Data;'
            % (ind, var, ind, args[0], kv, var, ind, var, var)
        )
    if name in TYPE_GETTERS:
        _, getfn, ty = TYPE_GETTERS[name]
        if name.startswith('xvoTable'):
            src = 'xrtValueObjectGet(%s, %s)' % (args[0], keyview_n(args[1], args[2]))
        elif name.startswith('xvoArray'):
            src = 'xrtValueArrayGet(%s, (size_t)(%s))' % (args[0], args[1])
        else:
            src = args[0]
        decl = m.group('decl').strip() if m.group('decl') else ''
        if not decl:
            return (
                '%s%s = 0;\n'
                '%s(void)%s(%s, &%s);'
                % (ind, var, ind, getfn, src, var)
            )
        return (
            '%s%s %s = 0;\n'
            '%s(void)%s(%s, &%s);'
            % (ind, decl, var, ind, getfn, src, var)
        )
    if name in ('xvoArrayGetValue', 'xvoListGetValue'):
        pass
    if name == 'xvoTableGetValue':
        return None
    return None

# ---------- 主流程 ----------

CALL_RE = re.compile(r'\b(?:xvo[A-Z]\w*|xrtDict[A-Z]\w*|xrtPathJoin|xrtJsonParse|xrtPathParent)\s*\(')

def translate(src, path):
    out = []
    for lineno, line in enumerate(src.split('\n'), 1):
        if ('xvo' not in line and 'xrtDict' not in line and 'xrtPathJoin' not in line
                and 'XVO_' not in line and 'xvalue' not in line
                and 'xrtJsonParse' not in line and 'xrtPathParent' not in line
                and 'xmutex' not in line and 'xcond' not in line
                and 'xrtFileWriteAll' not in line and 'xrtFileReadAll' not in line):
            out.append(line); continue
        # P3: setter 内嵌 TableGetText
        line = rewrite_setter_nested(line, path, lineno)
        # 语句级 typed getter
        if 'xvoTableGet' in line or 'xvoArrayGet' in line or re.search(r'\bxvoGet(Int|Bool|Float|Text)\s*\(', line):
            r = translate_line_stmt(line)
            if r is not None:
                out.append(r); continue
            if CALL_RE.search(line):
                line2, unresolved = rewrite_calls_expr(line)
                if unresolved:
                    stats['manual'] += 1
                    manual_log.append('%s:%d: %s' % (path, lineno, line.strip()))
                line = line2
            out.append(line); continue
        # 普通调用
        line2, unresolved = rewrite_calls_expr(line)
        if unresolved:
            stats['manual'] += 1
            manual_log.append('%s:%d: %s' % (path, lineno, line.strip()))
        out.append(line2)
    return '\n'.join(out)

def rewrite_calls_expr(line):
    """行内所有可改写调用；无法处理的标 XVO-MANUAL"""
    unresolved = False
    pos = 0
    while True:
        m = CALL_RE.search(line, pos)
        if not m:
            break
        start = m.start()
        p = match_paren(line, m.end() - 1)
        if p < 0:
            unresolved = True
            break
        name = m.group(0)[:-1].strip()
        inner = line[m.end():p]
        args = split_args(inner)
        rep = translate_call(name, args)
        if name in SETTERS:
            rep = translate_setter(name, args)
        if name in ('xvoTableGetText',):
            rep = None  # 表达式位置未处理（语句级已拦）
        if rep is None:
            unresolved = True
            pos = p + 1
            continue
        rep, _u = rewrite_calls_expr(rep)
        line = line[:start] + rep + line[p + 1:]
        pos = start + len(rep)
        stats['ok'] += 1
    if unresolved:
        line = '/* XVO-MANUAL */ ' + line
    return line, unresolved

def translate_setter(name, args):
    kind = SETTERS[name]
    kv = keyview_n(args[1], args[2])
    obj = args[0]
    if kind == 'text':
        text, tsize, take = args[3], args[4], args[5]
        tv = 'xrtStrView(%s)' % text if tsize.strip() in ('', '0') else 'xrtStrViewN(%s, (size_t)(%s))' % (text, tsize)
        return 'xrtValueObjectSetNew(%s, %s, xrtValueString(%s))' % (obj, kv, tv)
    if kind == 'int':
        return 'xrtValueObjectSetNew(%s, %s, xrtValueInt(%s))' % (obj, kv, args[3])
    if kind == 'float':
        return 'xrtValueObjectSetNew(%s, %s, xrtValueFloat(%s))' % (obj, kv, args[3])
    if kind == 'bool':
        # 单例 bool：Set 借引用，随后释放模板引用
        return ('(xrtValueObjectSet(%s, %s, xrtValueBool(%s)), xrtValueRelease(xrtValueBool(%s)))'
                % (obj, kv, args[3], args[3]))
    return None

def rewrite_setter_nested(line, path, lineno):
    """xvoTableSetText(o,k,n, xvoTableGetText(o2,k2,n2), ts, take) -> Set(o,K, Get(o2,K2))"""
    while True:
        m = re.search(r'xvoTableSetText\s*\(', line)
        if not m:
            return line
        p = match_paren(line, m.end() - 1)
        if p < 0:
            return line
        args = split_args(line[m.end():p])
        if len(args) == 6 and re.match(r'^xvoTableGetText\s*\(', args[3].strip()):
            g = args[3].strip()
            gp = match_paren(g, g.index('('))
            ga = split_args(g[g.index('(') + 1:gp])
            rep = 'xrtValueObjectSet(%s, %s, xrtValueObjectGet(%s, %s))' % (
                args[0], keyview_n(args[1], args[2]), ga[0], keyview_n(ga[1], ga[2]))
            line = line[:m.start()] + rep + line[p + 1:]
            stats['ok'] += 1
        else:
            return line

ENUM_MAP = {
    'XVO_DT_NULL': 'XVALUE_NULL', 'XVO_DT_BOOL': 'XVALUE_BOOL', 'XVO_DT_INT': 'XVALUE_INT',
    'XVO_DT_FLOAT': 'XVALUE_FLOAT', 'XVO_DT_TEXT': 'XVALUE_STRING', 'XVO_DT_TIME': 'XVALUE_TIME',
    'XVO_DT_POINT': 'XVALUE_POINTER', 'XVO_DT_ARRAY': 'XVALUE_ARRAY', 'XVO_DT_LIST': 'XVALUE_ARRAY',
    'XVO_DT_COLL': 'XVALUE_SET', 'XVO_DT_TABLE': 'XVALUE_OBJECT',
}

# 同名异形或改名的原生 xrt 漂移
XRT_RENAMES = {
    'xrtSHA256': 'xrtSha256',
    'xrtParseJSON': 'xrtJsonParse',
    'xrtStringifyJSON': 'xrtJsonStringify',
    'xrtPathGetDir': 'xrtPathParent',
    'xrtDictDestroy': 'xrtMapDestroy',
    'xrtDictCount': 'xrtMapCount',
    'xrtDictClear': 'xrtMapClear',
    'xrtDictWalk': 'xrtMapVisit',
    'xrtI64ToStr': 'xrtIntString',
    'xrtPathGetExt': 'xrtPathExt',
    'xrtPathGetName': 'xrtPathName',
    'xrtFileGetAll': 'xrtFileReadAll',
    'xrtDirDelete': 'xrtDirRemove',
    'xrtCondWaitTimeout': 'xrtCondWaitFor',
    'xrtMakeXIDS': 'xrtXidMakeString',
    'xrtCopyStr': 'xrtStrDupN',
}

TYPE_RENAMES = {
    'xdict': 'xmap*',
    'xmutex': 'xmutex*',
    'xcond': 'xcond*',
}

def bview(k, n):
    return '(xbytesview){ (cbytes)(%s), (size_t)(%s) }' % (k, n)

# 特殊签名形态的调用级规则
def translate_special(name, args):
    na = len(args)
    A = lambda i: args[i] if i < na else ''
    if name == 'xrtJsonParse':
        # 老两参形态 (ptr, len) -> 视图包装
        if na == 2:
            return 'xrtJsonParse(xrtStrViewN(%s, (size_t)(%s)))' % (A(0), A(1))
        return None
    if name == 'xrtPathParent':
        if na == 2 and A(1).strip().isdigit():
            return 'xrtPathParent(%s)' % A(0)
        return None
    if name == 'xvoCreateTime':
        return 'xrtValueTime(%s)' % A(0)
    if name == 'xrtFileWriteAll':
        if na == 3:
            return 'xrtFileWriteAll(%s, (xbytesview){ (cbytes)(%s), (size_t)(%s) })' % (A(0), A(1), A(2))
        return None
    if name == 'xrtStrDupN':
        if na == 2 and A(1).strip().isdigit() and A(1).strip() != '0':
            return 'xrtStrDupN(%s, (size_t)(%s))' % (A(0), A(1))
        if na == 2 and A(1).strip() == '0':
            return 'xrtStrDup(%s)' % A(0)
        return None
    return None

def translate_enums(src):
    for k, v in ENUM_MAP.items():
        src = re.sub(r'\b%s\b' % k, v, src)
    for k, v in XRT_RENAMES.items():
        src = re.sub(r'\b%s\b(?=\s*\()' % k, v, src)
    for k, v in TYPE_RENAMES.items():
        src = re.sub(r'\b%s\b' % k, v, src)
    # 老句柄按值传递 -> 新结构体指针
    src = re.sub(r'\bxvalue\b(?!\s*\*)', 'xvalue* ', src)
    # v->Type 成员访问（保守：仅已知变量形态）
    src = re.sub(r'\b([A-Za-z_]\w*(?:\([^()]*\))?)\s*->\s*Type\b', r'xrtValueType(\1)', src)
    return src

def main():
    paths = sys.argv[1:]
    for p in paths:
        with open(p, 'r', encoding='utf-8', errors='replace') as f:
            src = f.read()
        src = translate_enums(src)
        src = translate(src, p)
        sys.stdout.write(src)
    sys.stderr.write('\n[stats] ok=%d manual=%d\n' % (stats['ok'], stats['manual']))
    for l in manual_log[:40]:
        sys.stderr.write('  %s\n' % l)

if __name__ == '__main__':
    main()
