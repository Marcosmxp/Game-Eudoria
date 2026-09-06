@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul
color 0A
title Eudoria - One Click Setup / Build / Run

rem ================================================================
rem EUDORIA ONE CLICK
rem - Clones/updates the development branch
rem - Detects/installs Git, CMake and 7-Zip when possible
rem - Detects Visual Studio C++ Build Tools
rem - Finds Crystal Saga.rar, txt.rar and img.rar
rem - Validates each archive by its real payload contents
rem - Auto-detects txt.rar/img.rar beside Crystal Saga.rar when possible
rem - Extracts the real legacy HUD payload
rem - Extracts Role/Character display-list assets from the HUD payload
rem - Extracts/normalizes the real txt task catalog for TaskTracer
rem - Extracts one existing minimap from img.rar for UI preview
rem - Builds Eudoria x64 Release
rem - Starts Eudoria.exe from the repository root
rem ================================================================

set "REPO_URL=https://github.com/Marcosmxp/Game-Eudoria.git"
set "BRANCH=development"
set "SCRIPT_DIR=%~dp0"
set "REPO_DIR="
set "GIT_EXE="
set "CMAKE_EXE="
set "SEVENZIP_EXE="
set "VS_PATH="
set "CRYSTAL_ARCHIVE="
set "TXT_ARCHIVE="
set "IMG_ARCHIVE="
set "PAYLOAD_DIR="

call :Banner
call :ResolveTools || goto :Fail
call :ResolveRepo || goto :Fail
call :ResolveArchives || goto :Fail
call :ExtractAssets || goto :Fail
call :BuildGame || goto :Fail
call :RunGame || goto :Fail

echo.
echo ================================================================
echo Eudoria foi compilado e iniciado.
echo F11 = tela cheia
echo F2  = referencia visual de comparacao
echo ESC = sair
echo ================================================================
echo.
exit /b 0

:Banner
cls
echo ================================================================
echo                    E U D O R I A
echo             ONE CLICK - DEVELOPMENT BUILD
echo ================================================================
echo.
exit /b 0

:ResolveTools
echo [1/6] Verificando ferramentas...

call :FindGit
if not defined GIT_EXE (
    call :WingetInstall "Git.Git" "Git"
    call :FindGit
)
if not defined GIT_EXE (
    echo [ERRO] Git nao foi encontrado.
    exit /b 1
)
echo       Git: !GIT_EXE!

call :FindSevenZip
if not defined SEVENZIP_EXE (
    call :WingetInstall "7zip.7zip" "7-Zip"
    call :FindSevenZip
)
if not defined SEVENZIP_EXE (
    echo [ERRO] 7-Zip nao foi encontrado.
    exit /b 1
)
echo       7-Zip: !SEVENZIP_EXE!

call :FindVisualStudio
if not defined VS_PATH (
    echo.
    echo Visual Studio C++ Build Tools nao foi encontrado.
    echo O instalador pode ser grande e pode pedir permissao de administrador.
    call :WingetInstallVS
    call :FindVisualStudio
)
if not defined VS_PATH (
    echo [ERRO] Visual Studio com C++ Build Tools nao foi encontrado.
    echo Instale o workload "Desktop development with C++" e execute este BAT novamente.
    exit /b 1
)
echo       Visual Studio: !VS_PATH!

call :FindCMake
if not defined CMAKE_EXE (
    call :WingetInstall "Kitware.CMake" "CMake"
    call :FindCMake
)
if not defined CMAKE_EXE (
    echo [ERRO] CMake nao foi encontrado.
    exit /b 1
)
echo       CMake: !CMAKE_EXE!
echo.
exit /b 0

:FindGit
set "GIT_EXE="
for /f "delims=" %%I in ('where git.exe 2^>nul') do if not defined GIT_EXE set "GIT_EXE=%%I"
if not defined GIT_EXE if exist "%ProgramFiles%\Git\cmd\git.exe" set "GIT_EXE=%ProgramFiles%\Git\cmd\git.exe"
if not defined GIT_EXE if exist "%LocalAppData%\Programs\Git\cmd\git.exe" set "GIT_EXE=%LocalAppData%\Programs\Git\cmd\git.exe"
exit /b 0

