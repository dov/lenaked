Name "${NAME}"
OutFile "Install${CAMELNAME}-${VER}-${HOST}-${COMMITIDSHORT}.exe"
;Icon "${NAME}-logo.ico"
;UninstallIcon "${NAME}-logo-install.ico"

SetCompress force ; (can be off or force)
CRCCheck on ; (can be off)

LicenseText "${CAMELNAME} is a free program. Here is its license."
LicenseData "COPYING.dos"

InstallDir "$PROGRAMFILES${HOSTBITS}\${CAMELNAME}"
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${CAMELNAME}" ""

DirText "Select the directory to install ${CAMELNAME} in:"

; optional section
Section "Start Menu Shortcuts"
 CreateDirectory "$SMPROGRAMS\${CAMELNAME}"
 CreateShortCut "$SMPROGRAMS\${CAMELNAME}\Uninstall.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\uninst.exe" 0
 CreateShortCut "$SMPROGRAMS\${CAMELNAME}\${CAMELNAME}.lnk" "$INSTDIR\bin\${CAMELNAME}.exe" "" "$INSTDIR\bin\${CAMELNAME}.exe" 0
SectionEnd

Section "" ; (default section)

; List of files to install
SetOutPath "$INSTDIR"
File COPYING
File README.md
File AUTHORS
File NEWS
File lenaked.css

; databases
File ..\nikud-db.utf8
File ..\bialik.db


SetOutPath $INSTDIR\bin
File ${NAME}.exe
File ${SYSROOT}\mingw\bin\${LIBGCCDLL}
File ${SYSROOT}\mingw\bin\libexpat-1.dll
File ${SYSROOT}\mingw\bin\libstdc++-6.dll
File ${SYSROOT}\mingw\bin\iconv.dll
File ${SYSROOT}\mingw\bin\libpcre-1.dll
File ${SYSROOT}\mingw\bin\libintl-8.dll
File ${SYSROOT}\mingw\bin\libffi-6.dll
File ${SYSROOT}\mingw\bin\libgdk_pixbuf*.dll
File ${SYSROOT}\mingw\bin\libgtk-3-0.dll
File ${SYSROOT}\mingw\bin\libgdk-3-0.dll
File ${SYSROOT}\mingw\bin\libgio*.dll
File ${SYSROOT}\mingw\bin\libepoxy-0.dll
File ${SYSROOT}\mingw\bin\libcairo*.dll
File ${SYSROOT}\mingw\bin\libjasper*.dll
File ${SYSROOT}\mingw\bin\zlib*.dll
File ${SYSROOT}\mingw\bin\libglib*.dll
File ${SYSROOT}\mingw\bin\libatk*.dll
File ${SYSROOT}\mingw\bin\libgobject*.dll
File ${SYSROOT}\mingw\bin\libgmodule*.dll
File ${SYSROOT}\mingw\bin\libgthread*.dll
File ${SYSROOT}\mingw\bin\libpango*.dll
File ${SYSROOT}\mingw\bin\libpng*.dll
File ${SYSROOT}\mingw\bin\libpixman-1-0.dll
File ${SYSROOT}\mingw\bin\libfontconfig*.dll
File ${SYSROOT}\mingw\bin\libfreetype*.dll
File ${SYSROOT}\mingw\bin\libbz2*.dll
File ${SYSROOT}\mingw\bin\libwinpthread*.dll

# Not sure that I need this. It doesn't seem to make any difference.
SetOutPath $INSTDIR\etc\settings
FileOpen $9 settings.ini w ;Opens a Empty File an fills it
FileWrite $9 "[Settings]$\r$\n"
FileWrite $9 "gtk-theme-name=MS-Windows$\r$\n"
FileClose $9 ;Closes the filled file

SetOutPath $INSTDIR\lib\gdk-pixbuf-2.0\2.10.0
File /r ${SYSROOT}\mingw\lib\gdk-pixbuf-2.0\2.10.0\loaders
SetOutPath $INSTDIR\share\themes 
File /r ${SYSROOT}\mingw\share\themes\*
SetOutPath $INSTDIR\share\icons\Adwaita\16x16
File /r ${SYSROOT}\mingw\share\icons\Adwaita\16x16\actions
SetOutPath $INSTDIR\share\icons\Adwaita\22x22
File /r ${SYSROOT}\mingw\share\icons\Adwaita\22x22\actions
SetOutPath $INSTDIR\share\icons\Adwaita\48x48
File /r ${SYSROOT}\mingw\share\icons\Adwaita\48x48\actions
SetOutPath $INSTDIR\share\icons\Adwaita\64x64
File /r ${SYSROOT}\mingw\share\icons\Adwaita\64x64\actions

# Build the gdk-pixbuf.loaders file automatically
#ExpandEnvStrings $0 %COMSPEC%
#nsExec::ExecToStack '"$0" /C ""$INSTDIR\bin\gdk-pixbuf-query-loaders" > "$INSTDIR\lib\gdk-pixbuf-2.0\2.10.0\loaders.cache""'

System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'

; Unistaller section. Should really clean up file associations as well.
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\${NAME}" "" "$INSTDIR"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "DisplayName" "${NAME} (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}" "UninstallString" '"$INSTDIR\bin\uninst.exe"'

; write out uninstaller
WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd ; end of default section

; begin uninstall settings/section
UninstallText "This will uninstall ${NAME} from your system"

Section Uninstall
; add delete commands to delete whatever files/registry keys/etc you installed here.
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${NAME}"
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"
RMDir /r "$INSTDIR"
DeleteRegKey HKCR ".${NAME}"
DeleteRegKey HKCR "Applications\${NAME}.exe"
SectionEnd ; end of uninstall section

; eof
