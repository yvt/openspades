param([string]$OutputDirectory)

Add-Type -Assembly "System.IO.Compression.FileSystem"

if ($OutputDirectory -eq "") {
	$OutputDirectory = "."
}
if (-not (Test-Path $OutputDirectory)) {
	New-Item $OutputDirectory -Force -ItemType Directory | Out-Null
}

$SourceDirectory = Split-Path -Parent $PSCommandPath

function Make-Pak
{
	param( [String[]] $RelativePaths, [String] $PakName )

	Echo "Creating $($PakName)..."

	$OutputPak = Join-Path $OutputDirectory $PakName

	# Prepare a temporary directory
	$TempDir = "mkpak.tmp"
	if (Test-Path $TempDir) { Remove-Item $TempDir -Recurse -Force }
	New-Item $TempDir -ItemType Directory | Out-Null

	# TODO: exclude _Assets_ and .DS_Store

	# Copy target files to the temporary directory
	ForEach ($RelativePath in $RelativePaths) {
		# Create the parent directory if it doesn't exist
		$ParentDir = Split-Path -Parent (Join-Path $TempDir $RelativePath)
		if (-not (Test-Path $ParentDir)) {
			New-Item $ParentDir -Force -ItemType Directory | Out-Null
		}

		# Copy item
		Copy-Item (Join-Path $SourceDirectory $RelativePath) `
		  -Destination (Join-Path $TempDir $RelativePath) -Recurse
	}

	# Delete old file
	if (Test-Path $OutputPak) { Remove-Item $OutputPak -Force }

	# Create zip archive
	# PowerShell 5 supports Compress-Archive, but the method used here is
	# faster by a order of magnitude for some reason.
	# However, it's still much slower than "zip" command on Linux/macOS.
	[IO.Compression.ZipFile]::CreateFromDirectory($TempDir, $OutputPak,
	  [IO.Compression.CompressionLevel]::Fastest, $false)

	# Clean up
	Remove-Item $TempDir -Recurse -Force
}

Make-Pak -PakName pak002-Base.pak -RelativePaths `
  License/Credits-pak002-Base.md,
  Gfx, Scripts/Main.as,
  Scripts/Gui, Scripts/Base, Shaders, Sounds/Feedback,
  Sounds/Misc, Sounds/Player, Textures

Make-Pak -PakName pak005-Models.pak -RelativePaths `
  Maps, Models/MapObjects, Models/Player

Make-Pak -PakName pak010-BaseSkin.pak -RelativePaths `
  License/Credits-pak010-BaseSkin.md,
  Scripts/Skin, Sounds/Weapons, Models/Weapons, Models/MapObjects

Make-Pak -PakName pak050-Locales.pak -RelativePaths `
  License/Credits-pak050-Locales.md, Locales

Make-Pak -PakName pak999-References.pak -RelativePaths `
  License/Credits-pak999-References.md, Scripts/Reference
