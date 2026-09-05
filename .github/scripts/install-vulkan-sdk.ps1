# SPDX-FileCopyrightText: 2026 King Hallinta
# SPDX-License-Identifier: Apache-2.0

$ErrorActionPreference = 'Stop'

$root = 'C:\VulkanSDK\current'
$installer = Join-Path $env:RUNNER_TEMP 'vulkan-sdk-installer.exe'

Invoke-WebRequest -Uri 'https://sdk.lunarg.com/sdk/download/latest/windows/vulkan_sdk.exe' -OutFile $installer

$arguments = @(
	'--root', $root,
	'--accept-licenses',
	'--default-answer',
	'--confirm-command',
	'install'
)

$process = Start-Process -FilePath $installer -ArgumentList $arguments -Wait -NoNewWindow -PassThru

if ($process.ExitCode -ne 0)
{
	Write-Host "The Vulkan SDK installer exited with $($process.ExitCode)."
	exit $process.ExitCode
}

Add-Content -Path $env:GITHUB_ENV -Value "VULKAN_SDK=$root"
Add-Content -Path $env:GITHUB_PATH -Value (Join-Path $root 'Bin')
