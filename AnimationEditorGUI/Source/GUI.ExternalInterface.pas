unit GUI.ExternalInterface;

interface

uses
  Winapi.Windows,
  Vcl.Forms,
  Utils.FPUState,
  GUI.MainForm;

type
  TButtonCallback = procedure (const Name: PWideChar); stdcall;
  TIdleCallback = procedure; stdcall;

procedure aegInitialize; stdcall;
procedure aegSetIdleCallback(Callback: TIdleCallback); stdcall;
procedure aegSetOpenGLWindow(Window: HWND); stdcall;
procedure aegSetButtonCallback(const Name: PWideChar; Callback: TButtonCallback); stdcall;
procedure aegRun; stdcall;

implementation

exports
  aegInitialize,
  aegSetIdleCallback,
  aegSetOpenGLWindow,
  aegSetButtonCallback,
  aegRun;

type
  Dummy = class abstract
    class var
      ExternalIdleCallback: TIdleCallback;
      FPUState: TFPUState;
    class procedure IdleCallback(Sender: TObject; var Done: Boolean);
  end;

procedure aegInitialize;
begin
  Application.Initialize;
  Application.CreateForm(TAnimationEditorForm, AnimationEditorForm);
  Application.OnIdle:= Dummy.IdleCallback;
end;

procedure aegSetIdleCallback(Callback: TIdleCallback);
begin
  Dummy.ExternalIdleCallback:= Callback;
end;

procedure aegSetOpenGLWindow(Window: HWND);
begin
  AnimationEditorForm.SetOpenGLWindow(Window);
end;

procedure aegSetButtonCallback(const Name: PWideChar; Callback: TButtonCallback); stdcall;
begin
  // TODO
end;

procedure aegRun; stdcall;
begin
  Application.Run;
end;

{ Dummy }

class procedure Dummy.IdleCallback(Sender: TObject; var Done: Boolean);
begin
  if Assigned(ExternalIdleCallback) then
  begin
    ExternalIdleCallback();
    Done:= False;
  end;
end;

end.