:FindSevenZip
set "SEVENZIP_EXE="
for /f "delims=" %%I in ('where 7z.exe 2^>nul') do if not defined SEVENZIP_EXE set "SEVENZIP_EXE=%%I"
if not defined SEVENZIP_EXE if exist "%ProgramFiles%\7-Zip\7z.exe" set "SEVENZIP_EXE=%ProgramFiles%\7-Zip\7z.exe"
if not defined SEVENZIP_EXE if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" set "SEVENZIP_EXE=%ProgramFiles(x86)%\7-Zip\7z.exe"
exit /b 0

:FindVisualStudio
set "VS_PATH="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%I"
)
exit /b 0

:FindCMake
set "CMAKE_EXE="
for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
if not defined CMAKE_EXE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if defined VS_PATH if exist "!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
exit /b 0

:WingetInstall
set "PACKAGE_ID=%~1"
set "PACKAGE_NAME=%~2"
where winget.exe >nul 2>&1
if errorlevel 1 (
    echo [ERRO] !PACKAGE_NAME! esta ausente e winget nao esta disponivel para instalar automaticamente.
    exit /b 0
)
echo       Instalando !PACKAGE_NAME!...
winget install --id "!PACKAGE_ID!" -e --source winget --accept-package-agreements --accept-source-agreements
exit /b 0

:WingetInstallVS
where winget.exe >nul 2>&1
if errorlevel 1 exit /b 0
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget --accept-package-agreements --accept-source-agreements --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
exit /b 0

:ResolveRepo
echo [2/6] Preparando repositorio...

for %%I in ("%SCRIPT_DIR%Game-Eudoria") do set "REPO_DIR=%%~fI"

if exist "%SCRIPT_DIR%.git\HEAD" (
    if exist "%SCRIPT_DIR%CMakeLists.txt" (
        for %%I in ("%SCRIPT_DIR%.") do set "REPO_DIR=%%~fI"
    )
)

if not defined REPO_DIR (
    echo [ERRO] Nao foi possivel determinar a pasta do projeto.
    exit /b 1
)

echo       Destino do projeto: !REPO_DIR!

if not exist "!REPO_DIR!\.git\HEAD" (
    echo       Clonando !BRANCH!...
    "!GIT_EXE!" clone --branch "!BRANCH!" --single-branch "!REPO_URL!" "!REPO_DIR!"
    if errorlevel 1 exit /b 1
) else (
    echo       Repositorio encontrado. Atualizando !BRANCH!...
    pushd "!REPO_DIR!" >nul
    "!GIT_EXE!" fetch origin "!BRANCH!"
    if errorlevel 1 (popd >nul & exit /b 1)
    "!GIT_EXE!" checkout "!BRANCH!"
    if errorlevel 1 (popd >nul & exit /b 1)
    "!GIT_EXE!" pull --ff-only origin "!BRANCH!"
    if errorlevel 1 (popd >nul & exit /b 1)
    popd >nul
)
echo       Repo: !REPO_DIR!
echo.
exit /b 0

:ResolveArchives
echo [3/6] Localizando payloads...

call :AutoFindArchive "Crystal Saga.rar" CRYSTAL_ARCHIVE
if defined CRYSTAL_ARCHIVE call :ValidateArchive "!CRYSTAL_ARCHIVE!" "scripts/_assets/assets.swf" VALID_ARCHIVE
if not defined VALID_ARCHIVE set "CRYSTAL_ARCHIVE="

