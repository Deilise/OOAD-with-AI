# Render SSDs, SDs, class diagram(s), and domain diagram.
$OoadRoot = Split-Path $PSScriptRoot -Parent
$WorkspaceRoot = Split-Path $OoadRoot -Parent
$LocalJar = Join-Path $OoadRoot "tools\plantuml\plantuml.jar"
$WorkspaceJar = Join-Path $WorkspaceRoot "tools\plantuml\plantuml.jar"
$Jar = if (Test-Path $LocalJar) { $LocalJar } else { $WorkspaceJar }
$SsdDir = Join-Path $PSScriptRoot "ssd"
$SdDir = Join-Path $PSScriptRoot "sd"
$ClassDir = Join-Path $PSScriptRoot "class"
$DomainDir = Join-Path $PSScriptRoot "domain"
$Sources = @(
  Get-ChildItem $SsdDir -Filter "UC??_system_sequence.puml" | ForEach-Object { $_.FullName }
  Get-ChildItem $SdDir -Filter "UC??_sequence.puml" | ForEach-Object { $_.FullName }
  Get-ChildItem $ClassDir -Filter "*.puml" | ForEach-Object { $_.FullName }
  (Join-Path $DomainDir "RVC_domain.puml")
)
if (-not (Test-Path $Jar)) {
  throw "PlantUML jar not found at $LocalJar or $WorkspaceJar"
}
& java -jar $Jar -charset UTF-8 -tpng $Sources
Write-Host "Rendered: $Sources"
