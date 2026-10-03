# -*- coding: utf-8 -*-
"""登录失败诊断 —— 按用户要求查 **dba / ccs / gs** 三个服务的日志。

只读，不写任何东西。

usage:
  python -X utf8 _acct_diag.py [账号] [日志目录]
  python -X utf8 _acct_diag.py 7 gs_run

输出：
  1. lpcvm 进程（少一个就说明服务挂了 —— 这本身就是失败原因）
  2. 每个服务日志的大小/行数/末尾若干行
  3. 值得注意的行：DIALOG 弹窗、9xxx 错误码、close_client/踢线、拒绝/无接口、
     [crash]/assert、disconnect、filed 往返
  4. 本次账号在日志里出现的所有行（用于把客户端现象和服务端处理对上）
  5. 账号库/存档库里该账号的记录（只 SELECT）
"""
import io
import os
import re
import subprocess
import sys
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

acct = sys.argv[1] if len(sys.argv) > 1 else ''
logdir = sys.argv[2] if len(sys.argv) > 2 else 'gs_run'

PATTERNS = [
    ('DIALOG弹窗', r'DIALOG'),
    ('崩溃', r'\[crash\]|CRASH|Assertion|assert|segfault|access violation'),
    ('错误', r'\[err|\[error|ERROR|FATAL|fatal'),
    # 服务端错误码只在 DIALOG 行里出现（例如「原因：9101」），不要拿 9\d{3} 乱扫，
    # 否则 [recv-accept] 里的十六进制数据会被当成错误码（曾误报 23707 条）。
    ('错误码弹窗', r'原因[：:]\s*9\d{3}|9\d{3}\b(?=[^\d]|$)(?=.*(?:请求|角色|名字|登录))'),
    ('拒绝/无接口', r'拒绝|refus|NO INTERFACE|no interface|not found'),
    ('踢线/关连接', r'close_client|close-client|kick|踢'),
    ('断开', r'disconnect'),
    ('filed往返', r'\[send-filed\]|\[pkt-filed\]'),
    ('登录链路', r'do_login|create_new_char|load_existed_char|CHARTLIST|'
                 r'0x0472|0x4124|0x211B|0x205C|msg_l_auth|agent_result'),
]

TAIL = 15
LAST_HITS = 8


def procs():
    r = subprocess.run(['powershell', '-NoProfile', '-Command',
                        "Get-Process lpcvm -ErrorAction SilentlyContinue | "
                        "ForEach-Object { \"$($_.Id) $([int]($_.WorkingSet64/1MB))MB "
                        "start=$($_.StartTime.ToString('HH:mm:ss'))\" }"],
                       capture_output=True, text=True)
    return [l for l in (r.stdout or '').splitlines() if l.strip()]


