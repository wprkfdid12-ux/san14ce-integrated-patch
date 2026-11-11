# R43 시험4: 칸(RVA) 지정 4바이트 고침 — extra_bin_fixes(전역 치환)로는 위험한 것들. 두 번 돌려도 같음.
#  사용: python3 cell_fixes_r43.py <payload_new 루트>
import sys,os,struct
PAY=sys.argv[1]
FIX={ 'base/SAN14_IntegratedConvenience.dll':[
  (0x9b2d4,5,bytes.fromhex('e9c48dffff'),bytes.fromhex('e9148effff'),'0x18006ec40 game+0x89abf7 원본 jmp 기대 5바이트'),
  (0x6dad5+3,4,0x1410000,0x1410090,'BGM vtable 칸 lea(자리_대응 +0x3000 오역)'),(0x51c29+3,4,0x140d000,0x1410090,'BGM vtable 칸 스택 즉시값'),
  (0x51c17+3,4,0x140d1f8,0x1410288,''),(0x51c3b+3,4,0x140d008,0x1410098,''),(0x51c4d+3,4,0x140d020,0x14100b0,''),
  (0x511d4+3,4,0x13d0e80,0x13d3f10,'BGM 곡 ID 표'),(0x576d5+3,4,0x13d0e80,0x13d3f10,''),(0x6dc4d+3,4,0x13d0e80,0x13d3f10,''),
  (0x3c7f4+4,4,0x6c41ff,0x6c695f,'논공 동시 v2 기록'),(0x3c804+4,4,0x709733,0x70ba43,''),(0x3c874+4,4,0x1447e90,0x144af18,''),(0x106a18+0xc,5,bytes.fromhex('e8e01ef5ff'),bytes.fromhex('e8d01cf5ff'),'7번 포로 묶음 되돌리기용 원래 바이트(0x2e347b call 0x235150) — 실제 새 exe 바이트로'),(0x3e00f+1,4,0xfe5c1d69,0xfe5bed89,'고유무장 등록 lea 재지정 변위 상수 -(0x1a3e290+7)→-(0x1a41270+7) — 틀리면 lea 3곳이 표-0x2fe0 을 가리킴(3D installed() 실패·잘못된 읽기)')],
 'f27/SAN14Mods/Minimap/SAN14_FogTrial.dll':[(0x331a2+3,4,0x451361,0x453571,'cm_install or 상수'),(0x331b8+3,4,0x451362,0x453572,''),(0x331ce+3,4,0x451363,0x453573,''),(0x331a2+2,1,b'\xc8',b'\xc0','or r8→add r8: 새 기준 0x453570 은 비트4가 켜져 있어 rcx|base 가 rcx+base 와 달라짐(0x10~0x1c 칸 오독)'),(0x331b8+2,1,b'\xc8',b'\xc0',''),(0x331ce+2,1,b'\xc8',b'\xc0','')]}

