# 설치기(C#) 소스는 사용자 PC 의 설치.cmd 가 csc 로 빌드함 — 이 작업 공간엔 C# 컴파일러가 없어서, 고친 뒤 최소한의 문법 안전장치로 괄호 짝을 본다.
#  (7차 작업 중 줄 가운데 '//' 주석이 뒤 코드를 삼켜 중괄호가 하나 남는 실수를 이 검사로 잡았음)
#  + 7차 설치기 규칙: 연결 폴더 거부 없음 · 복구 실패는 보관 후 진행 · 깨진 기록은 보관 후 새로 · 상태 확인은 파일 하나 실패로 멈추지 않음
import re,glob,sys
fails=0
def check(c,m):
    global fails
    print(('  ok   ' if c else '  FAIL ')+m);fails+=0 if c else 1
def strip(t):
    t=re.sub(r'@"(?:[^"]|"")*"','""',t);t=re.sub(r'"(?:\\.|[^"\\\n])*"','""',t);t=re.sub(r"'(?:\\.|[^'\\])'","''",t);return re.sub(r'//[^\n]*','',t)
for f in sorted(glob.glob('/home/claude/r41/installer101/*.cs')):
    t=strip(open(f,encoding='utf-8-sig').read());n=f.split('/')[-1]
    check(t.count('{')==t.count('}') and t.count('(')==t.count(')') and t.count('[')==t.count(']'),n+' 괄호 짝')
core=open('/home/claude/r41/installer101/Core.cs',encoding='utf-8-sig').read();eng=open('/home/claude/r41/installer101/Engine.cs',encoding='utf-8-sig').read();app=open('/home/claude/r41/installer101/App.cs',encoding='utf-8-sig').read()
check('연결된 폴더/파일은 변경하지 않습니다' not in strip(core).replace('""','') and 'ReparsePoint)!=0)throw' not in core,'연결 폴더를 막지 않음')
check('try{RecoverCore();}' in core and '.broken_' in core,'복구 실패 → 복구 기록 보관 후 진행')
check('return s;\n  }' in eng and 'state.xml)을 읽지 못해 따로 보관' in eng,'깨진 설치 기록 → 보관 후 새 기록')
check('try{h=Hash(f.Target);}catch(Exception){ok=false;continue;}' in eng,'상태 확인: 파일 하나 실패로 멈추지 않음')
check('LostBackup' in eng and 'old.LostBackup.Contains(r)' in eng,'없어진 백업 → 제거 때 그 파일 남김')
check('lastLoadError' in app and 'RecoverNote' in app,'설치기 창: 진짜 이유 표시·복구 안내 기록')
print('전부 통과' if not fails else '실패 %d'%fails);sys.exit(1 if fails else 0)
