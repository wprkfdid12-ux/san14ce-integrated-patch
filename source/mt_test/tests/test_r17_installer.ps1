param([string]$SourceRoot)
# Run the actual installer/transaction code against isolated filesystem fixtures.
# PowerShell 7/Linux: only native DLL loading is substituted. Not Windows/game QA.
$ErrorActionPreference='Stop'
$SourceRoot=[IO.Path]::GetFullPath($SourceRoot)
$sandbox=Join-Path ([IO.Path]::GetTempPath()) ('s14r17_'+[Guid]::NewGuid().ToString('N'))
$pkg=Join-Path $sandbox 'pkg'
[IO.Directory]::CreateDirectory($pkg) | Out-Null
Copy-Item -LiteralPath (Join-Path $SourceRoot 'payload') -Destination $pkg -Recurse
Copy-Item -LiteralPath (Join-Path $SourceRoot 'Installer.Common.ps1') -Destination $pkg
Copy-Item -LiteralPath (Join-Path $SourceRoot 'package.manifest.json') -Destination $pkg
$report=@()
function Check($Value,[string]$Name) {
    if (-not $Value) { throw ('FAILED: '+$Name) }
    $script:report+=,@{name=$Name;pass=$true}
    Write-Host ('PASS '+$Name)
}
try {
    . (Join-Path $pkg 'Installer.Common.ps1')
    function Test-PrivateDllLoad($Manifest) { $script:probeCalls++ }
    $script:probeCalls=0
    $m=Read-Json (Join-Path $pkg 'package.manifest.json')
    $exeBytes=[Text.Encoding]::UTF8.GetBytes('isolated supported EXE fixture')
    $exe=Join-Path $sandbox 'fixture.exe';[IO.File]::WriteAllBytes($exe,$exeBytes)
    $m.SupportedExeSHA256=@(Hash-File $exe);$m.ReleaseReady=$true
    Write-JsonAtomic (Join-Path $pkg 'package.manifest.json') $m
    function New-Game([string]$Name) {
        $g=Join-Path $sandbox $Name;Ensure-Directory $g
        [IO.File]::WriteAllBytes((Join-Path $g 'SAN14CE_KO.exe'),$exeBytes)
        return $g
    }
    function Copy-Payload([string]$From,[string]$Game) {
        foreach ($p in Get-ChildItem -LiteralPath $From -File -Recurse) {
            $rel=$p.FullName.Substring($From.Length+1);$to=Join-Path $Game $rel
            Ensure-Directory ([IO.Path]::GetDirectoryName($to));[IO.File]::Copy($p.FullName,$to,$true)
        }
    }
    function Snapshot([string]$Game) {
        $out=@{};foreach ($f in Get-ChildItem -LiteralPath $Game -File -Recurse) {$out[$f.FullName]=Hash-File $f.FullName};return $out
    }
    function Same($Before) { foreach ($p in $Before.Keys) { if ((Hash-File $p) -ne $Before[$p]) { return $false } };return $true }
    function Install-Check([string]$Game,[string]$Loader) {
        Invoke-MeritInstall $Game;$s=Read-State $Game
        Check ($s.Active -and $s.SelectedLoader -eq $Loader) ('selected loader '+$Loader)
        Check ((Hash-File (Join-Path $Game 'SAN14_MeritTraits.dll')) -eq $m.Files['SAN14_MeritTraits.dll']) 'installed tested main DLL'
    }
    $g=New-Game 'clean';Install-Check $g 'dxgi.dll'
    $before=Snapshot $g;Invoke-MeritInstall $g;Check (Same $before) 'same-version reinstall leaves payload and rollback state unchanged'
    Invoke-MeritUninstall $g
    Check ((-not [IO.File]::Exists((Join-Path $g 'SAN14_MeritTraits.dll'))) -and (Hash-File (Join-Path $g 'dxgi.dll')) -eq $m.Loaders['dxgi.dll'].SHA256) 'uninstall removes only owned feature; preserves common loader'
    Invoke-MeritRollback $g;Check ((Read-State $g).Active) 'rollback restores removed feature state'
    Check ((Hash-File (Join-Path $g 'SAN14_MeritTraits.dll')) -eq $m.Files['SAN14_MeritTraits.dll']) 'rollback restores exact feature bytes'

    $related=Join-Path $SourceRoot 'references/related_officers_rc24/payload'
    foreach ($flavor in @('ProxyCompat1','Original')) {
        $g=New-Game $flavor;Copy-Payload $related $g
        if ($flavor -eq 'Original') {[IO.File]::Copy((Get-LoaderSource $m 'dxgi.dll'),(Join-Path $g 'dxgi.dll'),$true)}
        foreach ($n in @('version.dll','dinput8.dll','SAN14_Traits9.dll','SAN14_Traits9_UI.dll','SAN14_CaptiveDialog.dll')) {[IO.File]::WriteAllText((Join-Path $g $n),('unrelated '+$n))}
        $before=Snapshot $g;Install-Check $g 'dxgi.dll';Check (Same $before) ($flavor+': existing related officers and screenshot DLLs untouched')
        Invoke-MeritUninstall $g;Check (Same $before) ($flavor+': uninstall preserves related officers patch byte for byte')
    }
    foreach ($n in @('version.dll','dinput8.dll')) {
        $g=New-Game ('safe_'+$n);[IO.File]::Copy((Get-LoaderSource $m $n),(Join-Path $g $n))
        $before=Snapshot $g;Install-Check $g $n
        Check ((Same $before) -and -not [IO.File]::Exists((Join-Path $g 'dxgi.dll'))) ('SafeLoader reuse '+$n+' without a second proxy')
    }
    $g=New-Game 'unknown_dxgi';[IO.File]::WriteAllText((Join-Path $g 'dxgi.dll'),'unknown existing proxy')
    $before=Snapshot $g;Install-Check $g 'version.dll';Check (Same $before) 'unknown dxgi preserved while free version slot loads common Core'
    foreach ($kind in @('all_unknown','fake_marker')) {
        $g=New-Game $kind
        foreach ($n in $script:LoaderNames) {[IO.File]::WriteAllText((Join-Path $g $n),'unknown existing proxy')}
        if ($kind -eq 'fake_marker') {[IO.File]::WriteAllText((Join-Path $g 'dxgi.dll'),'SAN14Mods\Core\SAN14_ModCore.dll SAN14_CoreStart')}
        $before=Snapshot $g;$failed=$false
        try {Invoke-MeritInstall $g} catch {$failed=$true}
        Check ($failed -and (Same $before) -and -not [IO.File]::Exists((Join-Path $g 'SAN14_MeritTraits.dll'))) ($kind+': reject before any payload overwrite')
    }
    $g=New-Game 'r15_upgrade';$oldBase=Join-Path $SourceRoot 'audit/r15_release_baseline'
    Copy-Payload (Join-Path $oldBase 'payload') $g;$old=Read-Json (Join-Path $oldBase 'package.manifest.json')
    $entries=@();foreach ($n in $old.Files.Keys) {$entries+=,@{Name=$n;Managed=$true;BeforeHash='';Backup='';ExpectedHash=$old.Files[$n]}}
    Write-JsonAtomic (State-Path $g 'install-state.json') @{Format=1;Id=$m.Id;Version=$old.Version;Root=$g;Active=$true;Files=$entries;TransactionId='fixture'}
    # Reproduce installing a known relation-compatible loader after R15.
    [IO.File]::Copy((Join-Path $related 'dxgi.dll'),(Join-Path $g 'dxgi.dll'),$true)
    $shared=Hash-File (Join-Path $g 'dxgi.dll');Install-Check $g 'dxgi.dll'
    Invoke-MeritRollback $g
    Check ((Hash-File (Join-Path $g 'SAN14_MeritTraits.dll')) -eq $old.Files['SAN14_MeritTraits.dll']) 'R15 upgrade rollback restores original main DLL'
    Check ((Hash-File (Join-Path $g 'dxgi.dll')) -eq $shared) 'rollback never reverts the relation-compatible shared loader'
    Install-Check $g 'dxgi.dll'
    # External feature changes are refused, while unrelated mods remain intact.
    [IO.File]::WriteAllText((Join-Path $g 'SAN14_MeritTraits.dll'),'external change');$before=Snapshot $g;$failed=$false
    try {Invoke-MeritInstall $g} catch {$failed=$true}
    Check ($failed -and (Same $before)) 'externally modified feature is refused without overwriting'
    foreach ($tracked in @($true,$false)) {
        $g=New-Game ('r16_upgrade_'+$tracked);$oldBase=Join-Path $SourceRoot 'audit/r16_release_baseline'
        Copy-Payload (Join-Path $oldBase 'payload/SAN14Mods') (Join-Path $g 'SAN14Mods')
        [IO.File]::Copy((Join-Path $oldBase 'payload/SAN14_MeritTraits.dll'),(Join-Path $g 'SAN14_MeritTraits.dll'))
        [IO.File]::Copy((Join-Path $oldBase 'payload/loaders/dxgi.dll'),(Join-Path $g 'dxgi.dll'))
        $old=Read-Json (Join-Path $oldBase 'package.manifest.json')
        if ($tracked) {
            $entries=@();foreach ($n in $old.Files.Keys) {$entries+=,@{Name=$n;Managed=$true;BeforeHash='';Backup='';ExpectedHash=$old.Files[$n]}}
            Write-JsonAtomic (State-Path $g 'install-state.json') @{Format=1;Id=$m.Id;Version=$old.Version;Root=$g;Active=$true;Files=$entries;TransactionId='fixture';SelectedLoader='dxgi.dll'}
        }
        $shared=Hash-File (Join-Path $g 'dxgi.dll');Install-Check $g 'dxgi.dll';Invoke-MeritRollback $g
        Check ((Hash-File (Join-Path $g 'SAN14_MeritTraits.dll')) -eq $old.Files['SAN14_MeritTraits.dll']) ('R16 upgrade rollback restores exact main DLL: tracked='+$tracked)
        Check ((Hash-File (Join-Path $g 'dxgi.dll')) -eq $shared) 'R16 upgrade leaves shared loader unchanged'
    }
    # Inject a filesystem failure halfway through the actual commit and verify recovery.
    $g=New-Game 'interrupted';$realCopy=${function:Copy-Atomic};$script:copyCount=0
    function Copy-Atomic([string]$Source,[string]$Target,[string]$Expected) {
        $script:copyCount++;if ($script:copyCount -eq 3) {throw 'injected IO failure'}
        & $realCopy $Source $Target $Expected
    }
    $failed=$false;try {Invoke-MeritInstall $g} catch {$failed=$true}
    Check ($failed -and -not (Read-State $g)) 'interrupted first install leaves no active state'
    $left=@($script:CoreTargets | Where-Object {[IO.File]::Exists((Join-Path $g $_))})
    Check ($left.Count -eq 0) 'interrupted transaction removes partial feature/common payload'
    $result=@{dll_sha256=$m.Files['SAN14_MeritTraits.dll'];package_version=$m.Version;passed=$report.Count;tests=$report;powershell=$PSVersionTable.PSVersion.ToString();native_dll_loading_stubbed=$true;live_game_tested=$false}
    [IO.File]::WriteAllText((Join-Path $SourceRoot 'audit/r17_installer_tests.json'),($result | ConvertTo-Json -Depth 10))
} finally {
    if ([IO.Directory]::Exists($sandbox)) {[IO.Directory]::Delete($sandbox,$true)}
}
