param([string]$PackageRoot = "dist/CartFoundry", [string]$OutputRoot = "dist")
$ErrorActionPreference = "Stop"
Set-Location "$PSScriptRoot/.."
$PackageRoot = (Resolve-Path $PackageRoot).Path
New-Item -ItemType Directory -Force $OutputRoot | Out-Null
$OutputRoot = (Resolve-Path $OutputRoot).Path
$Metadata = Get-Content "$PackageRoot/resources/app-version.json" -Raw | ConvertFrom-Json
if ($Metadata.product -ne 'CartFoundry' -or $Metadata.platform -ne 'windows-x86_64') { throw 'Expected a Windows x86_64 package' }
python scripts/check-windows-runtime.py $PackageRoot
if ($LASTEXITCODE -ne 0) { throw 'Missing bundled runtime DLLs' }
$ISCC = Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1
if (!$ISCC) { $ISCC = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe' }
if (!(Test-Path $ISCC)) { throw 'Install Inno Setup 6 on the build machine' }
& $ISCC "/DPackageRoot=$PackageRoot" "/DOutputRoot=$OutputRoot" "/DAppVersion=$($Metadata.version)" "$PSScriptRoot/installer/cartfoundry.iss"
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed' }
$Installer = Join-Path $OutputRoot "CartFoundry-$($Metadata.version)-windows-x86_64-setup.exe"
$Hash = (Get-FileHash $Installer -Algorithm SHA256).Hash.ToLowerInvariant()
"$Hash  $([IO.Path]::GetFileName($Installer))" | Set-Content -Encoding ascii "$Installer.sha256"
Write-Output "Installer: $Installer"
