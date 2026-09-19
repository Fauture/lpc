# -*- coding: utf-8 -*-
"""Recover the test accounts' plain passwords from the stored hashes using the
driver's own formula (builtins.cpp md5: md5("<account><pwd>20070201"))."""
import hashlib
import itertools
import string
import sys

SALT = '20070201'
TARGETS = {
    '1': 'A1DCB9CC45C150193FBB4611FAC6DBE5',
    '11': '017ED13261F82BC453F2615F5BAD1769',
    'a': '42929920580FDA6B48A4DBBEAC729791',
    'b': '0E249716C69F0CB31EB3BD638C30F57F',
    '12': 'D0C30049C7E560172C826D4B41F0C312',
    '23': '1379AD102CA90883C63892D828D6815A',
}


def h(acct, pwd):
    return hashlib.md5(('%s%s%s' % (acct, pwd, SALT)).encode()).hexdigest().upper()


def words():
    yield ''
    base = ['awesrd', 'Awesrd', 'AWESRD', 'gm', 'GM', 'admin', 'test', 'pass',
            'password', 'qwe', 'abc', 'aaa', 'bbb', 'pwd', 'root', 'user',
            'hello', 'world', 'love', 'china', 'wenda', 'asktao', 'wd']
    for w in base:
        yield w
    alpha = string.ascii_lowercase + string.digits
    for n in range(1, 4):
        for t in itertools.product(alpha, repeat=n):
            yield ''.join(t)
    # word + digits
    for w in base:
        for n in range(0, 3):
            for t in itertools.product(string.digits, repeat=n):
                yield w + ''.join(t)


found = {}
for pwd in words():
    for acct, want in TARGETS.items():
        if acct in found:
            continue
        if h(acct, pwd) == want:
            found[acct] = pwd
            print('acct=%s pwd=%r' % (acct, pwd), flush=True)
    if len(found) == len(TARGETS):
        break
print('found %d/%d' % (len(found), len(TARGETS)))
