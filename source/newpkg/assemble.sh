#!/bin/bash
# R43 payload_new 조립: 바이너리 교체본(apply_new 결과) 위에 우리 모듈(build_out)·13번(mt/obj_new)·Diag.ini·데이터(f10/f31/f24) 를 얹는다
set -e; R=/home/claude/r41; W=$R/newpkg/payload_new
N=/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver
python3 $R/upd_tools/extra_bin_fixes.py $W $R/upd_tools/extra_bin_fixes.tsv
python3 $R/upd_tools/cell_fixes_r43.py $W
# 27번 FogTrial 서명 루프(apply_new 가 못 고치는 음수 변위·루프 끝·내장 서명) — 두 번 돌려도 같은 결과
python3 $R/upd_tools/fogtrial_fix.py $W/f27/SAN14Mods/Minimap/SAN14_FogTrial.dll $N/new.dec.exe $W/f27/SAN14Mods/Minimap/SAN14_FogTrial.dll | tail -1
python3 - <<'PY'
import os,shutil
R='/home/claude/r41';W=R+'/newpkg/payload_new';B=R+'/build_out';MT=R+'/mt/obj_new'
loc={}
for d,ds,fs in os.walk(W):
    for f in fs:loc.setdefault(f,[]).append(os.path.join(d,f))
n=0
for f in os.listdir(B):
    if not f.endswith('.dll') or f in('SAN14_PatchTester.dll','SAN14_SceneTest.dll'):continue
    tgt='SAN14_SettingsHelpScenic.dll.v2' if f=='SAN14_SettingsHelpScenic_domestic.dll' else f
    if tgt in loc:
        for p in loc[tgt]:shutil.copy(os.path.join(B,f),p);n+=1
    elif f=='SAN14_Diag.dll':os.makedirs(W+'/base',exist_ok=True);shutil.copy(os.path.join(B,f),W+'/base/SAN14_Diag.dll');n+=1
    elif f=='SAN14_FPS120Late.dll':shutil.copy(os.path.join(B,f),W+'/f19/SAN14_FPS120Late.dll');n+=1   # 2026-11-10 P201
    else:raise SystemExit('payload 에 자리 없음: '+f)
shutil.copy(MT+'/SAN14_MeritTraits.dll',loc['SAN14_MeritTraits.dll'][0]);shutil.copy(MT+'/SAN14_MeritResolver_CEKO.dll',loc['SAN14_MeritResolver_CEKO.dll'][0]);n+=2
os.makedirs(W+'/base/SAN14Mods/modules',exist_ok=True);shutil.copy(R+'/payload_r43/base/SAN14Mods/modules/Diag.ini',W+'/base/SAN14Mods/modules/Diag.ini')
NG='/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14'
os.makedirs(W+'/f10/0002',exist_ok=True);os.makedirs(W+'/f31/0002',exist_ok=True);os.makedirs(W+'/f24/SAN14Mods/DataToggle',exist_ok=True)
shutil.copy(R+'/newpkg/KIDSSystemResource.rdb.merged',W+'/f10/0002/KIDSSystemResource.rdb');shutil.copy(R+'/newpkg/ScreenLayout.rdb.merged',W+'/f31/0002/ScreenLayout.rdb')
shutil.copy(R+'/newpkg/data_new/fixdata_original_101.s14',W+'/f24/SAN14Mods/DataToggle/fixdata_original.s14')  # 새 판 원본(게임 폴더 사본은 24번이 깔리면 바뀌어 있어 쓰지 않음);shutil.copy(R+'/newpkg/fixdata_nostat_new.s14',W+'/f24/SAN14Mods/DataToggle/fixdata_nostat.s14')
# 2026-11-10 P201: 19번 등록을 '순서 경쟁을 없애는 부품'으로 돌림(본체 SAN14_FPS120.dll 은 그대로) / P202: 30번 휠클릭은 키트 소스 빌드 결과
open(W+'/f19/SAN14Mods/modules/FPS120.ini','wb').write('; 19번 120프레임 (R43 11-10: SAN14_FPS120Late.dll 이 본체 SAN14_FPS120.dll 을 싣고, 통합편의 6번 검사 동안 일기토 호출 자리를 원래대로 둔다)\r\n[Mod]\r\nId=SAN14CE.FPS120\r\nFile=SAN14_FPS120Late.dll\r\nEnabled=1\r\nCoreABI=65536\r\n'.encode('utf-8'))
shutil.copy(R+'/kits/WheelRegion/out/SAN14_WheelRegion.dll',W+'/f30/SAN14_WheelRegion.dll')
print('조립: 우리 모듈·13번',n,'개 + Diag.ini + 데이터 4개 + 19번 등록·30번 휠클릭')
PY
# R43 11번 3D: field_out_r43/normal (최종판 f11 에는 normal 하나만 — 12번 안개 1.6 미동봉)
mkdir -p "$W/f11/FieldLeader3D_Managed20" && cp /home/claude/r41/field_out_r43/normal/FieldLeader3D_Runtime.exe "$W/f11/FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe" && echo "f11 FieldLeader3D_Runtime.exe $(sha256sum "$W/f11/FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe" | cut -c1-16)"
