param(
    [string]$RootDir = (Split-Path -Parent $PSScriptRoot),
    [string]$BuildDir = (Join-Path $RootDir 'build'),
    [string]$PackageDir = (Join-Path $RootDir 'dist\LinePuppyLLK'),
    [string]$QtBinDir = 'D:\Qt_SoftwareDevelopmentTools\6.11.0\mingw_64\bin'
)

$ErrorActionPreference = 'Stop'

$exePath = Join-Path $BuildDir 'appLinePuppyLLK.exe'
$windeployqtPath = Join-Path $QtBinDir 'windeployqt.exe'
$qtTranslationsDir = Join-Path (Split-Path $QtBinDir -Parent) 'translations'

if(!(Test-Path $windeployqtPath)){
    throw "找不到 windeployqt：$windeployqtPath"
}

if(!(Test-Path $exePath)){
    Write-Host '未找到现成可执行文件，开始构建 Release 版本...'
    & cmake --build $BuildDir --config Release
}

if(!(Test-Path $exePath)){
    throw "未找到可执行文件：$exePath"
}

if(Test-Path $PackageDir){
    Remove-Item -Recurse -Force $PackageDir
}
New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null

Copy-Item $exePath $PackageDir

& $windeployqtPath --release --compiler-runtime --dir $PackageDir $exePath

$guideCandidates = @(
    (Join-Path $RootDir 'GameGuide.txt'),
    (Join-Path $RootDir '游戏指导书.txt')
)
$guideFile = $guideCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if($guideFile){
    Copy-Item $guideFile $PackageDir -Force
}

$packageTranslationsDir = Join-Path $PackageDir 'translations'
foreach($translationFile in @('qtbase_zh_CN.qm', 'qt_zh_CN.qm')){
    $sourceTranslation = Join-Path $qtTranslationsDir $translationFile
    if(Test-Path $sourceTranslation){
        Copy-Item $sourceTranslation $packageTranslationsDir -Force
    }
}

Write-Host ''
Write-Host '发布包已生成：'
Write-Host $PackageDir
Write-Host '朋友直接双击包内的 appLinePuppyLLK.exe 即可运行。'