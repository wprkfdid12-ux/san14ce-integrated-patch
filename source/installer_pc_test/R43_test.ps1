# 통합패치 R43 시험 (3번 방식): 최종판 복사 → R43 덮어쓰기 → 설치기 컴파일 → 설치기 자체 시험 → 설치기 창 띄우기
#  결과 기록: 이 파일 옆 R43_시험_결과.log (Claude 가 읽음). 게임 폴더는 설치기 창에서 '설치'를 누르기 전까지 안 건드림.
$ErrorActionPreference='Continue'
$here=Split-Path -Parent $MyInvocation.MyCommand.Path
$log=Join-Path $here 'R43_시험_결과.log'
function L($s){$t=(Get-Date).ToString('HH:mm:ss')+'  '+$s;Add-Content -LiteralPath $log -Value $t -Encoding UTF8;Write-Host $t}
Set-Content -LiteralPath $log -Value ('R43 시험 시작 '+(Get-Date).ToString('yyyy-MM-dd HH:mm:ss')) -Encoding UTF8
$final=Join-Path (Split-Path -Parent $here) '통합패치 11월 5일 최종판'
$test=Join-Path (Split-Path -Parent $here) 'R43_시험\R43_시험판'   # 옆에 다른 설치 자료 폴더가 없어야 '자료 ZIP 빠짐' 시험이 됨(설치기는 옆 폴더도 뒤짐)
$game='C:\Program Files (x86)\Steam\steamapps\common\Romance_of_the_Three_Kingdoms_14'
$ok=$true
# 처음 실행 때 만든 옛 시험판 폴더(통합패치\R43_시험판)는 지움 — 사용자 지시
$old=Join-Path (Split-Path -Parent $here) 'R43_시험판'
if(Test-Path -LiteralPath $old){try{Remove-Item -LiteralPath $old -Recurse -Force;L ('0) 옛 시험판 폴더 삭제: '+$old)}catch{L ('0) 옛 시험판 폴더 삭제 실패: '+$_.Exception.Message)}}
try{
 # 1) 시험판 폴더 = 최종판 복사(변경 없는 파일은 건너뜀)
 if(-not (Test-Path -LiteralPath $final)){throw ('최종판 폴더 없음: '+$final)}
 L ('1) 최종판 복사 → '+$test)
 $rc=(Start-Process robocopy -ArgumentList @('"'+$final+'"','"'+$test+'"','/E','/NFL','/NDL','/NJH','/NJS','/R:1','/W:1') -Wait -PassThru -NoNewWindow).ExitCode
 if($rc -ge 8){throw ('robocopy 실패 코드 '+$rc)}
 L ('   robocopy 코드 '+$rc+' (0~7 정상)')
 # 2) R43 덮어쓰기 ZIP 풀기
 $zips=Get-ChildItem -LiteralPath $here -Filter '통합패치_R43_덮어쓰기_*of*.zip' | Sort-Object Name
 if($zips.Count -lt 1){throw 'R43 덮어쓰기 ZIP 이 없음'}
 foreach($z in $zips){L ('2) 풀기 '+$z.Name);Expand-Archive -LiteralPath $z.FullName -DestinationPath $test -Force}
 # 2-1) SHA256SUMS 로 시험판 전체 확인
 $sums=Get-Content -LiteralPath (Join-Path $test 'SHA256SUMS.txt') -Encoding UTF8
 $bad=0;$n=0
 foreach($line in $sums){ if($line.Trim() -eq ''){continue}; $h=$line.Substring(0,64); $p=$line.Substring(66); $f=Join-Path $test ($p -replace '/','\')
  if(-not (Test-Path -LiteralPath $f)){L ('   없음: '+$p);$bad++;continue}
  $a=(Get-FileHash -LiteralPath $f -Algorithm SHA256).Hash.ToLower(); $n++
  if($a -ne $h){L ('   해시 다름: '+$p);$bad++} }
 L ('   SHA256SUMS 확인: '+$n+'개 중 다름/없음 '+$bad)
 if($bad -gt 0){$ok=$false}
 # 3) 설치기 컴파일(BuildAndRun.ps1 과 같은 인자)
 $inst=Join-Path $test 'support\installer101'
 $csc=$null
 foreach($c in @((Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'),(Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'))){ if(Test-Path -LiteralPath $c){$csc=$c;break} }
 if($null -eq $csc){throw 'csc.exe 없음'}
 $hashSource=Join-Path $inst 'ManifestHash.cs'
 $expected=([regex]::Match([IO.File]::ReadAllText($hashSource),'Value="([0-9a-f]{64})"')).Groups[1].Value
 $actual=(Get-FileHash -LiteralPath (Join-Path $test 'support\manifest101.xml') -Algorithm SHA256).Hash.ToLower()
 L ('3) manifest 해시 '+$(if($actual -eq $expected){'일치'}else{'불일치!'}))
 if($actual -ne $expected){$ok=$false}
 $src=@('Core.cs','Spec.cs','Stat.cs','Engine.cs','ManifestHash.cs','App.cs') | ForEach-Object { Join-Path $inst $_ }
 $exe=Join-Path $inst 'SAN14CE_Patch101.exe'
 if(Test-Path -LiteralPath $exe){Remove-Item -LiteralPath $exe -Force}
 $args1=@('/nologo','/codepage:65001','/platform:x64','/optimize+','/warnaserror+','/target:winexe','/main:San14Patch101.Program','/reference:System.dll','/reference:System.Core.dll','/reference:System.Xml.dll','/reference:System.Xml.Linq.dll','/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.IO.Compression.dll','/reference:System.IO.Compression.FileSystem.dll',('/out:'+$exe))+$src
 $out1=& $csc @args1 2>&1
 L ('   설치기 컴파일 코드 '+$LASTEXITCODE+' '+$(if(Test-Path -LiteralPath $exe){'(EXE 생성됨)'}else{'(EXE 없음!)'}))
 if($out1){$out1 | ForEach-Object { L ('   csc: '+$_) }}
 if($LASTEXITCODE -ne 0){$ok=$false}
 # 4) 설치기 자체 시험(가짜 게임 폴더, 임시 폴더에서만)
 $tsrc=@('Core.cs','Spec.cs','Stat.cs','Engine.cs','ManifestHash.cs') | ForEach-Object { Join-Path $inst $_ }
 $tsrc+= (Join-Path $here 'R43_Tests.cs')
 $texe=Join-Path $here 'R43_tests.exe'
 if(Test-Path -LiteralPath $texe){Remove-Item -LiteralPath $texe -Force}
 $args2=@('/nologo','/codepage:65001','/platform:x64','/optimize+','/target:exe','/main:San14Patch101.Tests','/reference:System.dll','/reference:System.Core.dll','/reference:System.Xml.dll','/reference:System.Xml.Linq.dll','/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.IO.Compression.dll','/reference:System.IO.Compression.FileSystem.dll',('/out:'+$texe))+$tsrc
 $out2=& $csc @args2 2>&1
 L ('4) 시험기 컴파일 코드 '+$LASTEXITCODE)
 if($out2){$out2 | ForEach-Object { L ('   csc: '+$_) }}
 if($LASTEXITCODE -eq 0){
  $work=Join-Path $env:TEMP 'san14_r43_tests'
  if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force}
  New-Item -ItemType Directory -Path $work | Out-Null
  $gexe=Join-Path $game 'SAN14CE_KO.exe'
  $font=Join-Path $game '.san14ce_compat_r3\blobs\a9bb649b4405a065f6330e70975a85a559cb8523af6c0df3ec63ea16685c6ae6'
  L ('   게임 exe 해시 '+(Get-FileHash -LiteralPath $gexe -Algorithm SHA256).Hash.ToLower())
  $tout=& $texe $test $work $gexe $font 2>&1
  $code=$LASTEXITCODE
  $tout | ForEach-Object { Add-Content -LiteralPath $log -Value ('   T: '+$_) -Encoding UTF8 }
  $pass=($tout | Where-Object { $_ -like 'PASS *' }).Count; $fail=($tout | Where-Object { $_ -like 'FAIL *' }).Count
  L ('   자체 시험 결과 코드 '+$code+'  PASS '+$pass+'  FAIL '+$fail)
  if($code -ne 0){$ok=$false}
  Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
 } else {$ok=$false}
}catch{ L ('오류: '+$_.Exception.Message); $ok=$false }
L ('끝: '+$(if($ok){'전부 통과'}else{'문제 있음 — 기록 확인'}))
Add-Type -AssemblyName System.Windows.Forms | Out-Null
if($ok){
 $r=[System.Windows.Forms.MessageBox]::Show("R43 시험판 준비·컴파일·자체 시험 전부 통과.`r`n`r`n지금 설치기 창을 띄울까요? (게임은 꺼져 있어야 함)`r`n설치기 창에서 항목 확인 후 '설치'를 누르면 게임 폴더에 깔립니다.","통합패치 R43 시험",[System.Windows.Forms.MessageBoxButtons]::YesNo)
 if($r -eq 'Yes'){ Start-Process -FilePath $exe -WorkingDirectory $test }
}else{
 [System.Windows.Forms.MessageBox]::Show("문제가 있어 설치기는 띄우지 않았습니다.`r`n기록: "+$log,"통합패치 R43 시험") | Out-Null
}
