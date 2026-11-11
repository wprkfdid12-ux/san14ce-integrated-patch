# R43.28: 설치기 업데이트(깃허브 새 판 받기) — 소스끼리 맞는지(C# 컴파일러가 여기 없어서 글자로 확인)
import re,sys
R='/home/claude/r41/installer101/';fails=0
def check(c,m):
    global fails
    print(('  ok   ' if c else '  FAIL ')+m);fails+=0 if c else 1
spec=open(R+'Spec.cs',encoding='utf-8-sig').read();up=open(R+'Update.cs',encoding='utf-8-sig').read();app=open(R+'App.cs',encoding='utf-8-sig').read()
ps=open(R+'BuildAndRun.ps1',encoding='utf-8-sig').read();ov=open('/home/claude/r41/build_overlay_r43.py',encoding='utf-8').read()
ver=re.search(r'public const string Version="([^"]+)"',spec).group(1)
check(re.fullmatch(r'R\d+\.\d+',ver) is not None,'Spec.Version 이 깃허브 판 번호 모양(R큰.차수): '+ver)
check("'Update.cs'" in ps and 'System.Web.Extensions.dll' in ps,'BuildAndRun 이 Update.cs 와 System.Web.Extensions 를 넣어 컴파일')
check("'Update.cs'" in ov,'꾸러미 만들 때 Update.cs 를 넣음')
check('[string]$AfterUpdate' in ps and "'--after-update'" in ps and "'--game'" in ps,'BuildAndRun 이 새 판 인자(--game·--after-update)를 넘김')
check('"--after-update"' in app and '"--game"' in app and 'mutex.WaitOne' in app,'설치기가 새 판 인자를 받고 옛 설치기 닫히기를 기다림')
check('Repository="wprkfdid12-ux/san14ce-integrated-patch"' in up,'깃허브 저장소 이름')
check('updates/index.json' in up and 'raw.githubusercontent.com/' in up and 'SafeFile' in up,'저장소 updates/index.json 판 목록을 읽음(경로는 updates/ 안만)')
check('updates/index.json' in open('/home/claude/r41/make_release_r43.py',encoding='utf-8').read(),'새 판 도구가 같은 판 목록을 만듦')
stamp=re.search(r'public const uint GameStamp=0x([0-9a-f]{8})u',up).group(1)
check(stamp in open('/home/claude/r41/make_release_r43.py',encoding='utf-8').read(),'배포 도구와 설치기의 게임판 값이 같음: '+stamp)
check('Expect(zip,r.AssetSha' in up and 'Files.HashFile(Native(work,kv.Key))!=kv.Value' in up,'받은 ZIP 지문·새 폴더 전체 지문 확인')
check('rel.Contains("..")' in up,'목록의 이상한 경로(..) 막음')
check(up.count('Directory.Move(work,dest)')==1 and 'Directory.Delete(dest' not in up,'같은 이름 폴더는 지우지 않고 옆으로 옮김')
check('StartUpdateCheck();' in app and app.count('Updates.Fetch()')==2,'깃허브는 켤 때 한 번(+버튼 누를 때만) 읽음 — 계속 읽지 않음')
check('ErrorBox.Show(this,"새 판 받기 실패"' in app and 'ErrorBox.Show(this,"공개판 확인 실패"' in app,'실패하면 자세한 오류 창')
check('체크 = 선택.' not in app,'제목 밑 설명 줄 삭제')
check('class ProgressForm' in up and up.count('p.Check()')>=5 and 'OperationCanceledException' in up and 'Directory.Delete(work,true);}catch(Exception){}' in up,'진행 창(단계·막대·취소) — 취소하면 만들던 새 폴더만 지움')
check('OpenNew=true' in up and 'Text="새 판 설치기 열기"' not in up and 'open.Visible' not in up,'끝나면 버튼 없이 바로 새 판 설치기를 엶(사용자 11-11)')
for w in ('버전 되돌리기…','"업데이트"','게임 폴더 변경…','"업데이트"','"작업 기록"','【다른 번호가 깔려 있어 함께 설치】'):check(w in app,'화면 글자: '+w)
# C# 5(.NET Framework csc)에서 안 되는 문법
for f in ('App.cs','Update.cs','Spec.cs','Engine.cs','Core.cs'):
    t=open(R+f,encoding='utf-8-sig').read();s=re.sub(r'"(?:\\.|[^"\\\n])*"','""',t);s=re.sub(r'//[^\n]*','',s)
    bad=[w for w,p in (('보간 문자열',r'\$"'),('?.',r'\w\?\.\w'),('nameof',r'\bnameof\('),('out var',r'\bout var\b')) if re.search(p,s)]
    check(not bad,f+' C# 5 문법만'+(' — '+','.join(bad) if bad else ''))
print('전부 통과' if not fails else '실패 %d'%fails);sys.exit(1 if fails else 0)
