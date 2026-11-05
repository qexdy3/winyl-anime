; Build with Inno Setup 5.6.1 (the last version supporting Windows XP).
; Pass /DStageDir=<absolute path to the prepared runtime files> to ISCC.
#ifndef StageDir
  #error StageDir must point to the prepared Winyl Anime runtime folder
#endif

[Setup]
AppId={{32A035C0-2DE7-4B98-9DAC-F0B82E102471}
AppName=Winyl Anime
AppVersion=3.3.1
AppPublisher=Winyl Anime community fork
AppPublisherURL=https://github.com/qexdy3/winyl-anime
DefaultDirName={localappdata}\Winyl Anime
DefaultGroupName=Winyl Anime
PrivilegesRequired=lowest
MinVersion=0,5.1
Compression=lzma
SolidCompression=yes
DisableWelcomePage=no
AllowNoIcons=yes
UninstallDisplayIcon={app}\Winyl.exe
OutputBaseFilename=WinylAnime_Setup_x86
#ifdef TestBuild
Uninstallable=no
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#StageDir}\Winyl.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\Bass\*"; DestDir: "{app}\Bass"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\Language\*"; DestDir: "{app}\Language"; Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\Equalizer\*"; DestDir: "{app}\Equalizer"; Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\Skin\*"; DestDir: "{app}\Skin"; Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\Portable.dat"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\Settings.xml"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\WinylAnime.install"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\License.txt"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#StageDir}\ThirdParty.txt"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{group}\Winyl Anime"; Filename: "{app}\Winyl.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\Winyl Anime"; Filename: "{app}\Winyl.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\Winyl.exe"; Description: "{cm:LaunchProgram,Winyl Anime}"; Flags: nowait postinstall skipifsilent

[Code]
function IsOtherWinyl: Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\Winyl.exe')) and
    not FileExists(ExpandConstant('{app}\WinylAnime.install'));
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpSelectDir) and IsOtherWinyl then begin
    MsgBox('This folder contains another Winyl installation. Choose a separate empty folder to protect its files and settings.', mbError, MB_OK);
    Result := False;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if IsOtherWinyl then
    Result := 'This folder contains another Winyl installation. Choose a separate folder.';
end;
