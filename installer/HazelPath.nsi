Unicode True

!include "LogicLib.nsh"
!include "WordFunc.nsh"

!define APP_NAME "HazelPath"
; Read the same release version used in the application window title.
!searchparse /file "..\src\AppVersion.h" '#define HAZELPATH_VERSION "' APP_VERSION '"'
!define APP_PUBLISHER "HazelPath"
!define APP_EXE "HazelPath.exe"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

Name "${APP_NAME} ${APP_VERSION}"
!define /date APP_BUILD_DATE "%Y-%m-%d"
OutFile "..\dist\HazelPath-Setup-${APP_VERSION}-${APP_BUILD_DATE}-x64.exe"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "Software\${APP_NAME}" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "${APP_PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright ${APP_PUBLISHER}"
VIAddVersionKey "FileDescription" "${APP_NAME} x64 Kurulum"
VIAddVersionKey "FileVersion" "${APP_VERSION}"
VIAddVersionKey "ProductVersion" "${APP_VERSION}"

Function .onInit
  ReadRegStr $0 HKLM "${UNINSTALL_KEY}" "DisplayVersion"
  ${If} $0 != ""
    ${VersionCompare} "$0" "${APP_VERSION}" $1
    ${If} $1 == 1
      MessageBox MB_ICONSTOP|MB_OK "Yuklu HazelPath surumu ($0), bu kurulumdan (${APP_VERSION}) daha yeni.$\r$\nKurulum iptal edildi."
      Abort
    ${ElseIf} $1 == 0
      MessageBox MB_ICONQUESTION|MB_YESNO "HazelPath ${APP_VERSION} zaten kurulu.$\r$\nYeniden kurmak istiyor musunuz?" IDYES +2
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "HazelPath" SEC_MAIN
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File "..\bin\HazelPath.exe"
  File "runtime\msvcp140.dll"
  File "runtime\vcruntime140.dll"
  File "runtime\vcruntime140_1.dll"

  SetOutPath "$INSTDIR\data"
  File /r "..\bin\data\*.*"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\${APP_NAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\${APP_EXE}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" 1

  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\HazelPath Kaldir.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$DESKTOP\${APP_NAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APP_NAME}"
  DeleteRegKey HKLM "${UNINSTALL_KEY}"
  DeleteRegKey HKLM "Software\${APP_NAME}"
  RMDir /r "$INSTDIR\data"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\${APP_EXE}"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
SectionEnd
