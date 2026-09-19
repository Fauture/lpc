import urllib.request, json, sys

URL = 'http://127.0.0.1:13337/mcp'
SID = [None]

def call(payload):
    data = json.dumps(payload).encode()
    req = urllib.request.Request(URL, data=data, method='POST')
    req.add_header('Content-Type', 'application/json')
    req.add_header('Accept', 'application/json, text/event-stream')
    if SID[0]:
        req.add_header('Mcp-Session-Id', SID[0])
    r = urllib.request.urlopen(req, timeout=120)
    sid = r.headers.get('Mcp-Session-Id') or r.headers.get('mcp-session-id')
    if sid:
        SID[0] = sid
    return r.read().decode('utf-8', 'replace')

def parse(body):
    out = []
    for chunk in body.split('\n'):
        chunk = chunk.strip()
        if chunk.startswith('data:'):
            chunk = chunk[5:].strip()
        if chunk.startswith('{'):
            try:
                out.append(json.loads(chunk))
            except Exception:
                pass
    return out

call({'jsonrpc': '2.0', 'id': 1, 'method': 'initialize',
      'params': {'protocolVersion': '2024-11-05', 'capabilities': {},
                 'clientInfo': {'name': 'dsh', 'version': '1'}}})
call({'jsonrpc': '2.0', 'method': 'notifications/initialized'})

def tool(name, args, rid=10):
    body = call({'jsonrpc': '2.0', 'id': rid, 'method': 'tools/call',
                 'params': {'name': name, 'arguments': args}})
    msgs = parse(body)
    if not msgs:
        return body
    m = msgs[-1]
    if 'result' in m:
        cont = m['result'].get('content') or []
        txt = '\n'.join(c.get('text', '') for c in cont)
        return txt
    return json.dumps(m)[:2000]

def coerce(v):
    """key=value without shell quoting pain: ints, lists, 's:' forced strings."""
    if v.startswith('s:'):
        return v[2:]
    if v and v[0] in '[{':
        return json.loads(v)
    if v.lower() in ('true', 'false'):
        return v.lower() == 'true'
    if ':' in v:                      # addr:size shorthand for get_bytes regions
        a, s = v.split(':', 1)
        return [{'addr': a, 'size': int(s, 0)}]
    if v.startswith('0x'):            # IDA tools take addresses as strings
        return v
    try:
        return int(v, 0)
    except ValueError:
        pass
    if ',' in v:
        out = []
        for part in v.split(','):
            try:
                out.append(int(part, 0))
            except ValueError:
                out.append(part)
        return out
    return v

def kw_args(rest):
    args = {}
    for kv in rest:
        if '=' in kv:
            k, v = kv.split('=', 1)
            args[k] = coerce(v)
    return args

if __name__ == '__main__':
    what = sys.argv[1] if len(sys.argv) > 1 else 'schema'
    if what == 'schema':
        body = call({'jsonrpc': '2.0', 'id': 2, 'method': 'tools/list'})
        for m in parse(body):
            if 'result' in m and 'tools' in m['result']:
                for t in m['result']['tools']:
                    if t['name'] in ('lookup_funcs', 'decompile', 'disasm'):
                        print(t['name'], json.dumps(t.get('inputSchema'))[:900])
    elif what == '-tools':
        body = call({'jsonrpc': '2.0', 'id': 2, 'method': 'tools/list'})
        for m in parse(body):
            if 'result' in m and 'tools' in m['result']:
                for t in m['result']['tools']:
                    print('%-28s %s' % (t['name'], (t.get('description') or '').split('\n')[0][:78]))
    elif what == 'lookup':
        print(tool('lookup_funcs', {'queries': [sys.argv[2]]}))
    elif what == 'decompile':
        print(tool('decompile', {'addresses': sys.argv[2:]}))
    elif what == 'dis':
        print(tool('disasm', {'addr': sys.argv[2], 'max_instructions': 60}))
    elif what == 'call':
        # _ida.py call <tool> key=value ...   OR   _ida.py call <tool> @file.json
        if len(sys.argv) > 3 and sys.argv[3].startswith('@'):
            args = json.load(open(sys.argv[3][1:], encoding='utf-8'))
        else:
            args = kw_args(sys.argv[3:])
        print(tool(sys.argv[2], args))
    elif what == 'bytes':
        print(tool('get_bytes', kw_args(sys.argv[2:])))
    elif what == 'str':
        print(tool('get_string', kw_args(sys.argv[2:])))
    else:
        print(tool(what, kw_args(sys.argv[2:])))
