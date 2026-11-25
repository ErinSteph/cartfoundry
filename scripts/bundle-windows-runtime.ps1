param([string]$Root = "dist/CartFoundry", [ValidateSet("x86_64", "arm64")][string]$Architecture = "x86_64")
$ErrorActionPreference = "Stop"
$VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
$VisualStudio = & $VsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$VisualStudio) { throw "Cannot locate the Visual C++ redistributable tools" }
$TargetArch = if ($Architecture -eq "x86_64") { "x64" } else { "arm64" }
$Redist = Get-ChildItem (Join-Path $VisualStudio "VC/Redist/MSVC") -Directory -Recurse -Filter 'Microsoft.VC*.CRT' |
    Where-Object { $_.Parent.Name -eq $TargetArch } | Sort-Object FullName -Descending | Select-Object -First 1
if (!$Redist) { throw "Cannot locate the redistributable CRT for $TargetArch" }
Copy-Item (Join-Path $Redist.FullName '*.dll') $Root -Force
$Notice = Join-Path $Root "resources/licenses/msvc"
New-Item -ItemType Directory -Force $Notice | Out-Null
Get-ChildItem (Join-Path $Redist.FullName '*.dll') | ForEach-Object { "$($_.Name) $($_.VersionInfo.FileVersion)" } |
    Set-Content (Join-Path $Notice 'runtime-version.txt')
foreach ($Name in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    if (!(Test-Path (Join-Path $Root $Name))) { throw "Missing app-local runtime $Name" }
}
Write-Output "Bundled application-local C++ runtime from $($Redist.FullName)"
