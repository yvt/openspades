#!/usr/bin/env pwsh
param(
    [string]$SourceDirectory = ".",
    [string]$ClangFormat = "clang-format"
)

# TODO: Run clang-format on C++ source files

# Run clang-format on AngelScript source files
$ScriptDirectory = Join-Path $SourceDirectory "Resources" "Scripts"
$Scripts = Get-ChildItem -Recurse -Include "*.as" $ScriptDirectory

$I = 0
foreach ($Item in $Scripts) {
    $Path = $Item.FullName
    $TmpPath = $Path.Substring(0, $Path.Length - 3) + ".java"

    # Make it pretend to be a Java source file (which clang-format understands)
    # I didn't choose C++ mainly due to the difference in how accessibility is
    # specified.
    Copy-Item $Path $TmpPath

    # Run clang-format
    &$ClangFormat -i -style=file $TmpPath

    # Rename it back
    Move-Item -Force $TmpPath $Path

    # Fix `@ this.`
    $Text = Get-Content $Path
    $Text = $Text.Replace("@ this.", "@this.")
    Set-Content $Path $Text

    $I += 1
    Write-Progress -Activity "Running clang-format on AngelScript source files" `
        -PercentComplete ($I / $Scripts.Count * 100)
}
