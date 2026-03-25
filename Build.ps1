param(
	[Parameter(Mandatory=$true)]
	[ValidateSet("all", "all_secure", "all_web", "all_web_secure")]
	[string]$Target,

	[Parameter(Mandatory=$true)]
	[string]$ProjectName
)

if (Test-Path "Deploy"){
	Remove-Item "Deploy" -Recurse -Force
}

mkdir "Deploy"

function Get-CMakeVersion {
    $cmakeFile = "CMakeLists.txt"

    if (!(Test-Path $cmakeFile)) {
        Write-Host "CMakeLists.txt introuvable, version par défaut 1.0.0"
        return "1.0.0"
    }

    $line = Select-String -Path $cmakeFile -Pattern 'set\(APP_VERSION\s+"([0-9\.]+)"\)' | Select-Object -First 1

    if ($line) {
        $match = [regex]::Match($line.Line, '"([0-9\.]+)"')
        if ($match.Success) {
            return $match.Groups[1].Value.Trim()
        }
    }

    Write-Host "Version non trouvée dans CMakeLists.txt, version par défaut 1.0.0"
    return "1.0.0"
}

function Build-Windows {
	Write-Host "=== Compilation Windows ==="

	Remove-Item "build" -Recurse -Force

	cmake -B build -S . -G "Visual Studio 17 2022" -T v143 -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/Static/msvc

	cmake --build build --config Release

	mkdir -p Deploy/Windows
	cp build/Release/*.exe Deploy/Windows

	Write-Host "=== Exécutable généré ==="
}

function Build-Windows-OpenSSL {
	Write-Host "=== Compilation Windows avec WebEngine ==="

	Remove-Item "build" -Recurse -Force

	cmake -B build -S . -G "Visual Studio 17 2022" -T v143 -DCMAKE_PREFIX_PATH=C:/Qt/6.10.2/Static/msvc -DOPENSSL_ROOT_DIR=C:/dev/vcpkg/installed/x64-windows-static -DOPENSSL_USE_STATIC_LIBS=ON

	cmake --build build --config Release

	mkdir -p Deploy/Windows
	cp build/Release/*.exe Deploy/Windows

	Write-Host "=== Exécutable généré ==="
}

function Build-Windows-Web {
	Write-Host "=== Compilation Windows avec WebEngine ==="

	Remove-Item "build" -Recurse -Force

	cmake -B build -S . -G "Visual Studio 17 2022" -T v143 `
	-DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64" `
	-DQt6WebEngineWidgets_DIR="C:/Qt/6.10.2/msvc2022_64/lib/cmake/Qt6WebEngineWidgets"

	cmake --build build --config Release

	mkdir -p Deploy/Windows
	cp build/Release/*.exe Deploy/Windows

	$env:Path = "C:/Qt/6.10.2/msvc2022_64/bin;" + $env:PATH
	windeployqt --release --no-translations "Deploy/Windows/$ProjectName.exe"

	Write-Host "=== Exécutable généré ==="
}

function Build-Windows-Web-OpenSSL {
	Write-Host "=== Compilation Windows avec WebEngine ==="

	Remove-Item "build" -Recurse -Force

	cmake -B build -S . -G "Visual Studio 17 2022" -T v143 `
	-DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64" `
	-DOPENSSL_ROOT_DIR=C:/dev/vcpkg/installed/x64-windows-static `
	-DOPENSSL_USE_STATIC_LIBS=ON

	cmake --build build --config Release

	mkdir -p Deploy/Windows
	cp build/Release/*.exe Deploy/Windows

	$env:Path = "C:/Qt/6.10.2/msvc2022_64/bin;" + $env:PATH
	windeployqt --release --no-translations "Deploy/Windows/$ProjectName.exe"

	Write-Host "=== Exécutable généré ==="
}

function Build-InnoSetup {
	Write-Host "=== Génération de l'installeur Inno Setup ==="

    $InnoPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    $ScriptPath = "$(PWD)\Installer\setup.iss"

    if (!(Test-Path $InnoPath)) {
        Write-Host "Erreur : ISCC.exe introuvable"
        exit 1
    }

    if (!(Test-Path $ScriptPath)) {
        Write-Host "Erreur : script Inno Setup introuvable : $ScriptPath"
        exit 1
    }

	$Version = Get-CMakeVersion

    & "$InnoPath" "/dProjectName=$ProjectName" "/dProjectVersion=$Version" "$ScriptPath"

    Write-Host "=== Installeur généré ==="
}

function Build-Linux {
	Write-Host "=== Compilation Linux ==="

		$clean = @"
rm -rf /home/echo/Projet/$ProjectName/build-linux
rm -rf /home/echo/Projet/$ProjectName/package
rm -rf /home/echo/Projet/$ProjectName/$ProjectName.tar.gz
"@
	wsl --distribution archlinux --user echo -- bash -lc "$clean"

	$cmd = @"
mkdir -p /home/echo/Projet/$ProjectName
cmake -B /home/echo/Projet/$ProjectName/build-linux -S . -DCMAKE_PREFIX_PATH=/home/echo/Qt/6.10.2/Static/gcc
cmake --build /home/echo/Projet/$ProjectName/build-linux

mkdir -p /home/echo/Projet/$ProjectName/package
cp /home/echo/Projet/$ProjectName/build-linux/$ProjectName /home/echo/Projet/$ProjectName/package/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/install.sh /home/echo/Projet/$ProjectName/package/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/Icone.png /home/echo/Projet/$ProjectName/package/

cd /home/echo/Projet/$ProjectName/package
tar -czf ../$ProjectName.tar.gz *
"@

	wsl --distribution archlinux --user echo -- bash -lc "$cmd"

	wsl --distribution archlinux --user echo -- bash -lc "cat /home/echo/Projet/$ProjectName/$ProjectName.tar.gz > Deploy/$ProjectName.tar.gz"
	
	Write-Host "=== Binaire généré ==="
}

function Build-Linux-OpenSSL {
	Write-Host "=== Compilation Linux ==="

		$clean = @"
rm -rf /home/echo/Projet/$ProjectName/build-linux
rm -rf /home/echo/Projet/$ProjectName/package
rm -rf /home/echo/Projet/$ProjectName/$ProjectName.tar.gz
"@
	wsl --distribution archlinux --user echo -- bash -lc "$clean"

	$cmd = @"
mkdir -p /home/echo/Projet/$ProjectName
cmake -B /home/echo/Projet/$ProjectName/build-linux -S . -DCMAKE_PREFIX_PATH=/home/echo/Qt/6.10.2/Static/gcc -DOPENSSL_ROOT_DIR=/usr
cmake --build /home/echo/Projet/$ProjectName/build-linux

mkdir -p /home/echo/Projet/$ProjectName/package
cp /home/echo/Projet/$ProjectName/build-linux/$ProjectName /home/echo/Projet/$ProjectName/package/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/install.sh /home/echo/Projet/$ProjectName/package/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/Icone.png /home/echo/Projet/$ProjectName/package/

cd /home/echo/Projet/$ProjectName/package
tar -czf ../$ProjectName.tar.gz *
"@

	wsl --distribution archlinux --user echo -- bash -lc "$cmd"

	wsl --distribution archlinux --user echo -- bash -lc "cat /home/echo/Projet/$ProjectName/$ProjectName.tar.gz > Deploy/$ProjectName.tar.gz"
	
	Write-Host "=== Binaire généré ==="
}

function Build-Linux-Web {
	Write-Host "=== Compilation Linux ==="

		$clean = @"
rm -rf /home/echo/Projet/$ProjectName/build-linux
rm -rf /home/echo/Projet/$ProjectName/package
rm -rf /home/echo/Projet/$ProjectName/$ProjectName.tar.gz
"@
	wsl --distribution archlinux --user echo -- bash -lc "$clean"

	$cmd = @"
mkdir -p /home/echo/Projet/$ProjectName
cmake -B /home/echo/Projet/$ProjectName/build-linux -S . -DCMAKE_PREFIX_PATH=/home/echo/Qt/6.10.2/Static/gcc -DQt6WebEngineWidgets_DIR=/usr/lib/cmake/Qt6WebEngineWidgets
cmake --build /home/echo/Projet/$ProjectName/build-linux

mkdir -p /home/echo/Projet/$ProjectName/package
cp /home/echo/Projet/$ProjectName/build-linux/$ProjectName /home/echo/Projet/$ProjectName/package/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/install.sh /home/echo/Projet/$ProjectName/package/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/Icone.png /home/echo/Projet/$ProjectName/package/

cd /home/echo/Projet/$ProjectName/package
tar -czf ../$ProjectName.tar.gz *
"@

	wsl --distribution archlinux --user echo -- bash -lc "$cmd"

	wsl --distribution archlinux --user echo -- bash -lc "cat /home/echo/Projet/$ProjectName/$ProjectName.tar.gz > Deploy/$ProjectName.tar.gz"
	
	Write-Host "=== Binaire généré ==="
}

function Build-Linux-Web-OpenSSL {
	Write-Host "=== Compilation Linux ==="

		$clean = @"
rm -rf /home/echo/Projet/$ProjectName/build-linux
rm -rf /home/echo/Projet/$ProjectName/AppDir
rm -rf /home/echo/Projet/$ProjectName/$ProjectName.AppImage
"@
	wsl --distribution AlmaLinux-9 --user echo -- bash -lc "$clean"

	$cmd = @"
mkdir -p /home/echo/Projet/$ProjectName
cmake -B /home/echo/Projet/$ProjectName/build-linux -S . -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=/usr -DOPENSSL_CRYPTO_LIBRARY=/usr/lib64/libcrypto.so -DOPENSSL_SSL_LIBRARY=/usr/lib64/libssl.so
cmake --build /home/echo/Projet/$ProjectName/build-linux

mkdir -p /home/echo/Projet/$ProjectName/AppDir/usr/bin
mkdir -p /home/echo/Projet/$ProjectName/AppDir/usr/share/applications
mkdir -p /home/echo/Projet/$ProjectName/AppDir/usr/share/icons/hicolor/256x256/apps

cp /home/echo/Projet/$ProjectName/build-linux/$ProjectName /home/echo/Projet/$ProjectName/AppDir/usr/bin/
cp /home/echo/Projet/$ProjectName/build-linux/Asset/Icone.png /home/echo/Projet/$ProjectName/AppDir/usr/share/icons/hicolor/256x256/apps/$ProjectName.png

cat > /home/echo/Projet/$ProjectName/AppDir/usr/share/applications/$ProjectName.desktop << EOF
[Desktop Entry]
Name=$ProjectName
Exec=$ProjectName
Icon=$ProjectName
Type=Application
Categories=Utility;
EOF

export QMAKE=/usr/bin/qmake6
export QML_SOURCES_PATHS=.
export NO_STRIP=1

cd /home/echo/Projet/$ProjectName

/home/echo/Tools/linuxdeploy/usr/bin/linuxdeploy --appdir AppDir --plugin qt --output appimage

mv $ProjectName-*.AppImage $ProjectName.AppImage
"@

	wsl --distribution AlmaLinux-9 --user echo -- bash -lc "$cmd"

	wsl --distribution AlmaLinux-9 --user echo -- bash -lc "cat /home/echo/Projet/$ProjectName/$ProjectName.AppImage > Deploy/$ProjectName.AppImage"
	
	Write-Host "=== Binaire généré ==="
}

switch ($Target) {
	"all" { Build-Windows; Build-InnoSetup; Build-Linux }
	"all_secure" { Build-Windows-OpenSSL; Build-InnoSetup; Build-Linux-OpenSSL }
	"all_web" { Build-Windows-Web; Build-InnoSetup; Build-Linux-Web }
	"all_web_secure" { Build-Windows-Web-OpenSSL; Build-InnoSetup; Build-Linux-Web-OpenSSL }
}