FIX['f14/SAN14Mods/ModPack/SAN14_TraitHover.dll']=[(18192, 4, 4286546537, 4286536537, 'call rel32 = 훅-(game+자리+5): 자리 0x80a4a2'), (18204, 4, 4286545791, 4286535791, '자리 0x80a78c'), (18216, 4, 4286550698, 4286540698, '자리 0x809461'), (18227, 4, 4286549834, 4286539834, '자리 0x8097c1')]
FIX['f14/SAN14Mods/ModPack/SAN14_TraitHover.dll.v2']=[(18192, 4, 4286546537, 4286536537, 'call rel32 = 훅-(game+자리+5): 자리 0x80a4a2'), (18204, 4, 4286545791, 4286535791, '자리 0x80a78c'), (18216, 4, 4286550698, 4286540698, '자리 0x809461'), (18227, 4, 4286549834, 4286539834, '자리 0x8097c1')]
FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']+=[(123650, 4, 4288236534, 4288226598, 'feed90 xchg rel32 -(칸+4)'), (123673, 4, 4288230741, 4288220805, ''), (123696, 4, 4288218990, 4288209054, ''), (123719, 4, 4288218804, 4288208868, ''), (123740, 4, 4288217979, 4288208043, ''), (58720, 4, 4274783840, 4274773728, 'vtable 이 게임 .text 안인지 검사(끝 주소)'), (58727, 4, 4274787936, 4274777824, '')]
SLOT_TARGET={'f14/SAN14Mods/ModPack/SAN14_TraitHover.dll':[(0x92ad0,0x8ca1c0)],'f14/SAN14Mods/ModPack/SAN14_TraitHover.dll.v2':[(0x92ad0,0x8ca1c0)]}
for _k in ('f14/SAN14Mods/ModPack/SAN14_TraitHover.dll','f14/SAN14Mods/ModPack/SAN14_TraitHover.dll.v2'):
    FIX[_k]=FIX[_k]+[(0x7550,4,0x807db6,0x80a4c6,'기대 코드 기록 rva(바이트 그대로라 record_fields 가 안 봄)'),(0x7758,4,0x8080a0,0x80a7b0,'같은 16바이트 기록 둘째(함수대응+0x580)'),(0x77d0,4,0x8080d0,0x80a7e0,'stale_records_r43: 같은 함수 +0x5b0'),(0x7938,4,0x806d60,0x809470,'+0x360'),(0x7b40,4,0x8070c0,0x8097d0,'+0x6c0'),(0x7b68,4,0x8070d0,0x8097e0,'+0x6d0')]
# 시험12: 5번 진행 배속 — 통합편의가 게임의 '지금 진행 속도' 칸을 [게임+0x1a49ca4]로 읽음(옛 0x1a46cc4 를 +0x2fe0 으로 옮김). 그 칸이 든 구조체
#  (게터 0x3ec1a0 이 돌려주는 0x1a49c90, 옛 0x3e9c90 → 0x1a46ca0)는 +0x2ff0 옮겨졌고 필드 배치(0xe58·0x4c·0x10 사용)도 같음 → 옛 기준+0x24 = 0x1a49cb4.
#  엉뚱한 칸을 읽어 '같은 화살표 다시 누름'을 못 알아봐 2배가 한 번도 안 켜졌음(기록: 매번 "원래 화살표 속도"). upd_tools 자료 칸 정렬 검사(/tmp/claude-0/pt/datashift.py)로 찾음.
FIX['base/SAN14_IntegratedConvenience.dll']=FIX['base/SAN14_IntegratedConvenience.dll']+[(0x32d7f+3,4,0x1a49ca4,0x1a49cb4,'진행 속도 칸(구조체 0x1a49c90+0x24)'),(0x463d3+3,4,0x1a49ca4,0x1a49cb4,''),(0x4a5f9+3,4,0x1a49ca4,0x1a49cb4,''),(0x4a759+3,4,0x1a49ca4,0x1a49cb4,'')]
# 시험14: 9번 콜라보 해금(ModPack SAN14_Collab.dll) — db 구조 안 오프셋(전법 표 db+0x774d0, 개성 표 db+0x7dd18)을 apply_new 가 코드 주소로 보고 +0xfa0 옮김(0x78470/0x7ecb8).
#  새 exe 도 db+0x774d0(260곳)·db+0x7dd18(249곳)을 그대로 씀(옛 판과 같은 횟수) → 엉뚱한 표를 봐서 해금이 하나도 안 됨 → 게임이 불러올 때 콜라보 개성·전법을 가진 무장의
#  개성 전부를 지우고 전법을 1(분전)로 채움(세이브 대조: 10-30·11-04 판은 관은병·조운·제갈량 개성 있음, 오늘 판은 0 / 전법 01×10).
FIX['f09/SAN14Mods/ModPack/SAN14_Collab.dll']=[(0x10c1+3,4,0xdb90,0xdaf0,'시험14b: 설치 확인용 vtable 칸 기대 함수(옛 값 그대로였음 → 설치 검사 실패로 아무것도 안 함). 새 판 칸 값 = 함수 본문 같음'),(0x10d5+3,4,0xdb30,0xda90,''),(0x10e9+3,4,0xdb60,0xdac0,''),(0x10fd+3,4,0xdb00,0xda60,''),(0x11e0+4,4,0x7ecb8,0x7dd18,'개성 표 db 오프셋'),(0x120d+4,4,0x78470,0x774d0,'전법 표 db 오프셋'),(0x1420+3,4,0x7ecb8,0x7dd18,''),(0x1427+2,4,0x78470,0x774d0,'')]
import json as _json
PAIRS64={'base/SAN14_IntegratedConvenience.dll':_json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'ic_e2770_fix_r43.json')))}  # 0x3fe70 검사표 {slot,target}: .data/BSS 쪽 34칸(rdata→bss, bss→vtable) 미번역 → 모드 설정 v2 실패 원인
FIX['f09/SAN14_Traits9_UI.dll']=[(0x2fc4+3,4,0x212a0,0x215d0,'개성 탭: 두 게임 함수 고르기 차이 상수(0x210a60 / 0x232030). 옛 차이 0x212a0 그대로면 0x231d00 엉뚱한 곳 호출 → 튕김')]

