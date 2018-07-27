library AnimationEditorGUI;

uses
  madExcept,
  madLinkDisAsm,
  madListHardware,
  madListProcesses,
  madListModules,
  Winapi.Windows,
  GUI.ExternalInterface in 'GUI.ExternalInterface.pas',
  GUI.MainForm in 'GUI.MainForm.pas' {AnimationEditorForm};

{$R *.res}

procedure DllMain(Reason: Integer);
begin
  case Reason of
  DLL_PROCESS_ATTACH:
  begin

  end;
  DLL_PROCESS_DETACH:
  begin

  end;
  end;
end;

begin
  DllProc:= DllMain;
  DllProc(DLL_PROCESS_ATTACH);
end.
