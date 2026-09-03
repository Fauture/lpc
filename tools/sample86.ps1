param([int]$pid2, [int]$samples = 5000, [int]$interval = 1)
$ErrorActionPreference = 'Stop'
$base = Split-Path -Parent $MyInvocation.MyCommand.Path
$code = Get-Content -Raw (Join-Path $base 'EipLoop86.cs')
Add-Type -TypeDefinition $code -Language CSharp
[Sampler86Loop]::Run($pid2, $samples, $interval)
