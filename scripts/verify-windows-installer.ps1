param([Parameter(Mandatory=$true)][string]$Installer, [string]$Payload = "dist/CartFoundry")
$ErrorActionPreference = "Stop"
$Installer = (Resolve-Path $Installer).Path
$Payload = (Resolve-Path $Payload).Path
$Root = Join-Path $env:LOCALAPPDATA 'Programs/CartFoundry'
$State = Join-Path $env:LOCALAPPDATA 'CartFoundry/Installer'
$Key = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{D4963015-2157-4FC7-8ABD-0A102C9DD92A}_is1'
if (Test-Path $Root) { throw 'Installer test requires a clean disposable Windows runner' }
$Process = Start-Process -FilePath $Installer -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-' -Wait -PassThru
if ($Process.ExitCode -ne 0) { throw "Installer exited $($Process.ExitCode)" }
$Entry = Get-ItemProperty $Key
if ($Entry.InstallLocation.TrimEnd('\') -ne $Root.TrimEnd('\')) { throw 'Wrong installation location' }
$Uninstaller = $Entry.UninstallString.Trim('"')
if (!(Test-Path $Uninstaller) -or !(Split-Path $Uninstaller -Parent).Equals($State, [StringComparison]::OrdinalIgnoreCase)) { throw 'Uninstaller must be outside the replaceable app folder' }
$Shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'CartFoundry/CartFoundry.lnk'
if (!(Test-Path $Shortcut)) { throw 'Start menu shortcut missing' }
python scripts/verify-package.py $Root
if ($LASTEXITCODE -ne 0) { throw 'Installed app failed verification' }
# The real updater replaces the whole app directory. Its helper tests already
# cover the transaction; exercise the installer's bookkeeping across that swap.
$Old = Join-Path $env:RUNNER_TEMP 'CartFoundry-before-update'
Move-Item $Root $Old
Copy-Item -Recurse $Payload $Root
if (!(Test-Path $Uninstaller) -or !(Test-Path $Key) -or !(Test-Path $Shortcut)) { throw 'Whole-app replacement broke uninstall or shortcuts' }
python scripts/verify-package.py $Root
if ($LASTEXITCODE -ne 0) { throw 'Replaced installation failed verification' }
# Do not remove user-created projects even if one was saved in the app folder.
$Project = Join-Path $Root 'user-project.cartfoundry'
Set-Content $Project 'user project must survive uninstall'
$Process = Start-Process -FilePath $Uninstaller -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -Wait -PassThru
if ($Process.ExitCode -ne 0) { throw "Uninstaller exited $($Process.ExitCode)" }
for ($Attempt = 0; $Attempt -lt 30 -and (Test-Path "$Root/CartFoundry.exe"); $Attempt++) { Start-Sleep -Milliseconds 500 }
if ((Test-Path "$Root/CartFoundry.exe") -or (Test-Path $Key) -or (Test-Path $Shortcut)) { throw 'Uninstall left the app or registration installed' }
if (!(Test-Path $Project)) { throw 'Uninstall removed a user project' }
Write-Output 'Verified installer, installed ROM export and GUI, whole-app replacement, shortcuts, uninstall and user-project preservation'
