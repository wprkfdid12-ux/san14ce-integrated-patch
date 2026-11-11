# apply_new.py 가 못 고치는 바이너리 안 4바이트 상수(함수 안 자리 등) — extra_bin_fixes.tsv 의 (파일, 옛, 새) 를 그대로 바꿈. 두 번 돌려도 같음.
import sys,struct
PAY=sys.argv[1]
for l in open(sys.argv[2],encoding='utf-8').read().splitlines()[1:]:
    if not l.strip():continue
    if l.startswith('#') or not l.strip():continue
    f,o,n,why=l.split('\t')[:4];p=PAY+'/'+f;b=bytearray(open(p,'rb').read());ob=int(o,16).to_bytes(4,'little');nb=int(n,16).to_bytes(4,'little')
    c=b.count(ob)
    if c==0:
        assert b.count(nb)>=1,(f,o,'옛 값도 새 값도 없음');print(f,o,'이미 고쳐짐');continue
    i=b.find(ob)
    while i>=0:b[i:i+4]=nb;i=b.find(ob,i+4)
    open(p,'wb').write(b);print(f,o,'→',n,c,'곳')
