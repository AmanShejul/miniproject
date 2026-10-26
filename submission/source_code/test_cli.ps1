[CmdletBinding()]
param(
    [string]$ExecutablePath = (Join-Path $PSScriptRoot "campus_navigation.exe")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $ExecutablePath -PathType Leaf)) {
    throw "Application executable not found: $ExecutablePath"
}

$resolvedExecutable = (Resolve-Path -LiteralPath $ExecutablePath).Path

function Invoke-CliCase {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][AllowEmptyString()][string[]]$InputLines,
        [string[]]$MustContain = @(),
        [string[]]$MustNotContain = @(),
        [string[]]$MustMatch = @(),
        [int]$ExpectedMenuPrompts = -1
    )

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $script:resolvedExecutable
    $startInfo.WorkingDirectory = $PSScriptRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        throw "Could not start CLI case '$Name'."
    }

    # Drain output concurrently so a full pipe cannot block the child process.
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if ($InputLines.Count -gt 0) {
        $inputText = [string]::Join([Environment]::NewLine, $InputLines) +
            [Environment]::NewLine
        $process.StandardInput.Write($inputText)
        $process.StandardInput.Flush()
    }
    $process.StandardInput.Close()

    if (-not $process.WaitForExit(10000)) {
        $process.Kill()
        throw "CLI case '$Name' timed out."
    }

    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    if ($process.ExitCode -ne 0) {
        throw "CLI case '$Name' exited $($process.ExitCode). Output: $stdout $stderr"
    }
    foreach ($expected in $MustContain) {
        if (-not $stdout.Contains($expected)) {
            throw "CLI case '$Name' did not print expected text: $expected. Output: $stdout"
        }
    }
    foreach ($unexpected in $MustNotContain) {
        if ($stdout.Contains($unexpected)) {
            throw "CLI case '$Name' unexpectedly printed: $unexpected"
        }
    }
    foreach ($pattern in $MustMatch) {
        if (-not [regex]::IsMatch($stdout, $pattern)) {
            throw "CLI case '$Name' did not match expected pattern: $pattern"
        }
    }
    if ($ExpectedMenuPrompts -ge 0) {
        $promptCount = [regex]::Matches($stdout, "Choose an option:").Count
        if ($promptCount -ne $ExpectedMenuPrompts) {
            throw "CLI case '$Name' displayed $promptCount menu prompts; expected $ExpectedMenuPrompts."
        }
    }

    Write-Output "PASS $Name"
}

$longMenuLine = -join ("x" * 300)
Invoke-CliCase -Name "invalid and overlong menu input" `
    -InputLines @("", "abc", "12abc", "-2", "2147483648", $longMenuLine, "0", "11", "10") `
    -MustContain @("Invalid input; please enter a whole number.",
                   "Invalid input; please enter a whole number without extra text.",
                   "Input line is too long; please try again.",
                   "Invalid menu option. Choose a number from 1 to 10.",
                   "Goodbye.") `
    -ExpectedMenuPrompts 9

$name63 = [string]::new([char]65, 63)
$name64 = [string]::new([char]66, 64)
Invoke-CliCase -Name "63-character name and overlong name" `
    -InputLines @("1", "10", $name63, "1", "11", $name64, "2", "10") `
    -MustContain @("Added ", "Location name must be at most 63 characters.") `
    -MustMatch @("ID: 10 \| Name: A{63}") `
    -MustNotContain @("ID: 11 | Name:")

Invoke-CliCase -Name "duplicate rejection and graph rollback" `
    -InputLines @("1", "10", "Hall", "1", "10", "Other Hall",
                  "1", "73", "hAlL", "1", "73", "Library",
                  "4", "10", "73", "250", "5", "6", "10", "73", "10") `
    -MustContain @("That location ID is already in use.",
                   "That location name is already in use.",
                   "Added Library (ID 73).",
                   "Hall (ID 10) <-> Library (ID 73): 250 m",
                   "Shortest route: Hall (ID 10) -> Library (ID 73)",
                   "Total distance: 250 m")

Invoke-CliCase -Name "invalid paths and duplicate undirected edge" `
    -InputLines @("1", "10", "North", "1", "73", "Library",
                  "4", "10", "10", "20", "4", "10", "73", "0",
                  "4", "10", "73", "-1", "4", "10", "999", "10",
                  "4", "10", "73", "100", "4", "73", "10", "200",
                  "10") `
    -MustContain @("A location cannot have a path to itself.",
                   "Path distance must be a positive number of metres.",
                   "One or more location IDs are not in the graph.",
                   "A path already exists between those locations.")

Invoke-CliCase -Name "fictional sample loaded once and shortest route" `
    -InputLines @("9", "9", "2", "6", "10", "900", "7", "10", "900", "10") `
    -MustContain @("Loaded five fictional demo locations and seven fictional paths.",
                   "These labels and distances are development examples, not a real campus map.",
                   "Fictional sample data can only be loaded into an empty map.",
                   "Shortest route: Demo North Gate (ID 10) -> Demo Library (ID 73) -> Demo Cafeteria (ID 420) -> Demo Sports Centre (ID 900)",
                   "Total distance: 510 m",
                   "Destination Demo Sports Centre (ID 900) is reachable from Demo North Gate (ID 10).")

Invoke-CliCase -Name "BFS excludes disconnected location" `
    -InputLines @("1", "10", "North", "1", "20", "South",
                  "7", "10", "20", "8", "10") `
    -MustContain @("Reachable locations in BFS order (1):",
                   "Destination South (ID 20) is not reachable from North (ID 10).",
                   "The campus graph is not connected.") `
    -MustNotContain @("  South (ID 20)")

$eofCases = @(
    @{ Name = "EOF at menu"; Input = @() },
    @{ Name = "EOF while adding location ID"; Input = @("1") },
    @{ Name = "EOF while adding location name"; Input = @("1", "10") },
    @{ Name = "EOF during search type"; Input = @("3") },
    @{ Name = "EOF during ID search"; Input = @("3", "1") },
    @{ Name = "EOF during name search"; Input = @("3", "2") },
    @{ Name = "EOF during first path ID"; Input = @("4") },
    @{ Name = "EOF during second path ID"; Input = @("4", "10") },
    @{ Name = "EOF during path distance"; Input = @("4", "10", "20") },
    @{ Name = "EOF during Dijkstra source"; Input = @("6") },
    @{ Name = "EOF during Dijkstra destination"; Input = @("6", "10") },
    @{ Name = "EOF during BFS source"; Input = @("7") },
    @{ Name = "EOF during BFS destination"; Input = @("7", "10") }
)
foreach ($case in $eofCases) {
    [void](Invoke-CliCase -Name $case.Name -InputLines $case.Input `
        -MustContain @("Input ended; exiting safely."))
}

Write-Output "All CLI regression cases passed."