# 시험7: 회귀 (x) switch 이분 탐색 경계값(= 다음 주소 - 1)·구간 끝값 — 주변 상수는 번역됐는데 경계만 옛값 (upd_tools/split_cmp_r43.py 로 찾음)
for _k in ('f14/SAN14Mods/ModPack/SAN14_TraitHover.dll','f14/SAN14Mods/ModPack/SAN14_TraitHover.dll.v2'):
    FIX[_k]=FIX[_k]+[(0x3b55+3,4,0x1460017,0x14630a7,'콜백 칸 switch 경계(0x1463090|0x14630a8 사이)'),(0x4cdc+1,4,0x1460017,0x14630a7,'콜백 검사 switch 경계(VNext 칸 0x1463090 이 엉뚱한 갈래로 감)')]
FIX.setdefault('base/SAN14_IntegratedConvenience.dll',[])
FIX['base/SAN14_IntegratedConvenience.dll']=FIX['base/SAN14_IntegratedConvenience.dll']+[(0x3f435+3,4,0x4562a0,0x458490,'0x14바이트 창(0x45847c) 덮는 기록 찾기 끝값 = 시작+0x14 (옛 0x45628c+0x14)')]
FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']=FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']+[(a+3,4,0x29fefff,0x2a01fff,'c48_code_matches 호출 대상 상한 = 이미지 크기-1') for a in (0x3504f,0x35089,0x350c3)]

# 시험10 전수 조사(census_r43.py + 썽크 검사): 27번 FogTrial 이 '레지스터에 게임 rva 를 넣어 두었다가 game 을 더해 call/jmp' 하는 자리 8곳 — 옛 판 값 그대로였음.
#  (옛 exe 의 jmp 썽크 0x450f80/0x9246b0/0x924fd0 는 새 판에 없어 대상 함수로 바로; 나머지는 같은 함수 새 자리. 머리 16바이트 대조 OK)
FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']=FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']+[
 (0x3f17+2,4,0xac9aa0,0xacc350,'name_restore call'),(0x2155f+1,4,0x2318b0,0x233310,'mini36_snapshot land call'),(0x239af+2,4,0x14cb00,0x14dae0,'terrain_build call'),
 (0x4e13+2,4,0x450f80,0x876580,'썽크→0x876580'),(0x53d0+1,4,0x450f80,0x876580,'썽크→0x876580'),(0x17c58+1,4,0x450f80,0x876580,'site_summary_poll 썽크→0x876580'),
 (0x578c+2,4,0x9246b0,0x87a690,'site_summary_release 썽크→0x87a690'),(0xed15+2,4,0x924fd0,0x87abc0,'hover36_tick 썽크→0x87abc0')]

