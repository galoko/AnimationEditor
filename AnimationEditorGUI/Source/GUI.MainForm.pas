unit GUI.MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages,
  System.SysUtils, System.Variants, System.Classes,
  Vcl.Graphics, Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls,
  Vcl.AppEvnts, Vcl.ComCtrls, Vcl.ExtDlgs,
  GUI.ExternalInterface, GUI.Timeline;

type
  TAnimationEditorForm = class(TForm)
    LowerControlPanel: TPanel;
    OpenGLPanel: TPanel;
    UpperControlPanel: TPanel;
    ApplicationEvents: TApplicationEvents;
    ResetRootPosition: TButton;
    XPos: TCheckBox;
    YPos: TCheckBox;
    ZPos: TCheckBox;
    XAxis: TCheckBox;
    YAxis: TCheckBox;
    ZAxis: TCheckBox;
    XPosInput: TEdit;
    YPosInput: TEdit;
    ZPosInput: TEdit;
    XAngleInput: TEdit;
    YAngleInput: TEdit;
    ZAngleInput: TEdit;
    XAxisBar: TTrackBar;
    YAxisBar: TTrackBar;
    ZAxisBar: TTrackBar;
    OpenFile: TButton;
    NewFile: TButton;
    OpenDialog: TOpenTextFileDialog;
    NewDialog: TSaveTextFileDialog;
    TimelinePlaceholder: TPanel;
    CreateState: TButton;
    DeleteState: TButton;
    MirrorState: TButton;
    UndoDelete: TButton;
    AnimationLength: TEdit;
    PlayStop: TButton;
    AnimationLoop: TCheckBox;
    procedure ApplicationEventsMessage(var Msg: tagMSG; var Handled: Boolean);
    procedure FormCreate(Sender: TObject);
    procedure ButtonClick(Sender: TObject);
    procedure CheckBoxClick(Sender: TObject);
    procedure EditKeyDown(Sender: TObject; var Key: Word; Shift: TShiftState);
    procedure EditKeyPress(Sender: TObject; var Key: Char);
    procedure TrackBarChange(Sender: TObject);
    procedure OpenFileClick(Sender: TObject);
    procedure NewFileClick(Sender: TObject);
    procedure TimelineChange(Sender: TObject);
  private
    { Private declarations }
    IsInSetMode: Boolean;

    OpenGLWindow: HWND;

    Timeline: TTimeline;

    procedure SetupCallbacks(Component: TComponent);
  public
    { Public declarations }
    ButtonCallback: TButtonCallback;
    CheckBoxCallback: TCheckBoxCallback;
    EditCallback: TEditCallback;
    TrackBarCallback: TTrackBarCallback;
    TimelineCallback: TTimelineCallback;

    procedure SetOpenGLWindow(Window: HWND);

    procedure SetEnabled(const Name: String; IsEnabled: Boolean); reintroduce;
    procedure SetChecked(const Name: String; IsChecked: Boolean);
    procedure SetText(const Name, Text: String);
    procedure SetPosition(const Name: String; t: Single);
    procedure SetTimelineState(Position, Length: Single; SelectedID: Integer; const Items: TArray<TTimelineItem>);
  end;

var
  AnimationEditorForm: TAnimationEditorForm;

implementation

{$R *.dfm}

procedure TAnimationEditorForm.FormCreate(Sender: TObject);
begin
  Timeline:= TTimeline.Create(Self);
  Timeline.OnChange:= TimelineChange;
  Timeline.Parent:= TimelinePlaceholder;

  SetupCallbacks(Self);
end;

procedure TAnimationEditorForm.SetupCallbacks(Component: TComponent);
var
  SubComponent: TComponent;
begin
  if Component is TButton then
  begin
    if not Assigned(TButton(Component).OnClick) then
      TButton(Component).OnClick:= ButtonClick;
  end
  else
  if Component is TCheckBox then
    TCheckBox(Component).OnClick:= CheckBoxClick
  else
  if Component is TEdit then
  begin
    TEdit(Component).OnKeyDown:= EditKeyDown;
    TEdit(Component).OnKeyPress:= EditKeyPress;
  end
  else
  if Component is TTrackBar then
    TTrackBar(Component).OnChange:= TrackBarChange;

  for SubComponent in Component do
    SetupCallbacks(SubComponent);
end;

