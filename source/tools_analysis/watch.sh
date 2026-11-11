U=/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14; A="$U/SAN14_기록/적용확인.log"; n=$(grep -a -n "2026-11-02 17:17:2" "$A" | head -1 | cut -d: -f1)
tail -n +$n "$A" | awk -F'\t' '{print $1"\t"$2"\t"$4}' | sort | uniq -c | sort -rn | head -14
tail -n +$n "$A" | grep -a -E "^R42F|^R42H|^R40E|FAIL|SKIPPED|^R42T9" | tail -8 | cut -c1-200
python3 - <<'P'
import re
U='/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14'
t=open(U+'/SAN14CE_11Patch.log','rb').read().decode('utf-16-le','replace').splitlines()
print('IC 17시 이후 줄',sum(1 for l in t if l.startswith('2026-11-02 1') and l[11:13]>='17'),'길찾기 보호',sum(1 for l in t if l.startswith('2026-11-02 1') and l[11:13]>='17' and '길찾기 보호' in l))
m=open(U+'/SAN14_MinimapPerf.log',encoding='utf-8',errors='replace').read()
print('frame',re.findall(r'frames=(\d+) frame_ms avg=([\d.]+) max=([\d.]+)',m)[:6])
cl=open(U+'/SAN14_CrashLog.log',encoding='utf-8',errors='replace').read()
print('crash',re.findall(r'=== (2026-11-2 (?:1[7-9]|2\d)[^\r\n]*)',cl))
d=open('/mnt/user-data/uploads/SAN14CE_ModCore/runtime.log','rb').read();i=d.rfind(b'SAN14 ModCore 1.0');s=d[i:].decode('utf-8','replace')
print('runtime chars',len(s),'duel',s.count('DUEL60 active'))
P
grep -a month $U/SeasonSync_log.txt | tail -1
ls -la $U/SAN14Mods/PrisonerReturn/PrisonerReturn.log $U/SAN14Mods/Ext/SAN14_FastMenu.log /mnt/user-data/uploads/SAN14CE_ModCore/runtime.log "$A" | awk '{print $5,$9}'
echo "== 전 항목 카운터 마지막 값"
tail -n +$n "$A" | grep -a -E "^R39C" | awk -F'\t' '{k=$2" "$4; last[k]=$5" @"$6} END{for(k in last) print k, last[k]}' | sort -n
python3 - <<'P'
import re
d=open('/mnt/user-data/uploads/SAN14CE_ModCore/runtime.log','rb').read();i=d.rfind(b'SAN14 ModCore 1.0');s=d[i:].decode('utf-8','replace')
a=s.count('DUEL60 active');e=s.count('duel ended');print('FPS120 일기토 시작/종료',a,e,'(짝 안 맞으면 이상)')
for k in ['FAIL','failed','skipped','mismatch','not found','실패']:
    c=s.count(k)
    if c: print('runtime 경고어',k,c)
U='/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14'
fm=open(U+'/SAN14Mods/Ext/SAN14_FastMenu.log',encoding='utf-8',errors='replace').read()
print('FastMenu 붙잡음 마지막',re.findall(r'붙잡음 (\d+)',fm)[-1:])
P
