unit GUI.Timeline;

interface

uses
  Winapi.Windows, Winapi.Messages,
  System.Types, System.SysUtils, System.Variants, System.Classes, System.Math, System.UITypes,
  Vcl.Graphics, Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls;

type
  PTimelineItem = ^TTimelineItem;
  TTimelineItem = record
    ID: Integer;
    Position: Single;
    constructor Create(ID: Integer; Position: Single);
  end;

  TItemChangeEvent = procedure (Sender: TObject; Item: TTimelineItem) of object;

  TTimeline = class(TFrame)
    Scene: TImage;
    procedure FrameResize(Sender: TObject);
    procedure SceneMouseDown(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure SceneMouseMove(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
    procedure SceneMouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure SceneDblClick(Sender: TObject);
  private
    const
      INDEX_NONE = -1;
      INDEX_CURSOR = -2;

      SelectionDistance = 5; // in pixels

      Padding = 0.25;

    var
      CurrentItems: TArray<TTimelineItem>;
      CursorPosition, TimelineLength: Single;
      ActiveIndex, SelectedIndex: Integer;

      LastMouseX, LockCounter: Integer;
      IsRedrawScheduled: Boolean;

    procedure Redraw;

    function GetScene : TCanvas;

    function PositionToX(Position: Single) : Integer;
    function XToPosition(X: Integer) : Single;

    function GetIndexByX(X: Integer) : Integer;
    procedure SetActivePositionFromX(X: Integer; const Sender: String);
    function IsSelected(const Item: TTimelineItem) : Boolean;
    function FindNewIndex(const OldItems: TArray<TTimelineItem>; OldIndex: Integer) : Integer;
    function FindIndexByID(ID: Integer) : Integer;

    function GetItems : TArray<TTimelineItem>;
    procedure SetItems(const NewItems: TArray<TTimelineItem>);

    procedure SetLength(LengthInSeconds: Single);

    function GetSelectedID: Integer;
    procedure SetSelectedID(NewSelectedID: Integer);

    procedure SetCursorPosition(Position: Single);
  public
    OnCursorChange, OnChange: TNotifyEvent;
    OnItemChange, OnItemSelected: TItemChangeEvent;

    constructor Create(AOwner: TComponent); override;

    property Position : Single read CursorPosition write SetCursorPosition;
    property Length : Single write SetLength;
    property Items : TArray<TTimelineItem> read GetItems write SetItems;
    property SelectedID : Integer read GetSelectedID write SetSelectedID;

    procedure Lock;
    procedure Unlock;
  end;

implementation

{$R *.dfm}

{ TTimeline }

constructor TTimeline.Create(AOwner: TComponent);
begin
  TimelineLength:= 5;
  ActiveIndex:= INDEX_NONE;
  SelectedIndex:= INDEX_NONE;

  inherited;

  FrameResize(AOwner);
end;

procedure TTimeline.FrameResize(Sender: TObject);
var
  Bitmap: TBitmap;
begin
  Bitmap:= TBitmap.Create;
  Bitmap.SetSize(Scene.Width, Scene.Height);

  Scene.Picture.Bitmap:= Bitmap;

  Redraw;
end;

procedure TTimeline.SceneMouseDown(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  if X = LastMouseX then
    Exit;

  ActiveIndex:= GetIndexByX(X);

  if ActiveIndex < 0 then
    SetActivePositionFromX(X, 'Down');

  LastMouseX:= X;
end;

procedure TTimeline.SceneMouseMove(Sender: TObject; Shift: TShiftState; X,
  Y: Integer);
var
  Point: TPoint;
begin
  if X = LastMouseX then
    Exit;

  Point:= TPoint.Create(X, Y);

  SetActivePositionFromX(X, 'Move');

  LastMouseX:= X;
end;

procedure TTimeline.SceneMouseUp(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
begin
  if X = LastMouseX then
    Exit;

  if ActiveIndex < 0 then
    SetActivePositionFromX(X, 'Up');

  ActiveIndex:= INDEX_NONE;

  LastMouseX:= X;
end;

procedure TTimeline.SceneDblClick(Sender: TObject);
var
  Index: Integer;
begin
  Index:= GetIndexByX(LastMouseX);
  if Index >= 0 then
  begin
    SelectedIndex:= Index;

    Redraw;

    if Assigned(OnItemSelected) then
      OnItemSelected(Self, CurrentItems[SelectedIndex]);

    if Assigned(OnChange) then
      OnChange(Self);
  end;
end;

procedure TTimeline.SetLength(LengthInSeconds: Single);
begin
  TimelineLength:= LengthInSeconds;
end;

function TTimeline.GetScene: TCanvas;
begin
  Result:= Self.Scene.Picture.Bitmap.Canvas;
end;

function TTimeline.GetSelectedID: Integer;
begin
  if SelectedIndex >= 0 then
    Result:= CurrentItems[SelectedIndex].ID
  else
    Result:= 0;
end;

procedure TTimeline.SetSelectedID(NewSelectedID: Integer);
begin
  SelectedIndex:= FindIndexByID(NewSelectedID);
  Redraw;
end;

function TTimeline.IsSelected(const Item: TTimelineItem): Boolean;
begin
  Result:= (SelectedIndex >= 0) and (Item.ID = CurrentItems[SelectedIndex].ID);
end;

function TTimeline.FindNewIndex(const OldItems: TArray<TTimelineItem>; OldIndex: Integer): Integer;
var
  OldID: Integer;
begin
  if OldIndex >= 0 then
  begin
    OldID:= OldItems[OldIndex].ID;

    Result:= FindIndexByID(OldID);
  end
  else
    Result:= OldIndex;
end;

function TTimeline.FindIndexByID(ID: Integer): Integer;
var
  Index: Integer;
begin
  Result:= INDEX_NONE;

  for Index := 0 to System.Length(CurrentItems) - 1 do
    if CurrentItems[Index].ID = ID then
      Exit(Index);
end;

function TTimeline.PositionToX(Position: Single): Integer;
var
  Scene: TCanvas;
  Rect: TRect;
begin
  Scene:= GetScene;
  Rect:= Scene.ClipRect;

  Result:= Round((Position + Padding / 2) / (TimelineLength + Padding) * (Rect.Width - 1));
end;

function TTimeline.XToPosition(X: Integer): Single;
var
  Scene: TCanvas;
  Rect: TRect;
begin
  Scene:= GetScene;
  Rect:= Scene.ClipRect;

  Result:= X / (Rect.Width - 1) * (TimelineLength + Padding) - Padding / 2;
end;

function TTimeline.GetIndexByX(X: Integer): Integer;
var
  NewActiveIndex, Index, ItemX, MinDist, Dist: Integer;
begin
  MinDist:= Round(SelectionDistance * 1.75) + 1;
  NewActiveIndex:= INDEX_CURSOR;
  for Index := 0 to System.Length(CurrentItems) - 1 do
  begin
    ItemX:= PositionToX(CurrentItems[Index].Position);

    Dist:= Abs(ItemX - X);
    if Dist < MinDist then
    begin
      NewActiveIndex:= Index;
      MinDist:= Dist;
    end;
  end;

  Result:= NewActiveIndex;
end;

procedure TTimeline.SetActivePositionFromX(X: Integer; const Sender: String);
var
  ActivePosition: PSingle;
begin
  if ActiveIndex = INDEX_CURSOR then
    ActivePosition:= @CursorPosition
  else
  if ActiveIndex <> INDEX_NONE then
    ActivePosition:= @CurrentItems[ActiveIndex].Position
  else
    Exit;

  SelectedIndex:= INDEX_NONE;

  ActivePosition^:= EnsureRange(XToPosition(X), 0, TimelineLength);

  if ActiveIndex = INDEX_CURSOR then
  begin
    if Assigned(OnCursorChange) then
      OnCursorChange(Self);

    if Assigned(OnChange) then
      OnChange(Self);
  end
  else
  if ActiveIndex >= 0 then
  begin
    if Assigned(OnItemChange) then
      OnItemChange(Self, CurrentItems[ActiveIndex]);

    if Assigned(OnChange) then
      OnChange(Self);
  end;

  Redraw;
end;

procedure TTimeline.SetCursorPosition(Position: Single);
begin
  CursorPosition:= Position;
  Redraw;
end;

function TTimeline.GetItems: TArray<TTimelineItem>;
begin
  Result:= Copy(CurrentItems);
end;

procedure TTimeline.SetItems(const NewItems: TArray<TTimelineItem>);
var
  OldItems: TArray<TTimelineItem>;
begin
  OldItems:= CurrentItems;
  CurrentItems:= Copy(NewItems);

  ActiveIndex:= FindNewIndex(OldItems, ActiveIndex);
  SelectedIndex:= FindNewIndex(OldItems, SelectedIndex);

  Redraw;
end;

procedure TTimeline.Redraw;
var
  Scene: TCanvas;
  Rect: TRect;

  procedure DrawTextAt(X, Y: Integer; Text: String; TextSize: Integer);
  var
    TextRect: TRect;
  begin
    TextRect:= TRect.Empty;
    Scene.TextRect(TextRect, Text, [tfCalcRect]);
    Inc(TextRect.Right);
    Inc(TextRect.Bottom);

    X:= EnsureRange(X - TextRect.Width div 2, 0, Rect.Width - 1 - TextRect.Width);
    TextRect.SetLocation(X, Y);

    Scene.Font.Size:= TextSize;
    Scene.TextOut(TextRect.Left, TextRect.Top, Text);
  end;

const
  Len = 15;
  DelimiterCount = 10;
  MiddleDelimeter = DelimiterCount div 2;

  VerticalPadding = 0.35;
var
  Index, DelimeterNum, X, Y: Integer;
  Position, Mul: Single;
  Second: Integer;
  Item: TTimelineItem;
  ItemRect: TRect;
begin
  if LockCounter > 0 then
  begin
    IsRedrawScheduled:= True;
    Exit
  end;

  IsRedrawScheduled:= False;

  Scene:= GetScene;

  Rect:= Scene.ClipRect;

  Scene.Brush.Color:= Self.Color;
  Scene.FillRect(Rect);

  Scene.Pen.Width:= 1;

  for Index := 0 to Trunc(TimelineLength * DelimiterCount) do
  begin
    DelimeterNum:= Index mod DelimiterCount;

    Position:= Index / DelimiterCount;
    Second:= Trunc(Position);

    Mul:= IfThen(DelimeterNum = 0, 1, IfThen(DelimeterNum = MiddleDelimeter, 0.6, 0.36));

    X:= PositionToX(Position);
    Y:= Trunc(Len * Mul);

    Scene.Pen.Color:= clGray;
    Scene.MoveTo(X, 0);
    Scene.LineTo(X, Rect.Height - 1);

    Scene.Pen.Color:= clBlack;
    Scene.MoveTo(X, 0);
    Scene.LineTo(X, Y);

    case DelimeterNum of
    0:               DrawTextAt(X, Y, Format('%ds', [Second]), 8);
    MiddleDelimeter: DrawTextAt(X, Y, Format('%d.5s', [Second]), 7);
    end;
  end;

  for Item in CurrentItems do
  begin
    if IsSelected(Item) then
      Scene.Brush.Color:= $FF7ABE
    else
      Scene.Brush.Color:= $6B1A18;

    Scene.Pen.Color:= Scene.Brush.Color;

    X:= PositionToX(Item.Position);
    ItemRect:= TRect.Create(TPoint.Zero, SelectionDistance * 2, Round(Rect.Height * (1 - VerticalPadding * 2)));
    ItemRect.Offset(X - SelectionDistance, Round(Rect.Height * VerticalPadding));
    Scene.Rectangle(ItemRect);
  end;

  X:= PositionToX(CursorPosition);
  Scene.Pen.Color:= $FFC55D;
  Scene.Pen.Width:= 1;
  Scene.MoveTo(X, Round((Rect.Height - 1) * (0 + VerticalPadding)));
  Scene.LineTo(X, Round((Rect.Height - 1) * (1 - VerticalPadding)));
end;

procedure TTimeline.Lock;
begin
  Inc(LockCounter);
end;

procedure TTimeline.Unlock;
begin
  Dec(LockCounter);
  if LockCounter = 0 then
  begin
    if IsRedrawScheduled then
      Redraw;
  end
  else
    Assert(LockCounter > 0);
end;

{ TTimelineItem }

constructor TTimelineItem.Create(ID: Integer; Position: Single);
begin
  Self.ID:= ID;
  Self.Position:= Position;
end;

end.
