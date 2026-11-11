param([string]$GameDirectory='',[string]$AfterUpdate='')
$ErrorActionPreference = 'Stop'
try {
    $installerDir = $PSScriptRoot
    if ([string]::IsNullOrEmpty($installerDir)) { $installerDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
    if ([string]::IsNullOrEmpty($installerDir)) { throw '설치기 폴더 위치를 알 수 없습니다. ZIP을 새 폴더에 풀고 설치.cmd 로 실행해 주세요.' }
    $supportDir = Split-Path -Parent $installerDir
    $root = Split-Path -Parent $supportDir
    $manifestPath = Join-Path $supportDir 'manifest101.xml'
    $hashSource = Join-Path $installerDir 'ManifestHash.cs'
    foreach ($p in @($manifestPath, $hashSource, (Join-Path $installerDir 'App.cs'))) {
        if (-not (Test-Path -LiteralPath $p)) { throw ('설치 자료가 없습니다: ' + $p + "`r`n두 ZIP을 모두 같은 폴더에 풀었는지 확인해 주세요.") }
    }
    $candidates = @(
        [System.IO.Path]::Combine($env:WINDIR, 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'),
        [System.IO.Path]::Combine($env:WINDIR, 'Microsoft.NET\Framework\v4.0.30319\csc.exe'))
    $csc = $null
    foreach ($c in $candidates) { if ([System.IO.File]::Exists($c)) { $csc = $c; break } }
    if ($null -eq $csc) { throw '.NET Framework C# 컴파일러(csc.exe)를 찾을 수 없습니다. Windows 10/11 기본 구성에 포함되어 있습니다.' }
    # 설치 자료 목록 무결성: manifest101.xml 의 SHA-256 이 소스에 박힌 값과 같아야 함
    $expected = ([regex]::Match([System.IO.File]::ReadAllText($hashSource), 'Value="([0-9a-f]{64})"')).Groups[1].Value
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $manifestBytes = [System.IO.File]::ReadAllBytes([string]$manifestPath)
    $actual = ([System.BitConverter]::ToString($sha256.ComputeHash($manifestBytes)) -replace '-', '').ToLowerInvariant()
    if ($actual -ne $expected) { throw '설치 자료 목록(manifest101.xml)이 손상되었습니다. ZIP 전체를 새 폴더에 다시 풀어 주세요.' }
    $sources = @('Core.cs','Spec.cs','Stat.cs','Engine.cs','ManifestHash.cs','Update.cs','App.cs') | ForEach-Object { [System.IO.Path]::Combine($installerDir, $_) }
    $out = [System.IO.Path]::Combine($installerDir, 'SAN14CE_Patch101.exe')
    if ([System.IO.File]::Exists($out)) {
        try { [System.IO.File]::Delete($out) }
        catch { throw ('설치기가 이미 켜져 있습니다. 켜져 있는 통합패치 설치기 창을 모두 닫고 설치.cmd 를 다시 실행해 주세요. (원인: ' + $_.Exception.Message + ')') }
    }
    $compileArgs = @('/nologo','/codepage:65001','/platform:x64','/optimize+','/warnaserror+','/target:winexe','/main:San14Patch101.Program',
        '/reference:System.dll','/reference:System.Core.dll','/reference:System.Xml.dll','/reference:System.Xml.Linq.dll',
        '/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.IO.Compression.dll','/reference:System.IO.Compression.FileSystem.dll','/reference:System.Web.Extensions.dll',
        ('/out:' + $out)) + $sources
    $buildOutput = & $csc @compileArgs 2>&1
    $code = $LASTEXITCODE
    if ($code -ne 0 -or -not [System.IO.File]::Exists($out)) {
        throw ('설치기 생성 실패. 게임 파일은 변경하지 않았습니다. 종료 코드: ' + $code + "`r`n" + ($buildOutput -join "`r`n"))
    }
    # R43.28: 새 판 받기 뒤(옛 설치기가 이 판을 열 때) 게임 폴더·이전 판을 넘김
    $argList = @()
    if (-not [string]::IsNullOrEmpty($GameDirectory)) { $argList += @('--game', ('"' + $GameDirectory + '"')) }
    if (-not [string]::IsNullOrEmpty($AfterUpdate)) { $argList += @('--after-update', ('"' + $AfterUpdate + '"')) }
    if ($argList.Count -gt 0) { $p = Start-Process -FilePath $out -WorkingDirectory $root -ArgumentList $argList -PassThru -Wait }
    else { $p = Start-Process -FilePath $out -WorkingDirectory $root -PassThru -Wait }
    exit $p.ExitCode
} catch {
    $msg = $_.Exception.Message + "`r`n(installerDir=" + $installerDir + ")`r`n" + $_.ScriptStackTrace
    try { [System.IO.File]::WriteAllText([System.IO.Path]::Combine($PSScriptRoot, 'BUILD_ERROR.txt'), $msg, [System.Text.Encoding]::UTF8) } catch {}
    Write-Host $msg
    # R39: 조용히 끝나지 않는다 — 원인·해결과 오류 전문을 복사 가능한 창으로
    try {
        Add-Type -AssemblyName System.Windows.Forms | Out-Null
        $hint = "설치기를 시작하지 못했습니다. 게임 파일은 바꾸지 않았습니다.`r`n`r`n" +
            "자주 있는 원인과 해결:`r`n" +
            " 0) 설치기 창이 이미 켜져 있으면 그 창을 모두 닫고 다시 실행 ('액세스가 거부되었습니다' 가 보이면 이 경우)`r`n" +
            " 1) 분할 ZIP(1of n … nof n)을 전부 같은 '새 폴더'에 풀었는지 확인 (하나라도 빠지면 자료 없음)`r`n" +
            " 2) Program Files, OneDrive, 한글/특수문자가 섞인 아주 긴 경로는 피하고 C:\통합패치 같은 폴더에 풀기`r`n" +
            " 3) 백신이 설치기(SAN14CE_Patch101.exe)를 지웠거나 막았으면 백신 예외에 그 폴더 추가 후 다시 실행`r`n" +
            " 4) 'csc.exe 를 찾을 수 없습니다' 면 Windows 의 .NET Framework 4.x 가 꺼져 있는 것 → Windows 기능 켜기/끄기에서 .NET Framework 4.8 켜기`r`n`r`n" +
            "아래 글자를 전부 복사해 댓글로 알려 주시면 원인을 찾습니다.`r`n`r`n---- 오류 전문 ----`r`n" + $msg
        $f = New-Object System.Windows.Forms.Form
        $f.Text = '통합패치 - 설치기 시작 실패'; $f.Width = 820; $f.Height = 560; $f.StartPosition = 'CenterScreen'
        $t = New-Object System.Windows.Forms.TextBox
        $t.Multiline = $true; $t.ReadOnly = $true; $t.ScrollBars = 'Both'; $t.Dock = 'Fill'; $t.Font = New-Object System.Drawing.Font('Malgun Gothic', 10); $t.Text = $hint
        $b = New-Object System.Windows.Forms.Button; $b.Text = '전부 복사'; $b.Dock = 'Bottom'; $b.Height = 36
        $b.Add_Click({ [System.Windows.Forms.Clipboard]::SetText($t.Text); $b.Text = '복사됨' })
        $c = New-Object System.Windows.Forms.Button; $c.Text = '닫기'; $c.Dock = 'Bottom'; $c.Height = 36; $c.Add_Click({ $f.Close() })
        $f.Controls.Add($t); $f.Controls.Add($b); $f.Controls.Add($c)
        [void]$f.ShowDialog()
    } catch {}
    exit 1
}
