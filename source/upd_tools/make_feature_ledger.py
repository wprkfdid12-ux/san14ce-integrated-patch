# 기능 대장: 34개 항목마다 깔리는 파일(DLL)·소스 유무·시작 기록 줄·동작 카운터·판정에 쓰는 기록 글자를 한 표로.
#  사용: python3 make_feature_ledger.py <manifest101.xml> <적용확인.log> <r41/build_out> > 기능_대장.tsv
import re,sys,os,collections
man,ap,bo=sys.argv[1:4]
NAMES=open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'판정.py'),encoding='utf-8').read()
NAMES=eval(re.search(r'NAMES=(\[.*?\])\n',NAMES,re.S).group(1))
files=collections.defaultdict(list)
for t,i in re.findall(r'<File target="([^"]+)"[^>]*id="(\d+)"',open(man,encoding='utf-8-sig').read()):files[int(i)].append(t)
built={f.lower() for f in os.listdir(bo) if f.endswith('.dll')}
starts=collections.defaultdict(set);cnt=collections.defaultdict(set)
for l in open(ap,encoding='utf-8',errors='replace'):
    c=l.rstrip('\n').split('\t')
    if c[0]=='R39' and len(c)>2 and c[1].isdigit():starts[int(c[1])].add(c[2])
    if c[0]=='R42D' and len(c)>3 and c[3]=='start':starts[int(c[1])].add(c[2]+'(R42D)')
    if c[0]=='R39C' and len(c)>3 and c[1].isdigit() and not c[3].startswith(('dump_','diag_')):cnt[int(c[1])].add(c[3])
judge=open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'판정.py'),encoding='utf-8').read()
keys=collections.defaultdict(list)
for i,body in re.findall(r"if i==(\d+):(.*?)\n    if",judge,re.S):keys[int(i)]+=re.findall(r"[RI]\('([^']+)'\)",body)
print('번호\t이름\t깔리는 DLL\t소스로 빌드\t시작 기록(적용확인.log)\t동작 카운터\t판정에 쓰는 기록 글자(runtime·IC 기록)')
for i in range(0,35):
    dl=[os.path.basename(f) for f in files[i] if f.lower().endswith('.dll')]
    print('\t'.join([str(i),'공통(기본, 모든 항목이 같이 씀)' if i==0 else NAMES[i-1],', '.join(dl) or '(파일만)',', '.join(d for d in dl if d.lower() in built) or '-',
      ', '.join(sorted(starts[i])) or '-',', '.join(sorted(cnt[i])) or '-',' / '.join(keys[i]) or '-']))
