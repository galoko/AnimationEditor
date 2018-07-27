unit GUI.ExternalInterface;

interface

uses
  Winapi.Windows,
  Vcl.Forms,
  Utils.FPUState;

type
  TButtonCallback = procedure (const Name: PWideChar); stdcall;
  TCheckBoxCallback = procedure (const Name: PWideChar; IsChecked: Boolean); stdcall;
  TEditCallback = procedure (const Name, Text: PWideChar); stdcall;
  TIdleCallback = procedure; stdcall;

procedure aegInitialize; stdcall;
procedure aegSetIdleCallback(Callback: TIdleCallback); stdcall;
procedure aegSetOpenGLWindow(Window: HWND); stdcall;
procedure aegSetButtonCallback(Callback: TButtonCallback); stdcall;
procedure aegSetCheckBoxCallback(Callback: TCheckBoxCallback); stdcall;
procedure aegSetEditCallback(Callback: TEditCallback); stdcall;
procedure aegSetEnabled(const Name: PWideChar; IsEnabled: Boolean); stdcall;
procedure aegSetChecked(const Name: PWideChar; IsChecked: Boolean); stdcall;
procedure aegSetText(const Name, Text: PWideChar); stdcall;
procedure aegRun; stdcall;

implementation

uses
  GUI.MainForm;

exports
  aegInitialize,
  aegSetIdleCallback,
  aegSetOpenGLWindow,
  aegSetButtonCallback,
  aegSetCheckBoxCallback,
  aegSetEditCallback,
  aegSetEnabled,
  aegSetChecked,
  aegSetText,
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

procedure aegSetButtonCallback(Callback: TButtonCallback);
begin
  AnimationEditorForm.SetButtonCallback(Callback);
end;

procedure aegSetCheckBoxCallback(Callback: TCheckBoxCallback);
begin
  AnimationEditorForm.SetCheckBoxCallback(Callback);
end;

procedure aegSetEditCallback(Callback: TEditCallback);
begin
  AnimationEditorForm.SetEditCallback(Callback);
end;

procedure aegSetEnabled(const Name: PWideChar; IsEnabled: Boolean);
begin
  AnimationEditorForm.SetEnabled(Name, IsEnabled);
end;

procedure aegSetChecked(const Name: PWideChar; IsChecked: Boolean);
begin
  AnimationEditorForm.SetChecked(Name, IsChecked);
end;

procedure aegSetText(const Name, Text: PWideChar);
begin
  AnimationEditorForm.SetText(Name, Text);
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