:PickCrystal
if not defined CRYSTAL_ARCHIVE (
    echo       Selecione CRYSTAL SAGA.RAR ^(HUD/SWF^)...
    call :PickFile "CRYSTAL SAGA.RAR - HUD e SWF" "Crystal Saga.rar" CRYSTAL_ARCHIVE
    if not defined CRYSTAL_ARCHIVE (
        echo [ERRO] Crystal Saga.rar e obrigatorio para reconstruir a HUD real.
        exit /b 1
    )
    call :ValidateArchive "!CRYSTAL_ARCHIVE!" "scripts/_assets/assets.swf" VALID_ARCHIVE
    if not defined VALID_ARCHIVE (
        echo       [ARQUIVO ERRADO] Esse RAR nao contem scripts/_assets/assets.swf.
        echo       Escolha o arquivo Crystal Saga.rar correto.
        set "CRYSTAL_ARCHIVE="
        goto :PickCrystal
    )
)
echo       HUD payload: !CRYSTAL_ARCHIVE!

rem txt.rar and img.rar are normally beside Crystal Saga.rar in the decoded
rem payload directory. Prefer those exact sibling files so they cannot be
rem accidentally swapped in the file picker.
for %%I in ("!CRYSTAL_ARCHIVE!") do set "PAYLOAD_DIR=%%~dpI"
if exist "!PAYLOAD_DIR!txt.rar" set "TXT_ARCHIVE=!PAYLOAD_DIR!txt.rar"
if exist "!PAYLOAD_DIR!img.rar" set "IMG_ARCHIVE=!PAYLOAD_DIR!img.rar"

if not defined TXT_ARCHIVE call :AutoFindArchive "txt.rar" TXT_ARCHIVE
if defined TXT_ARCHIVE call :ValidateArchive "!TXT_ARCHIVE!" "txt/itl.json" VALID_ARCHIVE
if not defined VALID_ARCHIVE set "TXT_ARCHIVE="

:PickTxt
if not defined TXT_ARCHIVE (
    echo       Selecione TXT.RAR ^(dados/configuracoes/quests^)...
    call :PickFile "TXT.RAR - dados reais, quests e configuracoes" "txt.rar" TXT_ARCHIVE
    if defined TXT_ARCHIVE (
        call :ValidateArchive "!TXT_ARCHIVE!" "txt/itl.json" VALID_ARCHIVE
        if not defined VALID_ARCHIVE (
            echo       [ARQUIVO ERRADO] Esse RAR nao contem txt/itl.json.
            echo       Nao selecione img.rar aqui. Escolha txt.rar.
            set "TXT_ARCHIVE="
            goto :PickTxt
        )
    )
)
if defined TXT_ARCHIVE (
    echo       Data payload: !TXT_ARCHIVE!
) else (
    echo       [AVISO] txt.rar nao selecionado. TaskTracer ficara sem catalogo real nesta execucao.
)

if not defined IMG_ARCHIVE call :AutoFindArchive "img.rar" IMG_ARCHIVE
if defined IMG_ARCHIVE call :ValidateArchive "!IMG_ARCHIVE!" "img/p*.jpg" VALID_ARCHIVE
if not defined VALID_ARCHIVE set "IMG_ARCHIVE="

:PickImg
if not defined IMG_ARCHIVE (
    echo       Selecione IMG.RAR ^(minimapas/imagens^). Cancelar apenas pula esta etapa...
    call :PickFile "IMG.RAR - minimapas e imagens" "img.rar" IMG_ARCHIVE
    if defined IMG_ARCHIVE (
        call :ValidateArchive "!IMG_ARCHIVE!" "img/p*.jpg" VALID_ARCHIVE
        if not defined VALID_ARCHIVE (
            echo       [ARQUIVO ERRADO] Esse RAR nao contem minimapas img/p*.jpg.
            echo       Nao selecione txt.rar aqui. Escolha img.rar.
            set "IMG_ARCHIVE="
            goto :PickImg
        )
    )
)
if defined IMG_ARCHIVE (
    echo       Minimap payload: !IMG_ARCHIVE!
) else (
    echo       img.rar nao selecionado. O jogo sera compilado sem o minimapa atual.
)
echo.
exit /b 0

