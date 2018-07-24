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
    object ConstrainPosition: TButton
      Left = 24
      Top = 16
      Width = 105
      Height = 25
      Caption = 'Constrain Position'
      TabOrder = 0
    end
    object XAxis: TCheckBox
      Left = 215
      Top = 20
      Width = 50
      Height = 17
      Caption = 'X Axis'
      TabOrder = 4
    end
    object YAxis: TCheckBox
      Left = 215
      Top = 43
      Width = 50
      Height = 17
      Caption = 'Y Axis'
      TabOrder = 5
    end
    object ZAxis: TCheckBox
      Left = 215
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
