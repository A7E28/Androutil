$existingPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::User)
$newPath = "C:\platform-tools"

if (-not $existingPath.Contains($newPath)) {
    $updatedPath = "$existingPath;$newPath"
    [System.Environment]::SetEnvironmentVariable("Path", $updatedPath, [System.EnvironmentVariableTarget]::User)
    "Path successfully appended."
} else {
    "Path already contains the new entry."
}