# 시험9: 회귀 (xi) 'base 를 trampoline 기준으로 바꾼 뒤'(sub rdx,r8) 쓰는 게임 주소 상수 — upd_tools/rel_base_r43.py 로 찾음.
#  통합편의 0x44e2c1(45바이트) 후크의 되돌아가는 jmp 대상 0x44c569 가 옛값 → 새 판 CUserStrategyState::enter(0x44c340) 한가운데로 뛰어 턴 진행 때 튕김(exe+0x44c57a).
FIX['base/SAN14_IntegratedConvenience.dll']=FIX['base/SAN14_IntegratedConvenience.dll']+[(0x54225+1,4,[0x8044b5b9,0x8044d80a],0x8044d7f9,'되돌아가기 jmp rel32 범위 검사(0x80000000+대상)'),(0x5423d+3,4,[0x44c569,0x44e7ba],0x44e7a9,'되돌아가기 jmp 대상: 시험10 — 원래 게임의 건너뛰기 자리 0x44e7ba(블록 안 jne 대상). 시험9 의 0x44e7a9(옛 판과 같은 상대 자리)는 cmp [rsi+0x484],0x20 / jl 0x44e1f8 되돌이 고리 안이라, 전략 화면에서 통합편의가 0 을 돌려줄 때마다 고리를 다시 돌아 한 프레임 700ms(1 FPS) ⇒ 시험12 정정: 1 FPS 진짜 원인은 34번 RegionOfficer 였음. 0x44e7ba 로 뛰면 0x44e7b0 `mov rdi,[rbp-0x11]` 을 건너뛰어 턴 진행 때 exe+0x44e7cb 널 쓰기 튕김(15:28 CrashLog). 옛 0x44c569 와 바이트까지 같은 자리 0x44e7a9(원래 대응)로 되돌림')]

# 시험16: 통합편의 db 구조 오프셋·숫자 상수를 apply_new 가 코드 주소로 보고 옮긴 자리(db 포인터를 IC 전역 0x1801a4ed8 에서 읽어 와 기존 검사에 안 걸림).
#  새 exe 도 db+0x6caf0(373곳)·0xdd38(873)·0xded8(248)·0xe0d0(443)·0x871e8(1729)·0x78740(8)을 옛 판과 같은 횟수로 씀, 바뀐 값은 0곳.
#  0x180031c90 = 포로가 있는 거점(도시/지역/관문·항구) 찾기 — 7번 포로 일괄 처우·포로 목록. 0x1800643c0 = db·world 확인(포로 처우 진입 검사).
#  0x180054f18/84: 탐색 예산 70000+단계×2400 (숫자, 주소 아님).
FIX['base/SAN14_IntegratedConvenience.dll']=FIX['base/SAN14_IntegratedConvenience.dll']+[
 (0x31d9f+2,4,0x6d930,0x6caf0,'포로 거점: 지역 표 db+0x6caf0'),(0x31e09+2,4,0xdc98,0xdd38,'포로 거점: 도시 표 db+0xdd38'),(0x31e36+2,4,0xde38,0xded8,'포로 거점: 관문·항구 표 db+0xded8'),
 (0x31f08+2,4,0xdc98,0xdd38,''),(0x31fee+2,4,0xdc98,0xdd38,''),(0x32066+2,4,0xe030,0xe0d0,'군단 표 db+0xe0d0'),
 (0x64437+3,4,0x88218,0x871e8,'world db+0x871e8 (포로 처우 진입 검사)'),(0x58df3+3,4,0x796e0,0x78740,'db+0x78740'),
 (0x54f18+1,4,0x110d0,0x11170,'탐색 예산 상수 70000'),(0x54f84+1,4,0x110d0,0x11170,'')]

# 대조 전수(시험18 뒤): 22번 DomesticCE 안 수학 라이브러리(삼각함수 각도 줄이기 __rem_pio2)의 상수 0x921fb(π/2 상위 비트)를 apply_new 가 옛 함수 주소로 보고 0x9323b 로 바꿈 → π/2 근처 각도 정밀 경로를 안 탐. 되돌림.
FIX['f22/SAN14_DomesticCE.dll']=FIX.get('f22/SAN14_DomesticCE.dll',[])+[(0x4a60+1,4,0x9323b,0x921fb,'__rem_pio2 (ix&0xfffff)==0x921fb')]

