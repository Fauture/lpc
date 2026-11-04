# -*- coding: utf-8 -*-
"""Original-style driver command line per service (shared by all launchers).

Mirrors the original magic_Linux32 launchers (linux/<svc>/run<svc>), translated
to '-' option prefixes (the VM accepts both '-' and '/', the original parser
vm_analyseArguments 0x80b9a64 takes 45/'-' and 47/'/'):

  rungs:   ./magic_Linux32 /r ./ /e /gs/pack_data/lib_gs32.pak/gs/start_gs.o
                           /b /gs/pack_data/lib_gs32.pak -m gs/gsmem.cfg
                           /D _RELEASE /D CONFIG=gs/gs1.ini /D AUTH=
                           /D AUTH_MD5=... /D INIT_SOCKET_SCANNER_TEMP
                           /D TRADITIONAL --enable-socket-daemon 0 --stack-size 1024
  runaaa:  ... /D _RELEASE /D AUTH= /D AUTH_MD5=... /D INIT_SOCKET_SCANNER_TEMP
               /s 32 /D TRADITIONAL --max-process 128 --enable-socket-daemon 0
  rundba:  ... -m dba/dbamem.cfg /D _RELEASE /s 32 /D AUTH= /D AUTH_MD5=...
               /D TRADITIONAL /D MAX_SERVICE_THREAD_NUM=30 --max-process 128
               --enable-socket-daemon 0
  runccs:  ... -m ccs/ccsmem.cfg /D AUTH= /D AUTH_MD5=... /D _RELEASE
               /D TRADITIONAL --max-process 128 --enable-socket-daemon 0

Usage:  from _svc_args import svc_args
        subprocess.Popen([exe] + svc_args('gs'), ...)
"""
AUTH_MD5 = '767E51FE9E9A2F1EB5E689D74AA02426'


def svc_args(name):
    """Return the full lpcvm.exe argument list for one service."""
    pak = '/%s/pack_data/lib_%s32.pak' % (name, name)
    # -r <dir>: the physical data root holding <svc>/pack_data/... (the
    # original deployment's cwd; in this repo that layout lives under linux/).
    args = ['-r', 'linux',
            '-e', '%s/%s/start_%s.o' % (pak, name, name),
            '-b', pak]
    if name in ('gs', 'dba', 'ccs'):
        args += ['-m', '%s/%smem.cfg' % (name, name)]
    if name in ('gs', 'dba', 'aaa'):
        args += ['-D', '_RELEASE']
    if name == 'gs':
        args += ['-D', 'CONFIG=gs/gs1.ini']
    args += ['-D', 'AUTH=']
    args += ['-D', 'AUTH_MD5=%s' % AUTH_MD5]
    if name in ('aaa', 'gs'):
        args += ['-D', 'INIT_SOCKET_SCANNER_TEMP']
    if name in ('aaa', 'dba'):
        args += ['-s', '32']
    if name in ('gs', 'aaa', 'dba', 'ccs'):
        args += ['-D', 'TRADITIONAL']
    if name == 'dba':
        args += ['-D', 'MAX_SERVICE_THREAD_NUM=30']
    if name in ('aaa', 'dba', 'ccs'):
        args += ['--max-process', '128']
    args += ['--enable-socket-daemon', '0']
    if name == 'gs':
        args += ['--stack-size', '1024']
    return args
