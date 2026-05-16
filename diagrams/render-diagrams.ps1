# Render SSDs, SDs, class diagram(s), and domain diagram.
$RepoRoot = Split-Path $PSScriptRoot -Parent
$Jar = Join-Path $RepoRoot "tools\plantuml\plantuml.jar"
$PlantUmlDir = Join-Path $PSScriptRoot "plantuml"
$SdDir = Join-Path $PSScriptRoot "sd"
$ClassDir = Join-Path $PSScriptRoot "class"
$Sources = @(
  Get-ChildItem $PlantUmlDir -Filter "UC??_system_sequence.puml" | ForEach-Object { $_.FullName }
  Get-ChildItem $SdDir -Filter "UC??_sequence.puml" | ForEach-Object { $_.FullName }
  Get-ChildItem $ClassDir -Filter "*.puml" | ForEach-Object { $_.FullName }
  (Join-Path $PlantUmlDir "RVC_domain.puml")
)
& java -jar $Jar -charset UTF-8 -tpng $Sources
Write-Host "Rendered: $Sources"
