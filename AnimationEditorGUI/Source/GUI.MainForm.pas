unit GUI.MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls, Vcl.AppEvnts;

type
  TAnimationEditorForm = class(TForm)
    LowerControlPanel: TPanel;
    OpenGLPanel: TPanel;
    TestButton: TButton;
    UpperControlPanel: TPanel;
    Button2: TButton;
    ApplicationEvents: TApplicationEvents;
    procedure ApplicationEventsMessage(var Msg: tagMSG; var Handled: Boolean);
  private
    { Private declarations }
    OpenGLWindow: HWND;
  public
    { Public declarations }
    procedure SetOpenGLWindow(Window: HWND);
  end;

var
  AnimationEditorForm: TAnimationEditorForm;

implementation

{$R *.dfm}

procedure TAnimationEditorForm.ApplicationEventsMessage(var Msg: tagMSG;
  var Handled: Boolean);
begin
  // keys redirect
  if Msg.hwnd <> OpenGLWindow then
    if (Msg.message = WM_KEYUP) or (Msg.message = WM_KEYDOWN) then
      PostMessage(OpenGLWindow, Msg.message, Msg.wParam, Msg.lParam);
end;

procedure TAnimationEditorForm.SetOpenGLWindow(Window: HWND);
begin
  OpenGLWindow:= Window;

  Winapi.Windows.SetParent(OpenGLWindow, OpenGLPanel.Handle);
  SetWindowPos(OpenGLWindow, 0, 0, 0, 0, 0, SWP_NOSIZE or SWP_NOZORDER);
  ShowWindow(OpenGLWindow, SW_SHOWNA);
end;

end.