def read_lines(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as fh:
        return fh.readlines()


def filed_link_report(logdir, acct=''):
    """0x3400 (read_content) request/reply accounting across gs and dba.

    This is the part that decides WHERE a 9030 broke:
      * gs sent  N requests, received M replies   -> M < N means replies were lost
      * dba received K requests, ran S SQL stmts  -> the handler must be reached
    Counted by simple substring rules on purpose: cookie-offset parsing of the
    hex bodies was wrong twice already (the logged body does not always start at
    the cmdNo), and a count is enough to localise the failure.
    """
    gs = os.path.join(logdir, 'live_gs.txt')
    dba = os.path.join(logdir, 'live_dba.txt')

    def count(path, pat):
        n = 0
        try:
            with open(path, 'r', encoding='utf-8', errors='replace') as fh:
                for l in fh:
                    if pat in l:
                        n += 1
        except Exception:
            return -1
        return n

    print('\n【2】filed 链路对账（0x3400 read_content）')
    g_send = count(gs, '[filed-rtt] send0x3400')
    g_recv = count(gs, '[filed-rtt] recv0x4401')
    g_park = count(gs, 'read_runtime_info')
    print('     gs  : 发出 0x3400 = %-6d 收到 0x4401 = %-6d 差 = %d'
          % (g_send, g_recv, g_send - g_recv))
    print('     gs  : read_runtime_info 相关行 = %d' % g_park)
    d_recv = count(dba, '[filed-rtt] recv0x3400')
    d_send = count(dba, '[filed-rtt] send0x4401')
    print('     dba : 收到 0x3400 = %-6d 发出 0x4401 = %-6d 差 = %d'
          % (d_recv, d_send, d_recv - d_send))
    d_sql = count(dba, '[mysql_select]')
    print('     dba : [mysql_select] = %d   (0 表示 SQL 都没执行)' % d_sql)
    if g_send > 0 and g_recv < g_send:
        print('     !! gs 有 %d 个请求没等到应答 —— 9030 的直接来源'
              % (g_send - g_recv))
    if d_recv > 0 and d_send < d_recv:
        print('     !! dba 收到 %d 个请求却只回了 %d 个 —— 处理函数没跑到回包'
              % (d_recv, d_send))


def main():
    print('=' * 78)
    print('登录失败诊断  账号=%r  日志目录=%s  %s'
          % (acct, logdir, time.strftime('%Y-%m-%d %H:%M:%S')))
    print('=' * 78)

    ps = procs()
    print('\n【1】lpcvm 进程：%d 个（正常应为 4：aaa/dba/ccs/gs）' % len(ps))
    for l in ps:
        print('     ', l)
    if len(ps) < 4:
        print('     !! 服务进程数不足 —— 服务挂了，这就是进不去游戏的原因')

    filed_link_report(logdir, acct)

    for name in ('gs', 'dba', 'ccs'):
        path = os.path.join(logdir, 'live_%s.txt' % name)
        print('\n' + '=' * 78)
        if not os.path.exists(path):
            print('【%s】日志不存在：%s' % (name, path))
            continue
        size = os.path.getsize(path)
        print('【%s】%s  %.1f MB  最后写入 %s'
              % (name, path, size / 1e6,
                 time.strftime('%H:%M:%S', time.localtime(os.path.getmtime(path)))))
        if size == 0:
            print('     !! 日志为空 —— 该服务可能启动就死了')
            continue
        try:
            lines = read_lines(path)
        except Exception as e:
            print('     读取失败：%s' % e)
            continue
        print('     行数=%d' % len(lines))
        print('     --- 末尾 %d 行 ---' % TAIL)
        for line in lines[-TAIL:]:
            print('     | ' + line.rstrip()[:190])

        counts = []
        for label, pat in PATTERNS:
            n = sum(1 for l in lines if re.search(pat, l))
            if n:
                counts.append('%s=%d' % (label, n))
        print('     --- 关键字统计 ---')
        print('     ' + ('  '.join(counts) if counts else '(无命中)'))

        for label, pat in (('DIALOG弹窗', r'DIALOG'),
                           ('踢线/关连接', r'close_client|kick|踢'),
                           ('拒绝/无接口', r'拒绝|refus|NO INTERFACE'),
                           ('崩溃', r'\[crash\]|Assertion|assert')):
            hits = [(i, l.rstrip()) for i, l in enumerate(lines) if re.search(pat, l)]
            if hits:
                print('     --- 最后 %d 条 %s（共 %d）---'
                      % (min(LAST_HITS, len(hits)), label, len(hits)))
                for i, line in hits[-LAST_HITS:]:
                    print('     %8d| %s' % (i, line[:190]))

        if acct:
            hits = [(i, l.rstrip()) for i, l in enumerate(lines) if acct in l]
            print('     --- 含账号 %r 的行：%d 条 ---' % (acct, len(hits)))
            for i, line in hits[-LAST_HITS:]:
                print('     %8d| %s' % (i, line[:190]))

    # 存档/账号库（只读）
    if acct:
        print('\n' + '=' * 78)
        print('【4】数据库里该账号的记录（只读 SELECT）')
        try:
            import pymysql
            c = pymysql.connect(host='192.168.200.128', user='root',
                                password='123456', charset='utf8')
            cur = c.cursor()
            cur.execute("SELECT account, blocked_time, temp_blocked_time, locked, "
                        "last_login_time, last_login_ip FROM dl_adb_all_test.account "
                        "WHERE account=%s", (acct,))
            print('     dl_adb_all_test.account ->', cur.fetchall())
            for db in ('dl_ddb_1_test', 'dl_mdb_1_test'):
                try:
                    cur.execute("SELECT COUNT(*) FROM %s.char_info WHERE account=%%s"
                                % db, (acct,))
                    print('     %s.char_info 行数=%s' % (db, cur.fetchone()[0]))
                except Exception as e:
                    print('     %s.char_info 查询失败: %s' % (db, e))
            for br in ('login', 'data1', 'carry'):
                try:
                    cur.execute("SELECT name, branch, LENGTH(content), time "
                                "FROM dl_ddb_1_test.data WHERE name=%s AND branch=%s",
                                (acct, br))
                    rows = cur.fetchall()
                    print('     ddb.data[%s] -> %s' % (br, rows if rows else '(无)'))
                except Exception as e:
                    print('     ddb.data[%s] 查询失败: %s' % (br, e))
            c.close()
        except Exception as e:
            print('     数据库查询失败：%s' % e)

    print('\n' + '=' * 78)
    return 0


if __name__ == '__main__':
    sys.exit(main())
