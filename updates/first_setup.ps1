# 통합패치 처음 설치(깃허브에서 받은 분용): updates 폴더의 기본판 조각 + 최신판 파일을 풀어 '통합패치' 폴더를 만들고 설치기를 연다.
# R43.31(사용자 11-11): 폴더 이름에 판 번호를 붙이지 않음 — 이후 업데이트는 이 폴더에 그대로 덮어씀.
# R43.39(P256): 풀기 전에 경로 길이(Windows 260자 한도)를 미리 보고 막음 · 파일 하나씩 풀고 막히면 몇 번 다시 시도 ·
#   끝내 막히면 백신(랜섬웨어 보호) 안내 · 옛 커밋 화면에서 받은 ZIP 이면 알림(설치기 [업데이트]가 새 판을 알려 줌).
# 모든 파일은 SHA-256 으로 확인한다. 게임 폴더는 여기서 건드리지 않는다(설치는 설치기에서).
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
try {
    $root = Split-Path -Parent $here
    $idxPath = Join-Path $here 'index.json'
    if (-not (Test-Path -LiteralPath $idxPath)) { throw ('판 목록이 없습니다: ' + $idxPath + "`r`nZIP 전체를 풀었는지 확인해 주세요.") }
    $idx = Get-Content -LiteralPath $idxPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $rel = @($idx.releases)[0]
    if ($null -eq $rel) { throw '판 목록에 판이 없습니다.' }
    if ((Split-Path -Leaf $root) -match '-[0-9a-f]{40}$') {
        Write-Host ('알림: 이 ZIP 은 깃허브의 옛 커밋 화면에서 받은 것입니다(폴더 이름 끝 커밋 번호). 들어 있는 판: ' + $rel.tag)
        Write-Host '      설치기가 열리면 [업데이트] 버튼이 최신판을 알려 줍니다. 다음부터는 저장소 첫 화면에서 Download ZIP 을 받아 주세요.'
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem | Out-Null
    function Sha([string]$p) {
        $s = [System.Security.Cryptography.SHA256]::Create()
        $f = [System.IO.File]::OpenRead($p)
        try { return ([System.BitConverter]::ToString($s.ComputeHash($f)) -replace '-', '').ToLowerInvariant() } finally { $f.Dispose(); $s.Dispose() }
    }
    function Check-Zip($file, $sha) {
        $p = Join-Path $root ($file -replace '/', '\')
        if (-not (Test-Path -LiteralPath $p)) { throw ('파일이 없습니다: ' + $p + "`r`n저장소 ZIP 을 끝까지 받았는지, 압축을 전부 풀었는지 확인해 주세요.") }
        if ((Sha $p) -ne $sha) { throw ('파일이 손상됐습니다(지문 다름): ' + $p + "`r`n저장소 ZIP 을 다시 받아 주세요.") }
        return $p
    }
    $zips = @()
    foreach ($b in @($idx.base)) { $zips += ,@($b.file, $b.sha256, '기본판') }
    $zips += ,@($rel.file, $rel.sha256, ('최신판(' + $rel.tag + ')'))
    $dest = Join-Path $root '통합패치'
    if (Test-Path -LiteralPath $dest) { $dest = $dest + ' (' + (Get-Date -Format 'yyyyMMdd_HHmmss') + ')' }
    # 풀기 전: 지문 확인 + 가장 긴 파일 경로가 Windows 한도(260자) 안인지
    $paths = @(); $longest = ''
    foreach ($z in $zips) {
        $p = Check-Zip $z[0] $z[1]; $paths += $p
        $a = [System.IO.Compression.ZipFile]::OpenRead($p)
        try { foreach ($e in $a.Entries) { $full = $dest + '\' + ($e.FullName -replace '/', '\'); if ($full.Length -gt $longest.Length) { $longest = $full } } } finally { $a.Dispose() }
    }
    if ($longest.Length -gt 250) {
        throw ('풀 폴더 위치가 너무 깊습니다(가장 긴 파일 경로 ' + $longest.Length + '자, Windows 한도 260자).' + "`r`n" +
               '받은 ZIP 을 C:\통합패치받기 처럼 짧은 곳에 풀고 처음_설치.cmd 를 다시 실행해 주세요.' + "`r`n" +
               '(압축 풀기에서 같은 이름 폴더가 두 번 겹치면 특히 깊어집니다.)')
    }
    New-Item -ItemType Directory -Path $dest | Out-Null
    Write-Host ('통합패치 ' + $rel.tag + ' 를 이 폴더에 만듭니다: ' + $dest)
    $destFull = [System.IO.Path]::GetFullPath($dest).TrimEnd('\') + '\'
    for ($i = 0; $i -lt $zips.Count; $i++) {
        Write-Host ($zips[$i][2] + ' 푸는 중 ' + ($i + 1) + '/' + $zips.Count + ' ...')
        $a = [System.IO.Compression.ZipFile]::OpenRead($paths[$i])
        try {
            foreach ($e in $a.Entries) {
                if ($e.FullName.EndsWith('/')) { continue }
                $out = [System.IO.Path]::GetFullPath((Join-Path $dest ($e.FullName -replace '/', '\')))
                if (-not $out.StartsWith($destFull, [System.StringComparison]::OrdinalIgnoreCase)) { throw ('ZIP 안 파일 이름이 이상합니다: ' + $e.FullName) }
                [System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($out)) | Out-Null
                $err = $null
                for ($t = 1; $t -le 5; $t++) {
                    try { [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $out, $true); if ([System.IO.File]::Exists($out)) { $err = $null; break }; $err = '풀었는데 파일이 보이지 않음' }
                    catch { $err = $_.Exception.Message }
                    Start-Sleep -Milliseconds (400 * $t)
                }
                if ($null -ne $err) {
                    throw ('파일을 쓰지 못했습니다(5번 다시 시도): ' + $out + "`r`n원인: " + $err + "`r`n" +
                           '백신(랜섬웨어 보호·실시간 감시)이 막았을 수 있습니다. 백신 알림에서 허용한 뒤 처음_설치.cmd 를 다시 실행해 주세요.')
                }
            }
        } finally { $a.Dispose() }
    }
    Write-Host '파일 확인 중 ...'
    $bad = @()
    foreach ($line in [System.IO.File]::ReadAllLines((Join-Path $dest 'SHA256SUMS.txt'))) {
        if ($line.Length -lt 66) { continue }
        $sha = $line.Substring(0, 64).ToLowerInvariant(); $f = $line.Substring(64).Trim().TrimStart('*')
        $fp = Join-Path $dest ($f -replace '/', '\')
        $ok = $false
        for ($t = 1; $t -le 3 -and -not $ok; $t++) {
            try { $ok = [System.IO.File]::Exists($fp) -and ((Sha $fp) -eq $sha) } catch { $ok = $false }
            if (-not $ok) { Start-Sleep -Milliseconds 300 }
        }
        if (-not $ok) { $bad += $f }
    }
    if ($bad.Count -gt 0) {
        throw ('풀린 파일 ' + $bad.Count + '개가 목록과 다릅니다(예: ' + $bad[0] + ").`r`n" +
               '백신이 파일을 옮겼거나 막았을 수 있습니다. 백신에서 허용한 뒤 다시 실행하거나, 저장소 ZIP 을 다시 받아 주세요.')
    }
    Write-Host '완료. 설치기를 엽니다.'
    Start-Process -FilePath (Join-Path $dest '설치.cmd') -WorkingDirectory $dest
    exit 0
} catch {
    $msg = "통합패치 처음 설치를 끝내지 못했습니다. 게임 파일은 바꾸지 않았습니다.`r`n`r`n" + $_.Exception.Message + "`r`n`r`n이 글자를 그대로 복사해 댓글로 알려 주시면 원인을 확인합니다."
    Write-Host $msg
    try { Add-Type -AssemblyName System.Windows.Forms | Out-Null; [System.Windows.Forms.MessageBox]::Show($msg, '통합패치 - 처음 설치 실패') | Out-Null } catch {}
    exit 1
}
