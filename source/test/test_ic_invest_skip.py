# P205(2026-11-09 사용자): 2번 '턴 종료 명승 자동 투자'가 안 됨(새 판 88턴 중 투자 0턴, '구조/비용 확인 실패' 15턴).
#  원인: 통합편의 투자 한 건(IC+0x40480)이 '지금 발전시킬 수 없는 명승'(비용<1·기간 0)에 -1 을 돌려주고, 턴 끝 고리(IC+0x6933)가 -1 에서 남은 명승을 전부 건너뜀.
#  고침(ICGuard 묶음 I): 값 범위 밖 출구(IC+0x4067c)의 -1 → 0. 쓴 뒤 확인이 어긋난 출구(IC+0x40673)는 그대로.
# 이 시험: 꾸러미의 통합편의 바이트에서 ① IC+0x4067c 로 뛰는 곳이 '값 범위 검사 4곳'뿐인지(상태를 바꾸기 전) ② 쓴 뒤 확인 실패는 다른 출구인지
#          ③ 턴 끝 고리가 -1 에서 멈추고 1 일 때만 세는지 ④ ICGuard 소스가 그 자리만 그 바이트로 바꾸는지
#  실행: python3 test/test_ic_invest_skip.py
import struct,sys,os,re
R=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
d=open(R+'/overlay_r43/pkg/support/payload/base/SAN14_IntegratedConvenience.dll','rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];o=struct.unpack_from('<H',d,pe+20)[0]
secs=[struct.unpack_from('<8sIIII',d,pe+24+o+40*k) for k in range(n)]
def off(r):
    for nm,vs,va,rs,rp in secs:
        if va<=r<va+max(vs,rs):return r-va+rp
def rd(r,k):return d[off(r):off(r)+k]
fails=0
def ck(c,m):
    global fails;print(('  ok   ' if c else '  FAIL ')+m);fails+=0 if c else 1
F0,F1=0x40480,0x4069d
ck(rd(0x40480,7)==bytes.fromhex('488bc448895810'),'투자 함수 입구 IC+0x40480 = mov rax,rsp; mov [rax+0x10],rbx (ICGuard 가 감싸는 7바이트)')
ck(rd(0x4067c,9)==bytes.fromhex('b8ffffffffeb0233c0'),'IC+0x4067c = mov eax,-1; jmp +2; xor eax,eax (값 범위 밖 출구)')
ck(rd(0x40673,7)==bytes.fromhex('beffffffff8bc6'),'IC+0x40673 = mov esi,-1; mov eax,esi (쓴 뒤 확인 실패 출구)')
# 함수 안의 jcc rel32(0f 8x) 대상
far={}
b=rd(F0,F1-F0)
for i in range(len(b)-6):
    if b[i]==0x0f and 0x80<=b[i+1]<=0x8f:
        t=F0+i+6+struct.unpack_from('<i',b,i+2)[0]
        if F0<=t<F1+0x10:far.setdefault(t,[]).append(F0+i)
ck(sorted(far.get(0x4067c,[]))==[0x40592,0x405a1,0x405aa,0x405ba],'IC+0x4067c 로 뛰는 곳 = 값 범위 검사 4곳(비용<1 · 기간 0/과대 · 날짜<0 · 날짜 넘침): %s'%[hex(x) for x in sorted(far.get(0x4067c,[]))])
ck(rd(0x4058f,3)==bytes.fromhex('443bee') and rd(0x40598,3)==bytes.fromhex('8d4fff'),'그 검사들은 비용(r13d)·기간(edi)을 보는 자리')
first_write=0x405c9
ck(rd(first_write,4)==bytes.fromhex('c6456402'),'명승 상태를 처음 쓰는 자리 IC+0x405c9 (mov byte [rbp+0x64],2)')
ck(all(s<first_write for s in far.get(0x4067c,[])),'값 범위 검사 4곳은 전부 상태를 쓰기 전 → 0 으로 바꿔도 아무것도 안 바뀐 채 건너뜀')
near=[F0+i for i in range(len(b)-2) if b[i]==0x75 and F0+i+2+struct.unpack_from('<b',b,i+1)[0]==0x40673 and F0+i>first_write]
ck(len(near)>=4,'쓴 뒤 확인이 어긋나면 IC+0x40673(-1 그대로)로 감: %d곳'%len(near))
# 턴 끝 고리
ck(rd(0x6933,5)[0]==0xe8 and 0x6933+5+struct.unpack_from('<i',rd(0x6933,5),1)[0]==0x40480,'턴 끝 고리 IC+0x6933 이 투자 함수를 부름')
ck(rd(0x6938,12)==bytes.fromhex('85c0786983f8017503ff470c'),'고리: 음수면 중단(js) · 1 일 때만 투자 수 +1 · 0 이면 다음 명승으로')
s=open(R+'/src/ic_guard.cpp',encoding='utf-8').read()
ck('tx.bytes(ic+0x4067c,before,after,5);' in s and 'before[]={0xb8,0xff,0xff,0xff,0xff},after[]={0x31,0xc0,0x90,0x90,0x90}' in s,'ICGuard 묶음 I: IC+0x4067c 5바이트만 mov eax,-1 → xor eax,eax; nop3')
ck('tx.detour(ic+0x40480,e,7,(void*)w_invest)' in s and 'invest_skip' in s,'ICGuard 묶음 I: 건너뛴 명승의 단계·비용·기간을 기록(자가진단)')
ck('0x40673' in s and 'i_done=install_i()' in s,'설치 호출과 설명 있음')
# P205 확정(11-10 실게임 2턴: 구당협 Lv1 '투자가능'·설정 켬인데 투자 0곳, 투자 함수가 불리지도 않음): 턴 끝 고리 들머리의 문지기
#  IC+0x686d  test byte [정책 ic+0x105010],2 → 0 이면 투자 단계 통째로 건너뜀.  IC+0x687a 켬 칸(ic+0x10500c) 도 0 이면 건너뜀.
#  사용자 ini: AutoScenery=1 인데 AutoPersonnelState=196617(0x30009) → 비트 2 가 꺼져 있음. 옛 설정 창은 켬 칸만 쓰고 정책 비트를 안 켰다.
ck(rd(0x686d,7)[:2]==bytes.fromhex('f605') and rd(0x686d,7)[6]==2 and 0x686d+7+struct.unpack_from('<i',rd(0x686d,7),2)[0]==0x105010,'문지기 1: IC+0x686d 정책(ic+0x105010) 비트 2 검사')
ck(rd(0x687a,7)[:3]==bytes.fromhex('0fb605') and 0x687a+7+struct.unpack_from('<i',rd(0x687a,7),3)[0]==0x10500c,'문지기 2: IC+0x687a 켬 칸(ic+0x10500c) 검사')
ck((0x30009&2)==0,'사용자 저장값 196617 은 투자 비트가 꺼진 상태(버그 재현 근거)')
ck('auto_policy_bits_fixed' in s and 'if(at<U8>(ic,0x10500c))want|=2;if(at<U8>(ic,0x10500d))want|=4;' in s and 'fn<U8(*)(uint64_t)>(ic,0x65580)' in s,'ICGuard 묶음 J: 켬 칸이 켜져 있으면 정책 비트를 맞춤(통합편의 정책 저장 함수로)')
m=open(R+'/src/mod_settings.cpp',encoding='utf-8').read()
ck('if(r.arg==12||r.arg==13){int bit=r.arg==12?2:4;' in m and 'if(r.arg==12)on=on&&(at<int>(ic,0x105010)&2);' in m,'설정 창: 켜고 끌 때 정책 비트도 같이, 화면 값은 둘 다 켜져야 켬')
print('전부 통과' if not fails else '실패 %d'%fails);sys.exit(1 if fails else 0)
