# SPDX-FileCopyrightText: 2026 King Hallinta
# SPDX-License-Identifier: Apache-2.0

$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

if (-not $installation)
{
	Write-Host 'No Visual Studio installation with the C++ toolset was found.'
	exit 1
}

$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'

cmd /c "call `"$vcvars`" >nul && set" |
	Where-Object { $_ -match '^(PATH|INCLUDE|LIB|LIBPATH|VCINSTALLDIR|VCToolsInstallDir|VSINSTALLDIR|WindowsSdkDir|WindowsSDKVersion|UCRTVersion)=' } |
	ForEach-Object { Add-Content -Path $env:GITHUB_ENV -Value $_ }

Add-Content -Path $env:GITHUB_ENV -Value "VCPKG_ROOT=$env:VCPKG_INSTALLATION_ROOT"

if (-not (Get-Command ninja -ErrorAction SilentlyContinue))
{
	choco install ninja --no-progress --yes
}
