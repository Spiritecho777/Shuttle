param(
	[Parameter(Mandatory=$true)]
	[ValidateSet("Generate", "Translate")]
	[string]$Target
)

$Path = "$PSScriptRoot/Asset"
echo $Path
function Generates {
	if (Test-Path "$Path/translations"){
		Remove-Item "$Path/translations" -Recurse -Force
	}

	mkdir "$Path/translations"

	$PathTranslate = "$PSScriptRoot/Asset/translations"

	lupdate . -ts "$PathTranslate/shuttle_fr.ts" "$PathTranslate/shuttle_en.ts" "$PathTranslate/shuttle_ja.ts" "$PathTranslate/shuttle_bz.ts"
}

function Translates {
	if (-not (Test-Path "$Path/translations")){
		Write-host "Aucun fichier de traductions trouver veuillez d'abord faire Generate"
		return
	}
	
	$PathTranslate = "$PSScriptRoot/Asset/translations"

	lrelease "$PathTranslate/shuttle_fr.ts" "$PathTranslate/shuttle_en.ts" "$PathTranslate/shuttle_ja.ts" "$PathTranslate/shuttle_bz.ts"
}

switch ($Target) {
	"Generate" { Generates }
	"Translate" { Translates }
}