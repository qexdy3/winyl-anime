param(
    [Parameter(Mandatory=$true)][string]$RuntimeDir,
    [Parameter(Mandatory=$true)][string]$Compiler,
    [string]$OutputDir = (Join-Path $PSScriptRoot '..\dist'),
    [string]$StagingRoot = $env:TEMP,
    [switch]$TestBuild
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$runtime = (Resolve-Path $RuntimeDir).Path
$stage = Join-Path $StagingRoot ('WinylAnime-stage-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage, (Join-Path $stage 'Skin') | Out-Null
Copy-Item (Join-Path $repo 'Release\Winyl.exe') (Join-Path $stage 'Winyl.exe')
foreach ($name in @('bass.dll','bassasio.dll','bassmix.dll','basswasapi.dll','bass_fx.dll')) {
    Copy-Item (Join-Path $runtime $name) $stage
}
foreach ($name in @('Bass','Language','Equalizer')) {
    Copy-Item (Join-Path $runtime $name) $stage -Recurse
}
Copy-Item (Join-Path $runtime 'Skin\*.wzp') (Join-Path $stage 'Skin')
Copy-Item (Join-Path $repo 'themes\Tokyo_Ghoul_No_Blue_Icons') (Join-Path $stage 'Skin') -Recurse
Copy-Item (Join-Path $repo 'LICENSE') (Join-Path $stage 'License.txt')
[IO.File]::WriteAllText((Join-Path $stage 'Portable.dat'), '')
[IO.File]::WriteAllText((Join-Path $stage 'WinylAnime.install'), 'Winyl Anime installation marker')
[IO.File]::WriteAllText((Join-Path $stage 'Settings.xml'), '<Settings><Version ID="30301" Portable="1"/><Skin ID="Tokyo_Ghoul_No_Blue_Icons" Pack="0"/></Settings>')
[IO.File]::WriteAllText((Join-Path $stage 'ThirdParty.txt'), "BASS and BASS add-ons are distributed under their own terms: https://www.un4seen.com/bass.html`r`nTokyo Ghoul names and artwork may belong to their respective rights holders.`r`n")
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$output = (Resolve-Path $OutputDir).Path
$arguments = @("/DStageDir=$stage", "/O$output")
if ($TestBuild) { $arguments += '/DTestBuild' }
$arguments += (Join-Path $PSScriptRoot 'WinylAnime.iss')
& $Compiler @arguments
if ($LASTEXITCODE -ne 0) { throw "ISCC failed with exit code $LASTEXITCODE" }
Write-Host "Installer: $(Join-Path $output 'WinylAnime_Setup_x86.exe')"
