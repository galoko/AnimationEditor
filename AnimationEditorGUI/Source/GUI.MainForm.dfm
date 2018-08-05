object AnimationEditorForm: TAnimationEditorForm
  Left = 0
  Top = 0
  BorderStyle = bsNone
  Caption = 'Animation Editor'
  ClientHeight = 960
  ClientWidth = 1280
  Color = clWhite
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poDesigned
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object LowerControlPanel: TPanel
    Left = 0
    Top = 840
    Width = 1280
    Height = 120
    Align = alBottom
    BevelOuter = bvNone
    Ctl3D = True
    ParentBackground = False
    ParentCtl3D = False
    TabOrder = 0
    object ResetRootPosition: TButton
      Left = 7
      Top = 90
      Width = 105
      Height = 25
      Caption = 'Reset Root Position'
      TabOrder = 0
    end
    object XAxis: TCheckBox
      Left = 352
      Top = 7
      Width = 50
      Height = 17
      Caption = 'X Axis'
      TabOrder = 4
    end
    object YAxis: TCheckBox
      Left = 352
      Top = 30
      Width = 50
      Height = 17
      Caption = 'Y Axis'
      TabOrder = 5
    end
    object ZAxis: TCheckBox
      Left = 352
      Top = 53
      Width = 50
      Height = 17
      Caption = 'Z Axis'
      TabOrder = 6
    end
    object XPos: TCheckBox
      Left = 215
      Top = 7
      Width = 66
      Height = 17
      Caption = 'X Position'
      TabOrder = 1
    end
    object YPos: TCheckBox
      Left = 215
      Top = 30
      Width = 66
      Height = 17
      Caption = 'Y Position'
      TabOrder = 2
    end
    object ZPos: TCheckBox
      Left = 215
      Top = 53
      Width = 66
      Height = 17
      Caption = 'Z Position'
      TabOrder = 3
    end
    object XPosInput: TEdit
      Left = 287
      Top = 3
      Width = 49
      Height = 21
      TabOrder = 7
    end
    object YPosInput: TEdit
      Left = 287
      Top = 30
      Width = 49
      Height = 21
      TabOrder = 8
    end
    object ZPosInput: TEdit
      Left = 287
      Top = 57
      Width = 49
      Height = 21
      TabOrder = 9
    end
    object YAngleInput: TEdit
      Left = 408
      Top = 30
      Width = 49
      Height = 21
      TabOrder = 10
    end
    object ZAngleInput: TEdit
      Left = 408
      Top = 57
      Width = 49
      Height = 21
      TabOrder = 11
    end
    object XAngleInput: TEdit
      Left = 408
      Top = 3
      Width = 49
      Height = 21
      TabOrder = 12
    end
    object XAxisBar: TTrackBar
      Left = 463
      Top = 3
      Width = 75
      Height = 21
      Max = 3600
      TabOrder = 13
    end
    object YAxisBar: TTrackBar
      Left = 463
      Top = 30
      Width = 75
      Height = 21
      Max = 3600
      TabOrder = 14
    end
    object ZAxisBar: TTrackBar
      Left = 463
      Top = 57
      Width = 75
      Height = 21
      Max = 3600
      TabOrder = 15
    end
    object OpenFile: TButton
      Left = 5
      Top = 3
      Width = 75
      Height = 25
      Caption = 'Open File'
      TabOrder = 16
      OnClick = OpenFileClick
    end
    object NewFile: TButton
      Left = 5
      Top = 29
      Width = 75
      Height = 25
      Caption = 'New File'
      TabOrder = 17
      OnClick = NewFileClick
    end
    object TimelinePlaceholder: TPanel
      Left = 544
      Top = 3
      Width = 721
      Height = 89
      BevelOuter = bvNone
      Color = clHighlight
      ParentBackground = False
      TabOrder = 18
    end
    object CreateState: TButton
      Left = 83
      Top = 3
      Width = 75
      Height = 25
      Caption = 'Create State'
      TabOrder = 19
    end
    object DeleteState: TButton
      Left = 83
      Top = 29
      Width = 75
      Height = 25
      Caption = 'Delete State'
      TabOrder = 20
    end
    object MirrorState: TButton
      Left = 118
      Top = 90
      Width = 75
      Height = 25
      Caption = 'Mirror State'
      TabOrder = 21
    end
    object UndoDelete: TButton
      Left = 83
      Top = 55
      Width = 75
      Height = 25
      Caption = 'Undo Delete'
      TabOrder = 22
    end
    object AnimationLength: TEdit
      Left = 777
      Top = 96
      Width = 33
      Height = 21
      TabOrder = 23
    end
    object PlayStop: TButton
      Left = 641
      Top = 94
      Width = 75
      Height = 25
      Caption = 'Play/Stop'
      TabOrder = 24
    end
    object AnimationLoop: TCheckBox
      Left = 592
      Top = 98
      Width = 41
      Height = 17
      Caption = 'Loop'
      TabOrder = 25
    end
  end
  object OpenGLPanel: TPanel
    Left = 0
    Top = 120
    Width = 1280
    Height = 720
    Align = alClient
    BevelOuter = bvNone
    Color = clWhite
    Ctl3D = True
    ParentBackground = False
    ParentCtl3D = False
    TabOrder = 1
  end
  object UpperControlPanel: TPanel
    Left = 0
    Top = 0
    Width = 1280
    Height = 120
    Align = alTop
    BevelOuter = bvNone
    Ctl3D = True
    ParentBackground = False
    ParentCtl3D = False
    TabOrder = 2
  end
  object ApplicationEvents: TApplicationEvents
    OnMessage = ApplicationEventsMessage
    Left = 224
    Top = 264
  end
  object OpenDialog: TOpenTextFileDialog
    Filter = '*.xml'
    Left = 200
    Top = 448
  end
  object NewDialog: TSaveTextFileDialog
    Filter = '*.xml'
    Left = 296
    Top = 448
  end
end
