unit GUI.ExternalInterface;

interface

uses
  Winapi.Windows,
  Vcl.Forms,
  GUI.Timeline;

type
  TButtonCallback = procedure (const Name: PWideChar); stdcall;
  TCheckBoxCallback = procedure (const Name: PWideChar; IsChecked: Boolean); stdcall;
  TEditCallback = procedure (const Name, Text: PWideChar); stdcall;
  TTrackBarCallback = procedure (const Name: PWideChar; t: Single); stdcall;
  TTimelineCallback = procedure (Position: Single; SelectedID: Integer; Items: PTimelineItem; ItemsCount: Integer); stdcall;
  TIdleCallback = procedure; stdcall;

procedure aegInitialize; stdcall;
procedure aegSetOpenGLWindow(Window: HWND); stdcall;

procedure aegSetIdleCallback(Callback: TIdleCallback); stdcall;
procedure aegSetButtonCallback(Callback: TButtonCallback); stdcall;
procedure aegSetCheckBoxCallback(Callback: TCheckBoxCallback); stdcall;
procedure aegSetEditCallback(Callback: TEditCallback); stdcall;
procedure aegSetTrackBarCallback(Callback: TTrackBarCallback); stdcall;
procedure aegSetTimelineCallback(Callback: TTimelineCallback); stdcall;

procedure aegSetEnabled(const Name: PWideChar; IsEnabled: Boolean); stdcall;
procedure aegSetChecked(const Name: PWideChar; IsChecked: Boolean); stdcall;
procedure aegSetText(const Name, Text: PWideChar); stdcall;
procedure aegSetPosition(const Name: PWideChar; t: Single); stdcall;
procedure aegSetTimelineState(Position: Single; SelectedID: Integer; Items: PTimelineItem; ItemsCount: Integer); stdcall;

procedure aegRun; stdcall;
procedure aegFinalize; stdcall;

implementation

uses
  GUI.MainForm;

exports
  aegInitialize,
  aegSetOpenGLWindow,

  aegSetIdleCallback,
  aegSetButtonCallback,
  aegSetCheckBoxCallback,
  aegSetEditCallback,
  aegSetTrackBarCallback,
  aegSetTimelineCallback,

  aegSetEnabled,
  aegSetChecked,
  aegSetText,
  aegSetPosition,
  aegSetTimelineState,

  aegRun,
  aegFinalize;

type
  Dummy = class abstract
    class var
      ExternalIdleCallback: TIdleCallback;
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
  AnimationEditorForm.ButtonCallback:= Callback;
end;

procedure aegSetCheckBoxCallback(Callback: TCheckBoxCallback);
begin
  AnimationEditorForm.CheckBoxCallback:= Callback;
end;

procedure aegSetEditCallback(Callback: TEditCallback);
begin
  AnimationEditorForm.EditCallback:= Callback;
end;

procedure aegSetTrackBarCallback(Callback: TTrackBarCallback);
begin
  AnimationEditorForm.TrackBarCallback:= Callback;
end;

procedure aegSetTimelineCallback(Callback: TTimelineCallback);
begin
  AnimationEditorForm.TimelineCallback:= Callback;
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

procedure aegSetPosition(const Name: PWideChar; t: Single);
begin
  AnimationEditorForm.SetPosition(Name, t);
end;

procedure aegSetTimelineState(Position: Single; SelectedID: Integer; Items: PTimelineItem; ItemsCount: Integer);
var
  ItemsArray: TArray<TTimelineItem>;
begin
  SetLength(ItemsArray, ItemsCount);
  if ItemsCount > 0 then
    Move(Items^, ItemsArray[0], Length(ItemsArray) * SizeOf(ItemsArray[0]));

  AnimationEditorForm.SetTimelineState(Position, SelectedID, ItemsArray);
end;

procedure aegRun;
begin
  Application.Run;
end;

procedure aegFinalize;
begin
  //
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
