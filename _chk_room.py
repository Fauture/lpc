import io
d = io.open('src/original_vm.cpp', 'rb').read()
print('CRLF', d.count(b'\r\n'), 'bareLF', d.count(b'\n') - d.count(b'\r\n'))
t = d.decode('utf-8')
print('roomLog def:', t.count('void roomLog('))
print('roomLog calls:', t.count('roomLog("'))
print('LPCVM_ROOM:', t.count('LPCVM_ROOM'))
