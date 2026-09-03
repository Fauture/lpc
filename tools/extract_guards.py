import re, glob
for f in sorted(glob.glob(r"tools\handler_dump\disasm_cmd_cfg_get_*.txt")):
    name = f.split("disasm_")[1].replace(".txt","")
    txt = open(f, encoding="utf-8", errors="replace").read()
    m = re.search(r"void main\([^)]*\) \{(.*?)\n\}", txt, re.S)
    if not m:
        print("===", name, ": no main body"); continue
    body = m.group(1)
    guards = re.findall(r"push_i\((\d+)\)", body)
    print("===", name, "| guards:", guards[:10])
