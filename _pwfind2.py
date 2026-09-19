# -*- coding: utf-8 -*-
"""Try the driver's stored-password formulas against the test accounts."""
import hashlib
import itertools

SALT = '20070201'
TARGETS = {
    '1': 'A1DCB9CC45C150193FBB4611FAC6DBE5',
    '11': '017ED13261F82BC453F2615F5BAD1769',
    'a': '42929920580FDA6B48A4DBBEAC729791',
    'b': '0E249716C69F0CB31EB3BD638C30F57F',
    '12': 'D0C30049C7E560172C826D4B41F0C312',
    '23': '1379AD102CA90883C63892D828D6815A',
}


def M(s):
    return hashlib.md5(s.encode()).hexdigest().upper()


CANDS = ['1', '11', 'a', 'b', '12', '23', '123456', '111111', '000000',
         'admin', 'awesrd', 'test', 'aaa', 'gm', '2', '22', 'qwe123',
         'a123456', 'abc123', 'password', '123', '1234', '12345']


def formulas(acct, pwd):
    mu, ml = M(pwd), M(pwd).lower()
    yield 'acct+Md5(pwd)+salt', M(acct + mu + SALT)
    yield 'acct+md5(pwd)+salt', M(acct + ml + SALT)
    yield 'acct+pwd+salt', M(acct + pwd + SALT)
    yield 'Md5(pwd)+salt', M(mu + SALT)
    yield 'acct+pwd', M(acct + pwd)
    yield 'Md5(pwd)', mu
    yield 'acct+Md5(Md5(pwd))+salt', M(acct + M(mu) + SALT)
    yield 'pwd+salt', M(pwd + SALT)
    yield 'Md5(acct+pwd)', M(acct + pwd)
    yield 'acct+pwd+acct', M(acct + pwd + acct)


hits = 0
for acct, want in TARGETS.items():
    for pwd in CANDS:
        for name, got in formulas(acct, pwd):
            if got == want:
                print('HIT acct=%r pwd=%r formula=%s' % (acct, pwd, name))
                hits += 1
print('hits: %d' % hits)