# 대조 전수: 9번 Traits9_UI 사실무장/신무장 편집기 — 게임 두 자리(0x4e958b call, 0x542e53 jmp)를 패딩 동굴(0x494431)로 보내는 rel32 상수가 옛 판 값 그대로였음.
#  옛 값이면 call 대상이 0x494741(CSaveLoadDialog 함수 한가운데) → 편집기 열 때·장수 누를 때 rbx 깨짐·스택 4바이트 어긋남 → exe+0x92f39c 널 읽기 튕김(15:52·16:22 CrashLog).
FIX['f09/SAN14_Traits9_UI.dll']=FIX['f09/SAN14_Traits9_UI.dll']+[(0x1b71+7,4,0xfffab1b1,0xfffaaea1,'call rel32 0x4e958b→0x494431'),(0x1b84+7,4,0xfff518c9,0xfff515d9,'jmp rel32 0x542e53→0x494431')]

# 대조 전수: 27번 FogTrial(미니맵) 셰이더 진입점 이름 "vs" — 글자열 '\0vs\0'(=0x00737600)을 apply_new 가 옛 코드 주소로 보고 0x00739990 으로 바꿈 → D3DCompile 실패(hr=80004005) → 미니맵이 한 번도 안 그려짐(draws=0). 되돌림.
FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']=FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']+[(0x3d770,4,0x00739990,0x00737600,'"CreateDeviceContextState\\0vs\\0" 글자열')]
# 정식판: 27번 FogTrial 안 FEED90(옛 판 4쪽 패치, 새 판 자리 미이식) — 호출하는 곳이 없는 코드지만 어떤 경로로도 돌지 않게 막음
#  feed90_install 첫 바이트 ret, feed90_validate = xor eax,eax; ret (언제나 '맞지 않음')
FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']=FIX['f27/SAN14Mods/Minimap/SAN14_FogTrial.dll']+[(0x1e180,1,b'\x41',b'\xc3','feed90_install → ret'),(0x1e580,3,b'\x48\x8b\x0d',b'\x31\xc0\xc3','feed90_validate → 0')]
# 6차(11-08 20:2x, 포로 회유 제보 '등용 눌러도 메시지 없이 모두 거절' — 본인 PC 재현 "최근 등용: 처리 0명, 거절 0명, 제외 2명"):
#  장부 오염(자리_대응에 섞인 IC 내부 주소 0x1a3ed0·0x19e6a0)으로 apply_new 가 IC 자기 표 변위를 '번역'해 둔 2곳이 base_flow 복원에서 빠졌음.
#  ① 0x23d1d cmp r12,[r14+rbx*8+disp]: 포로별 억류 거점 표(목록 만들 때 0x57e13 이 IC+0x1a3ed0 에 씀)와 비교 → 0x1a51d0 이면 늘 다름 → 모든 포로 '제외'
#  ② 0x29c73 mov rax,[r13+rsi*8+disp]: 옛 설정 창 스위치 콜백 표 IC+0x19e6a0(0x2c61b 는 그대로) → 0x19f9a0(빈 BSS)이면 콜백 안 불림
#  1.0.1 IC(payload_cur)와 대조: IC 기준 레지스터 접근 중 바뀐 건 이 2곳뿐, rip 상대 변경 0 (test/test_ic_self_refs.py 가 강제).
FIX['base/SAN14_IntegratedConvenience.dll']=FIX['base/SAN14_IntegratedConvenience.dll']+[
 (0x23d1d+4,4,0x1a51d0,0x1a3ed0,'포로별 억류 거점 표 IC+0x1a3ed0 (7번 포로 회유 제외 버그)'),
 (0x29c73+4,4,0x19f9a0,0x19e6a0,'옛 설정 창 스위치 콜백 표 IC+0x19e6a0')]

