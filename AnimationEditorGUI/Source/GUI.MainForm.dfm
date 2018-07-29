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
      Left = 24
      Top = 16
      Width = 105
      Height = 25
      Caption = 'Reset Root Position'
      TabOrder = 0
    end
    object XAxis: TCheckBox
      Left = 296
      Top = 20
      Width = 50
      Height = 17
      Caption = 'X Axis'
      TabOrder = 4
    end
    object YAxis: TCheckBox
      Left = 296
      Top = 43
      Width = 50
      Height = 17
      Caption = 'Y Axis'
      TabOrder = 5
    end
    object ZAxis: TCheckBox
      Left = 296
      Top = 66
      Width = 50
      Height = 17
      Caption = 'Z Axis'
      TabOrder = 6
    end
    object XPos: TCheckBox
      Left = 143
      Top = 20
      Width = 66
      Height = 17
      Caption = 'X Position'
      TabOrder = 1
    end
    object YPos: TCheckBox
      Left = 143
      Top = 43
      Width = 66
      Height = 17
      Caption = 'Y Position'
      TabOrder = 2
    end
    object ZPos: TCheckBox
      Left = 143
      Top = 66
      Width = 66
      Height = 17
      Caption = 'Z Position'
      TabOrder = 3
    end
    object XPosInput: TEdit
      Left = 215
      Top = 16
      Width = 49
      Height = 21
      TabOrder = 7
    end
    object YPosInput: TEdit
      Left = 215
      Top = 43
      Width = 49
      Height = 21
      TabOrder = 8
    end
    object ZPosInput: TEdit
      Left = 215
      Top = 70
      Width = 49
      Height = 21
      TabOrder = 9
    end
    object YAngleInput: TEdit
      Left = 352
      Top = 43
      Width = 49
      Height = 21
      TabOrder = 10
    end
    object ZAngleInput: TEdit
      Left = 352
      Top = 70
      Width = 49
      Height = 21
      TabOrder = 11
    end
    object XAngleInput: TEdit
      Left = 352
      Top = 16
      Width = 49
      Height = 21
      TabOrder = 12
    end
    object XAxisBar: TTrackBar
      Left = 407
      Top = 16
      Width = 75
      Height = 21
      Max = 3600
      TabOrder = 13
    end
    object YAxisBar: TTrackBar
      Left = 407
      Top = 43
      Width = 75
      Height = 21
      Max = 3600
      TabOrder = 14
    end
    object ZAxisBar: TTrackBar
      Left = 407
      Top = 70
      Width = 75
      Height = 21
      Max = 3600
      TabOrder = 15
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
end
