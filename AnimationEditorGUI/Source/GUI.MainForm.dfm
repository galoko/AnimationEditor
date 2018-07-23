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
    object TestButton: TButton
      Left = 232
      Top = 48
      Width = 75
      Height = 25
      Caption = 'Test'
      TabOrder = 0
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
  object Button2: TButton
    Left = 248
    Top = 96
    Width = 75
    Height = 25
    Caption = 'Suchara'
    TabOrder = 2
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
    TabOrder = 3
  end
  object ApplicationEvents: TApplicationEvents
    OnMessage = ApplicationEventsMessage
    Left = 224
    Top = 264
  end
end
