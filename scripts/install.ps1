[CmdletBinding()]
param(
    [switch]$Force,
    [string[]]$Only
)

$ErrorActionPreference = 'Stop'

$RootDir = Split-Path -Parent $PSScriptRoot
$VendorDir = Join-Path $RootDir 'vendor'
$LockDir = Join-Path $VendorDir '.locks'
$Manifest = Join-Path $PSScriptRoot 'dependencies.txt'

function Invoke-Git {
    param([string]$WorkDir, [string[]]$Arguments)
    & git -C $WorkDir @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') failed in $WorkDir"
    }
}

function Get-Dependencies {
    Get-Content $Manifest |
        Where-Object { $_.Trim() -and -not $_.TrimStart().StartsWith('#') } |
        ForEach-Object {
            $fields = $_.Trim() -split '\s+'
            [pscustomobject]@{
                Name       = $fields[0]
                Url        = $fields[1]
                Ref        = $fields[2]
                Submodules = $fields[3] -eq 'yes'
            }
        }
}

function Install-Dependency {
    param($Dependency)

    $target = Join-Path $VendorDir $Dependency.Name
    $lockFile = Join-Path $LockDir $Dependency.Name
    $lockValue = "$($Dependency.Url) $($Dependency.Ref)"

    if (-not $Force -and (Test-Path $target) -and (Test-Path $lockFile) -and ((Get-Content $lockFile -Raw).Trim() -eq $lockValue)) {
        Write-Host "[skip] $($Dependency.Name) @ $($Dependency.Ref)"
        return
    }

    Write-Host "[get ] $($Dependency.Name) @ $($Dependency.Ref)"
    if (Test-Path $target) {
        Remove-Item -Recurse -Force $target
    }
    New-Item -ItemType Directory -Force $target | Out-Null

    Invoke-Git $target @('init', '--quiet')
    Invoke-Git $target @('remote', 'add', 'origin', $Dependency.Url)
    Invoke-Git $target @('fetch', '--quiet', '--depth', '1', 'origin', $Dependency.Ref)
    Invoke-Git $target @('-c', 'advice.detachedHead=false', 'checkout', '--quiet', 'FETCH_HEAD')
    if ($Dependency.Submodules) {
        Invoke-Git $target @('submodule', 'update', '--init', '--recursive', '--depth', '1', '--quiet')
    }

    Set-Content -Path $lockFile -Value $lockValue -Encoding ascii
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'git is required but was not found on PATH.'
}

New-Item -ItemType Directory -Force $LockDir | Out-Null

$dependencies = Get-Dependencies
if ($Only) {
    $dependencies = $dependencies | Where-Object { $Only -contains $_.Name }
}

foreach ($dependency in $dependencies) {
    Install-Dependency $dependency
}

Write-Host "Dependencies are ready in $VendorDir"