procedure TAnimationEditorForm.OpenFileClick(Sender: TObject);
begin
  if OpenDialog.Execute and Assigned(EditCallback) then
    EditCallback(PWideChar(OpenDialog.Name), PWideChar(OpenDialog.FileName));
end;

procedure TAnimationEditorForm.NewFileClick(Sender: TObject);
begin
  if NewDialog.Execute and Assigned(EditCallback) then
    EditCallback(PWideChar(NewDialog.Name), PWideChar(NewDialog.FileName));
end;

procedure TAnimationEditorForm.ButtonClick(Sender: TObject);
var
  Button: TButton;
begin
  if not IsInSetMode and Assigned(ButtonCallback) then
  begin
    Button:= Sender as TButton;
    ButtonCallback(PWideChar(Button.Name));
  end;
end;

procedure TAnimationEditorForm.CheckBoxClick(Sender: TObject);
var
  CheckBox: TCheckBox;
begin
  if not IsInSetMode and Assigned(CheckBoxCallback) then
  begin
    CheckBox:= Sender as TCheckBox;
    CheckBoxCallback(PWideChar(CheckBox.Name), CheckBox.Checked);
  end;
end;

procedure TAnimationEditorForm.EditKeyDown(Sender: TObject; var Key: Word; Shift: TShiftState);
var
  Edit: TEdit;
begin
  Edit:= Sender as TEdit;

  if Key = VK_RETURN then
  begin
    Key:= 0;

    if Assigned(EditCallback) then
      EditCallback(PWideChar(Edit.Name), PWideChar(Edit.Text));

    Edit.Parent.SetFocus;
  end;
end;

procedure TAnimationEditorForm.EditKeyPress(Sender: TObject; var Key: Char);
begin
  if Ord(Key) = VK_RETURN then
    Key:= #0;
end;

procedure TAnimationEditorForm.TrackBarChange(Sender: TObject);
var
  TrackBar: TTrackBar;
  t: Single;
begin
  if not IsInSetMode and Assigned(TrackBarCallback) then
  begin
    TrackBar:= Sender as TTrackBar;

    t:= (TrackBar.Position - TrackBar.Min) / (TrackBar.Max - TrackBar.Min);

    TrackBarCallback(PWideChar(TrackBar.Name), t);
  end;
end;

procedure TAnimationEditorForm.TimelineChange(Sender: TObject);
var
  Items: TArray<TTimelineItem>;
begin
  if Assigned(TimelineCallback) then
  begin
    Items:= Timeline.Items;
    TimelineCallback(Timeline.Position, Timeline.Length, Timeline.SelectedID, PTimelineItem(Items), Length(Items));
  end;
end;

procedure TAnimationEditorForm.ApplicationEventsMessage(var Msg: tagMSG; var Handled: Boolean);
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

procedure TAnimationEditorForm.SetEnabled(const Name: String;
  IsEnabled: Boolean);
var
  Component: TComponent;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TWinControl then
  begin
    IsInSetMode:= True;
    TWinControl(Component).Enabled:= IsEnabled;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetChecked(const Name: String;
  IsChecked: Boolean);
var
  Component: TComponent;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TCheckBox then
  begin
    IsInSetMode:= True;
    TCheckBox(Component).Checked:= IsChecked;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetText(const Name, Text: String);
var
  Component: TComponent;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TEdit then
  begin
    if TEdit(Component).Focused then
      Exit;

    IsInSetMode:= True;
    TEdit(Component).Text:= Text;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetPosition(const Name: String; t: Single);
var
  Component: TComponent;
  TrackBar: TTrackBar;
  Position: Integer;
begin
  Component:= FindComponent(Name);
  if Component = nil then
    Exit;

  if Component is TTrackBar then
  begin
    TrackBar:= TTrackBar(Component);
    (*
    if TrackBar.Focused then
      Exit;
    *)

    Position:= TrackBar.Min + Round((TrackBar.Max - TrackBar.Min) * t);

    IsInSetMode:= True;
    TrackBar.Position:= Position;
    IsInSetMode:= False;
  end;
end;

procedure TAnimationEditorForm.SetTimelineState(Position, Length: Single; SelectedID: Integer; const Items: TArray<TTimelineItem>);
begin
  Timeline.Lock;
  try
    Timeline.Items:= Items;
    Timeline.Length:= Length;
    Timeline.Position:= Position;
    Timeline.SelectedID:= SelectedID;
  finally
    Timeline.Unlock;
  end;
end;

end.