# 9차(11-09, 사용자: 원본 확인 실패 자리가 있으면 그 자리를 쓰는 기능만 끄게): 통합편의 시작 대기(0x4b5a0 고리)는 해독 표시(exe+0xffcbe8 32바이트)
#  다음에 2,346곳(0xb7310 표)이 전부 원래 모양이 될 때까지 최대 3분 기다리고, 하나라도 다르면 1~7번 전부 안 켬("3분 안에 … 확인하지 못했습니다").
#  그런데 이 2,346곳은 '신무장·스탯 확장'(통합편의가 적용하지 않는 기능, 기록 "신무장·스탯 확장은 적용하지 않음") 자리이고, 그 표를 쓰는
#  기능(모델 등록 묶음 0x3ce20·0x3f1b0·0x3e490)은 뒤의 재확인(0x4b93f, 실패 시 sil=0)이 따로 막음. 1~7번 각 기능은 자기 자리를 스스로 확인(0x6e700 등)함.
#  → 대기 고리에서는 해독 표시만 보고 2,346곳 비교는 건너뜀(0x4b5c1 'xor ebx,ebx' → 'jmp 0x4b603'). 다른 모드가 그 자리 하나를 먼저 고쳐도
#    1~7번은 바로 켜지고, 그 표에 기대는 기능만 재확인에서 꺼짐.
FIX['base/SAN14_IntegratedConvenience.dll']=FIX['base/SAN14_IntegratedConvenience.dll']+[
 (0x4b5c1,2,b'\x33\xdb',b'\xeb\x40','시작 대기: 2,346곳 전부 원본 조건 빼고 해독 표시만 (재확인 0x4b93f 가 그 표 쓰는 기능만 막음)')]

def secs(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    return [struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
for rel,L in FIX.items():
    p=os.path.join(PAY,rel);d=bytearray(open(p,'rb').read());S=secs(d);n=0
    def fo(rva):
        for vs,va,rs,ra in S:
            if va<=rva<va+rs:return ra+rva-va
    for rva,ln,o,nw,why in L:
        f=fo(rva);obs=[x if isinstance(x,bytes) else struct.pack('<I',x) for x in (o if isinstance(o,list) else [o])];nb=nw if isinstance(nw,bytes) else struct.pack('<I',nw)
        cur=bytes(d[f:f+ln])
        if cur==nb:continue
        assert cur in obs,(rel,hex(rva),cur.hex());d[f:f+ln]=nb;n+=1   # o 가 목록이면 그 중 하나(이전 시험판 값 되돌리기용)
    # {kind,slot,target} 기록: slot 이 게임 자료 범위인 칸 바로 뒤 target 이 옛 값이면 교체
    for o,nw in SLOT_TARGET.get(rel,[]):
        for vs,va,rs,ra in S:
            for k in range(ra+4,ra+rs-4,4):
                if struct.unpack_from('<I',d,k)[0]==o and 0x1340000<=struct.unpack_from('<I',d,k-4)[0]<0x2a02000:
                    struct.pack_into('<I',d,k,nw);n+=1
    for rva,ns,nt in PAIRS64.get(rel,[]):
        f=fo(rva)
        if struct.unpack_from('<QQ',d,f)!=(ns,nt):struct.pack_into('<QQ',d,f,ns,nt);n+=1
    if n:open(p,'wb').write(d)
    print(rel,'고침',n,'칸')

# R43 8차: 14번 TraitHover 한 파일로(9·13 유무와 상관없이 같은 파일, 화면 부품이 실제로 켜졌는지 스스로 봄) — upd_tools/traithover_unify_r43.py
import subprocess
_th=os.path.join(PAY,'f14/SAN14Mods/ModPack/SAN14_TraitHover.dll')
subprocess.check_call([sys.executable,os.path.join(os.path.dirname(os.path.abspath(__file__)),'traithover_unify_r43.py'),_th+'.v2',_th+'.v2',_th])
