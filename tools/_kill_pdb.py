import subprocess
r = subprocess.run(['taskkill', '/F', '/PID', '17572'], capture_output=True)
print(r.stdout.decode('gbk', errors='replace'))
