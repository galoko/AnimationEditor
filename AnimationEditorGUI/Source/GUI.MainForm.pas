unit GUI.MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls;

type
  TAnimationEditorForm = class(TForm)
    LowerControlPanel: TPanel;
    OpenGLPanel: TPanel;
    TestButton: TButton;
    UpperControlPanel: TPanel;
    Button2: TButton;
  private
    { Private declarations }
  public
    { Public declarations }
    procedure SetOpenGLWindow(Window: HWND);
  end;

var
  AnimationEditorForm: TAnimationEditorForm;

implementation

{$R *.dfm}

procedure TAnimationEditorForm.SetOpenGLWindow(Window: HWND);
begin
  Winapi.Windows.SetParent(Window, OpenGLPanel.Handle);
  SetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE or SWP_NOZORDER);
  ShowWindow(Window, SW_SHOWNA);
end;

end.