:AutoFindArchive
set "TARGET_NAME=%~1"
set "%~2="
for %%D in ("%USERPROFILE%\Downloads" "%USERPROFILE%\Desktop" "%USERPROFILE%\Documents" "%SCRIPT_DIR%") do (
    if exist "%%~D\!TARGET_NAME!" set "%~2=%%~D\!TARGET_NAME!"
)
exit /b 0

:ValidateArchive
set "ARCHIVE_CHECK=%~1"
set "ARCHIVE_ENTRY=%~2"
set "%~3="
if not exist "!ARCHIVE_CHECK!" exit /b 0
"!SEVENZIP_EXE!" l -ba "!ARCHIVE_CHECK!" "!ARCHIVE_ENTRY!" 2>nul | findstr /R /C:"[^ ]" >nul
if not errorlevel 1 set "%~3=1"
exit /b 0

:PickFile
set "PICK_TITLE=%~1"
set "PICK_DEFAULT=%~2"
set "PICK_RESULT="
for /f "usebackq delims=" %%I in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Add-Type -AssemblyName System.Windows.Forms; $d=New-Object System.Windows.Forms.OpenFileDialog; $d.Title='%PICK_TITLE%'; $d.Filter='RAR archives (*.rar)|*.rar|All files (*.*)|*.*'; $d.FileName='%PICK_DEFAULT%'; if($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){$d.FileName}"`) do set "PICK_RESULT=%%I"
set "%~3=!PICK_RESULT!"
exit /b 0

:ExtractAssets
echo [4/6] Extraindo payloads reais...
pushd "!REPO_DIR!" >nul

powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\extract_legacy_hud.ps1" -Archive "!CRYSTAL_ARCHIVE!" -SevenZip "!SEVENZIP_EXE!"
if errorlevel 1 (popd >nul & exit /b 1)

echo       Extraindo UI Character do payload real...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\extract_role_character.ps1" -Archive "!CRYSTAL_ARCHIVE!" -SevenZip "!SEVENZIP_EXE!"
if errorlevel 1 (popd >nul & exit /b 1)

if defined TXT_ARCHIVE (
    echo       Extraindo catalogo real de quests/configuracoes de txt.rar...
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\extract_legacy_data.ps1" -Archive "!TXT_ARCHIVE!" -SevenZip "!SEVENZIP_EXE!"
    if errorlevel 1 (popd >nul & exit /b 1)
)

if defined IMG_ARCHIVE (
    echo       Extraindo um minimapa existente de img.rar para preview da UI...
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\extract_minimap.ps1" -Archive "!IMG_ARCHIVE!" -SevenZip "!SEVENZIP_EXE!"
    if errorlevel 1 (
        echo [AVISO] Nenhum minimapa de preview foi extraido. O restante continuara normalmente.
    )
)

popd >nul
echo.
exit /b 0

:BuildGame
echo [5/6] Compilando Eudoria x64 Release...
pushd "!REPO_DIR!" >nul
"!CMAKE_EXE!" -S . -B build -A x64
if errorlevel 1 (popd >nul & exit /b 1)
"!CMAKE_EXE!" --build build --config Release --parallel
if errorlevel 1 (popd >nul & exit /b 1)
popd >nul

if not exist "!REPO_DIR!\build\Release\Eudoria.exe" (
    echo [ERRO] Build terminou sem gerar build\Release\Eudoria.exe.
    exit /b 1
)
echo.
exit /b 0

:RunGame
echo [6/6] Iniciando Eudoria...
pushd "!REPO_DIR!" >nul
start "Eudoria" ".\build\Release\Eudoria.exe"
popd >nul
exit /b 0

:Fail
echo.
echo ================================================================
echo O processo foi interrompido por um erro.
echo Leia a mensagem acima. Nada do seu payload original foi alterado.
echo ================================================================
echo.
pause
exit /b 1