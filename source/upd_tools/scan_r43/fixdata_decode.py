import sys,re,base64,struct
sys.argv=['x','/home/claude/r41/installer101/Stat.cs','/home/claude/r41/newpkg/payload_new/f24/SAN14Mods/DataToggle/fixdata_original.s14']
src=open('/home/claude/r41/upd_tools/fixdata_nostat.py',encoding='utf-8').read()
src=src.split("src=open(SRC,'rb').read()")[0]
exec(src)
raw,table,header=decode(open(SRC,'rb').read())
open('/tmp/claude-0/tac/raw.bin','wb').write(raw)
print(len(raw),COUNTS[:10])
