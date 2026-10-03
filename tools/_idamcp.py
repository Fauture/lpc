# -*- coding: utf-8 -*-
"""Minimal MCP (streamable HTTP / SSE) client for the local IDA MCP server.

usage:
  python -X utf8 _idamcp.py list                 # tools/list
  python -X utf8 _idamcp.py call <tool> '<json>' # tools/call
  python -X utf8 _idamcp.py raw <method> '<json>'
"""
import io
import json
import sys
import urllib.error
import urllib.request

URL = 'http://127.0.0.1:13337/mcp'
SESSION = {'id': None}


def rpc(method, params=None, notify=False):
    body = {'jsonrpc': '2.0', 'method': method}
    if params is not None:
        body['params'] = params
    if not notify:
        body['id'] = 1
    data = json.dumps(body).encode('utf-8')
    req = urllib.request.Request(URL, data=data, method='POST')
    req.add_header('Content-Type', 'application/json')
    req.add_header('Accept', 'application/json, text/event-stream')
    if SESSION['id']:
        req.add_header('Mcp-Session-Id', SESSION['id'])
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            sid = resp.headers.get('Mcp-Session-Id') or resp.headers.get('mcp-session-id')
            if sid:
                SESSION['id'] = sid
            raw = resp.read().decode('utf-8', 'replace')
    except urllib.error.HTTPError as e:
        return {'http_error': e.code, 'body': e.read().decode('utf-8', 'replace')[:500]}
    except Exception as e:
        return {'error': str(e)}
    # SSE framed?  -> take the last data: line
    if raw.lstrip().startswith('event:') or 'data:' in raw[:200]:
        out = None
        for line in raw.splitlines():
            line = line.strip()
            if line.startswith('data:'):
                try:
                    out = json.loads(line[5:].strip())
                except Exception:
                    pass
        return out if out is not None else {'sse_raw': raw[:600]}
    try:
        return json.loads(raw)
    except Exception:
        return {'raw': raw[:600]}


def ensure_init():
    r = rpc('initialize', {'protocolVersion': '2024-11-05',
                           'capabilities': {},
                           'clientInfo': {'name': 'lpc-agent', 'version': '1.0'}})
    print('initialize ->', json.dumps(r, ensure_ascii=False)[:400], flush=True)
    rpc('notifications/initialized', {}, notify=True)


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'list'
    ensure_init()
    if cmd == 'list':
        r = rpc('tools/list', {})
        tools = (r or {}).get('result', {}).get('tools', [])
        print('tools: %d' % len(tools), flush=True)
        for t in tools:
            print('  %-32s %s' % (t.get('name'), (t.get('description') or '')[:90]), flush=True)
    elif cmd == 'schema':
        r = rpc('tools/list', {})
        tools = (r or {}).get('result', {}).get('tools', [])
        want = sys.argv[2:]
        for t in tools:
            if not want or t.get('name') in want:
                print('== %s' % t.get('name'))
                print(json.dumps(t.get('inputSchema'), ensure_ascii=False)[:1200])
    elif cmd == 'look':
        r = rpc('tools/call', {'name': 'lookup_funcs', 'arguments': {'queries': sys.argv[2:]}})
        print(json.dumps(r, ensure_ascii=False, indent=1)[:6000], flush=True)
    elif cmd == 'dec':
        import os
        os.makedirs('_ida_out', exist_ok=True)
        for a in sys.argv[2:]:
            r = rpc('tools/call', {'name': 'decompile', 'arguments': {'addr': a, 'include_addresses': False}})
            sc = (r or {}).get('result', {}).get('structuredContent') or {}
            code = sc.get('code')
            if code is None:
                # fall back to the text payload
                try:
                    code = json.loads(r['result']['content'][0]['text']).get('code')
                except Exception:
                    code = json.dumps(r, ensure_ascii=False)
            safe = a.replace('0x', 'addr_').replace('/', '_')
            out = os.path.join('_ida_out', '%s.c' % safe)
            io.open(out, 'w', encoding='utf-8', newline='\n').write(code)
            print('==== %s -> %s (%d bytes)' % (a, out, len(code)), flush=True)
            print(code[:2500], flush=True)
    elif cmd == 'dis':
        for a in sys.argv[2:]:
            r = rpc('tools/call', {'name': 'disasm', 'arguments': {'addr': a}})
            print('==== %s' % a, flush=True)
            print(json.dumps(r, ensure_ascii=False, indent=1)[:14000], flush=True)
    elif cmd == 'call':
        name = sys.argv[2]
        raw = sys.argv[3] if len(sys.argv) > 3 else '{}'
        if raw.startswith('@'):
            raw = io.open(raw[1:], encoding='utf-8').read()
        args = json.loads(raw)
        r = rpc('tools/call', {'name': name, 'arguments': args})
        print(json.dumps(r, ensure_ascii=False, indent=1)[:12000], flush=True)
    elif cmd == 'raw':
        name = sys.argv[2]
        raw = sys.argv[3] if len(sys.argv) > 3 else '{}'
        if raw.startswith('@'):
            raw = io.open(raw[1:], encoding='utf-8').read()
        print(json.dumps(rpc(name, json.loads(raw)), ensure_ascii=False, indent=1)[:12000], flush=True)


main()
