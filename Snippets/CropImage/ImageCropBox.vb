﻿Imports System.Diagnostics
Imports System.Drawing
Imports System.Drawing.Imaging
Imports System.Drawing.Drawing2D

Public Class ImageCropBox
    Private Const Spacing As Integer = 2
    Private Const SpotSize As Integer = 7
    Private _Image As Bitmap
    Private _FadeImage As Bitmap
    Private _MinimalSelectionSize As Size
    Private _SelectionRectangle As Rectangle
    Private _PreserveAspectRatio As Boolean = False
    Private ClientImageOrigin As Point
    Private ClientImageExtent As Size
    Private ClientSelectionRectangle As Rectangle
    Private BoundaryPenA As Pen
    Private BoundaryPenB As Pen
    Private SpotBrush As Brush
    Private SpotPen As Pen
    Private DragSpotIndex As Integer ' 0..3 Corner Resize, 4 Body Move
    Private MouseDownSelection As Rectangle
    Private MouseDownPosition As Point

    Public Event SelectionRectangleChanged As EventHandler
    Public Event SelectionDoubleClick As EventHandler

    Public Sub New()
        ' This call is required by the designer.
        InitializeComponent()
        ' Add any initialization after the InitializeComponent() call.
        _MinimalSelectionSize = New Size(0, 0)
        _PreserveAspectRatio = False
    End Sub

    Private Function ApplyMinimalSelection(ByVal SelectionRectangle As Rectangle) As Rectangle
        Dim MinimalSelectionSize = EffectiveMinimalSelectionSize
        If SelectionRectangle.Width < MinimalSelectionSize.Width Then
            SelectionRectangle.Width = MinimalSelectionSize.Width
            If _Image IsNot Nothing AndAlso SelectionRectangle.Right > _Image.Width Then SelectionRectangle.X = Math.Max(0, _Image.Width - SelectionRectangle.Width)
        End If
        If SelectionRectangle.Height < MinimalSelectionSize.Height Then
            SelectionRectangle.Height = MinimalSelectionSize.Height
            If _Image IsNot Nothing AndAlso SelectionRectangle.Bottom > _Image.Height Then SelectionRectangle.Y = Math.Max(0, _Image.Height - SelectionRectangle.Height)
        End If
        ApplyMinimalSelection = SelectionRectangle
    End Function
    Private Function ApplyAspectRatio(ByVal SelectionRectangle As Rectangle, ByRef AdjustedSelectionRectangle As Rectangle) As Boolean
        AdjustedSelectionRectangle = SelectionRectangle
        ApplyAspectRatio = False
        If Not _PreserveAspectRatio Then Exit Function
        If _Image Is Nothing Then Exit Function
        Debug.Assert(_Image.Height > 0 AndAlso _Image.Width > 0)
        If SelectionRectangle.Width <= 0 Or SelectionRectangle.Height <= 0 Then Exit Function
        Dim AdjustedHeight As Integer = Math.Round(SelectionRectangle.Width * _Image.Height / _Image.Width)
        Dim AdjustedWidth As Integer = Math.Round(SelectionRectangle.Height * _Image.Width / _Image.Height)
        Debug.Assert(AdjustedWidth >= SelectionRectangle.Width Or AdjustedHeight >= SelectionRectangle.Height)
        If AdjustedWidth < SelectionRectangle.Width Then
            AdjustedSelectionRectangle.Width = AdjustedWidth
            ApplyAspectRatio = True
        ElseIf AdjustedHeight < SelectionRectangle.Height Then
            AdjustedSelectionRectangle.Height = AdjustedHeight
            ApplyAspectRatio = True
        End If
    End Function
    Private Function ApplyAspectRatioAndCenterRectangle(ByVal SelectionRectangle As Rectangle) As Rectangle
        Dim AdjustedSelectionRectangle As Rectangle
        If ApplyAspectRatio(SelectionRectangle, AdjustedSelectionRectangle) Then
            AdjustedSelectionRectangle.Offset((SelectionRectangle.Width - AdjustedSelectionRectangle.Width) / 2, (SelectionRectangle.Height - AdjustedSelectionRectangle.Height) / 2)
            ApplyAspectRatioAndCenterRectangle = AdjustedSelectionRectangle
        Else
            ApplyAspectRatioAndCenterRectangle = SelectionRectangle
        End If
    End Function

    Public Property Image As Bitmap
        Get
            Image = _Image
        End Get
        Set(value As Bitmap)
            _Image = value
            If _Image IsNot Nothing Then
                SelectionRectangle = New Rectangle(value.Width * 1 / 16, value.Height * 1 / 16, value.Width * 14 / 16, value.Height * 14 / 16)
                'SelectionRectangle = New Rectangle(0, 0, value.Width, value.Height)
                _FadeImage = _Image.Clone
                Dim FadeImageGraphics As Graphics = Graphics.FromImage(_FadeImage)
                Dim FadeBrush As HatchBrush = New HatchBrush(HatchStyle.Percent25, Color.FromArgb(0, Color.Black), Color.FromArgb(255, Color.Black))
                FadeImageGraphics.FillRectangle(FadeBrush, 0, 0, _FadeImage.Width, _FadeImage.Height)
                FadeImageGraphics.Save()
            Else
                _FadeImage = Nothing
            End If
            Update()
        End Set
    End Property
    Public Property MinimalSelectionSize As Size
        Get
            MinimalSelectionSize = _MinimalSelectionSize
        End Get
        Set(value As Size)
            Debug.Assert(value.Width >= 0 And value.Height >= 0)
            _MinimalSelectionSize = value
            ' TODO: Update SelectionRectangle
        End Set
    End Property
    Private ReadOnly Property EffectiveMinimalSelectionSize As Size
        Get
            EffectiveMinimalSelectionSize = MinimalSelectionSize
            If MinimalSelectionSize.Width > 0 And MinimalSelectionSize.Height > 0 And PreserveAspectRatio And Image IsNot Nothing Then
                Dim Position = New Rectangle(0, 0, MinimalSelectionSize.Width, MinimalSelectionSize.Height)
                Dim AdjustedPositon As Rectangle
                If ApplyAspectRatio(Position, AdjustedPositon) Then EffectiveMinimalSelectionSize = AdjustedPositon.Size
            End If
        End Get
    End Property
    Public Property SelectionRectangle As Rectangle
        Get
            SelectionRectangle = _SelectionRectangle
        End Get
        Set(value As Rectangle)
            Dim EffectiveSelectionRectangle As Rectangle = ApplyMinimalSelection(value)
            If PreserveAspectRatio Then EffectiveSelectionRectangle = ApplyAspectRatioAndCenterRectangle(EffectiveSelectionRectangle)
            Dim SelectionRectangleChanged As Boolean = _SelectionRectangle <> EffectiveSelectionRectangle
            _SelectionRectangle = EffectiveSelectionRectangle
            RaiseEvent SelectionRectangleChanged(Me, New System.EventArgs())
            If SelectionRectangleChanged Then Invalidate()
        End Set
    End Property
    Public Property PreserveAspectRatio As Boolean
        Get
            PreserveAspectRatio = _PreserveAspectRatio
        End Get
        Set(value As Boolean)
            If _PreserveAspectRatio = value Then Exit Property
            _PreserveAspectRatio = value
            If _PreserveAspectRatio Then SelectionRectangle = ApplyAspectRatioAndCenterRectangle(_SelectionRectangle)
        End Set
    End Property

    Private Sub ImageCropBox_Load(sender As System.Object, e As System.EventArgs) Handles MyBase.Load
        BoundaryPenA = New Pen(Brushes.White)
        BoundaryPenA.Width = 1
        BoundaryPenA.DashPattern = New Single() {8.0F, 4.0F}
        BoundaryPenB = New Pen(Brushes.Black)
        BoundaryPenB.Width = 1
        BoundaryPenB.DashPattern = New Single() {4.0F, 8.0F}
        BoundaryPenB.DashOffset = 4
        SpotBrush = Brushes.Black
        SpotPen = New Pen(Brushes.White)
        SpotPen.Width = 1
        ResizeRedraw = True
    End Sub
    Private Sub DrawSpot(Graphics As Graphics, Position As Point)
        Graphics.FillRectangle(SpotBrush, Position.X - CInt(SpotSize / 2) - 1, Position.Y - CInt(SpotSize / 2) - 1, SpotSize + 2, SpotSize + 2)
        Graphics.DrawRectangle(SpotPen, Position.X - CInt(SpotSize / 2), Position.Y - CInt(SpotSize / 2), SpotSize, SpotSize)
    End Sub
    Private Sub DrawSpot(Graphics As Graphics, PositionX As Integer, PositionY As Integer)
        DrawSpot(Graphics, New Point(PositionX, PositionY))
    End Sub
    Private Sub ImageCropBox_Paint(sender As System.Object, e As System.Windows.Forms.PaintEventArgs) Handles MyBase.Paint
        If Image Is Nothing Then Exit Sub
        Dim Extent = Me.Size
        Extent.Width -= Spacing + SpotSize + Spacing
        Extent.Height -= Spacing + SpotSize + Spacing
        If Extent.Width <= 0 Or Extent.Height <= 0 Then Exit Sub
        Debug.Assert(Image.Width > 0 AndAlso Image.Height > 0)
        Dim Ratio As Double = Math.Max(Image.Width / Extent.Width, Image.Height / Extent.Height)
        If Ratio > 1 And Ratio < 1.1 Then Ratio = 1.0 ' Snap to 100%
        ClientImageExtent.Width = Math.Round(Image.Width / Ratio)
        ClientImageExtent.Height = Math.Round(Image.Height / Ratio)
        ClientImageOrigin.X = Spacing + SpotSize / 2 + (Extent.Width - ClientImageExtent.Width) / 2
        ClientImageOrigin.Y = Spacing + SpotSize / 2 + (Extent.Height - ClientImageExtent.Height) / 2
        e.Graphics.DrawImage(_FadeImage, ClientImageOrigin.X, ClientImageOrigin.Y, ClientImageExtent.Width, ClientImageExtent.Height)
        ClientSelectionRectangle.X = Math.Round(_SelectionRectangle.Left / Ratio)
        ClientSelectionRectangle.Y = Math.Round(_SelectionRectangle.Top / Ratio)
        ClientSelectionRectangle.Width = Math.Round(_SelectionRectangle.Right / Ratio) - ClientSelectionRectangle.X
        ClientSelectionRectangle.Height = Math.Round(_SelectionRectangle.Bottom / Ratio) - ClientSelectionRectangle.Y
        ClientSelectionRectangle.Offset(ClientImageOrigin)
        e.Graphics.SetClip(ClientSelectionRectangle)
        e.Graphics.DrawImage(_Image, ClientImageOrigin.X, ClientImageOrigin.Y, ClientImageExtent.Width, ClientImageExtent.Height)
        e.Graphics.ResetClip()
        e.Graphics.DrawRectangle(BoundaryPenA, ClientSelectionRectangle)
        e.Graphics.DrawRectangle(BoundaryPenB, ClientSelectionRectangle)
        DrawSpot(e.Graphics, ClientSelectionRectangle.Left, ClientSelectionRectangle.Top)
        DrawSpot(e.Graphics, ClientSelectionRectangle.Right, ClientSelectionRectangle.Top)
        DrawSpot(e.Graphics, ClientSelectionRectangle.Left, ClientSelectionRectangle.Bottom)
        DrawSpot(e.Graphics, ClientSelectionRectangle.Right, ClientSelectionRectangle.Bottom)
    End Sub
    Private Function SourcePointFromPoint(Position As Point) As Point
        Dim SourcePosition As Point
        SourcePosition.X = Math.Round((Position.X - ClientImageOrigin.X) * Image.Width / ClientImageExtent.Width)
        SourcePosition.Y = Math.Round((Position.Y - ClientImageOrigin.Y) * Image.Height / ClientImageExtent.Height)
        SourcePointFromPoint = SourcePosition
    End Function
    Private Function PointFromSourcePoint(SourcePosition As Point) As Point
        Dim Position As Point
        Position.X = ClientImageOrigin.X + Math.Round(SourcePosition.X * ClientImageExtent.Width / Image.Width)
        Position.Y = ClientImageOrigin.Y + Math.Round(SourcePosition.Y * ClientImageExtent.Height / Image.Height)
        PointFromSourcePoint = Position
    End Function
    Private Function IsSpotPosition(SpotPosition As Point, Position As Point) As Boolean
        Dim SpotPositionEx = New Rectangle(SpotPosition.X - SpotSize / 2, SpotPosition.Y - SpotSize / 2, SpotSize, SpotSize)
        IsSpotPosition = SpotPositionEx.Contains(Position)
    End Function
    Private Function IsSpotPosition(SpotPositionX As Integer, SpotPositionY As Integer, PositionX As Integer, PositionY As Integer) As Boolean
        IsSpotPosition = IsSpotPosition(New Point(SpotPositionX, SpotPositionY), New Point(PositionX, PositionY))
    End Function
    Private Sub ApplyPointConstraint(ByRef Position As Point, P1 As Point, P2 As Point)
        If Position.X > P2.X Then Position.X = P2.X
        If Position.Y > P2.Y Then Position.Y = P2.Y
        If Position.X < P1.X Then Position.X = P1.X
        If Position.Y < P1.Y Then Position.Y = P1.Y
    End Sub
    Private Sub ImageCropBox_MouseMove(sender As Object, e As System.Windows.Forms.MouseEventArgs) Handles Me.MouseMove
        Dim Position As Point = e.Location, SourcePosition As Point
        If Capture Then
            Dim MinimalSelectionSize = EffectiveMinimalSelectionSize
            Dim P1 As Point, P2 As Point
            Dim NewSelectionRectangle As Rectangle
            Select Case DragSpotIndex
                Case 0 ' Left Top
                    P1 = ClientImageOrigin
                    P2 = PointFromSourcePoint(New Point(SelectionRectangle.Right - MinimalSelectionSize.Width, SelectionRectangle.Bottom - MinimalSelectionSize.Height))
                    ApplyPointConstraint(Position, P1, P2)
                    SourcePosition = SourcePointFromPoint(Position)
                    NewSelectionRectangle = Rectangle.FromLTRB(SourcePosition.X, SourcePosition.Y, SelectionRectangle.Right, SelectionRectangle.Bottom)
                    NewSelectionRectangle = ApplyMinimalSelection(ApplyAspectRatioAndCenterRectangle(NewSelectionRectangle))
                    NewSelectionRectangle.Offset(SelectionRectangle.Right - NewSelectionRectangle.Right, SelectionRectangle.Bottom - NewSelectionRectangle.Bottom)
                    SelectionRectangle = NewSelectionRectangle
                Case 1 ' Right Top
                    P1 = New Point(ClientImageOrigin.X + ClientImageExtent.Width, ClientImageOrigin.Y)
                    P2 = PointFromSourcePoint(New Point(SelectionRectangle.Left + MinimalSelectionSize.Width, SelectionRectangle.Bottom - MinimalSelectionSize.Height))
                    ApplyPointConstraint(Position, New Point(P2.X, P1.Y), New Point(P1.X, P2.Y))
                    SourcePosition = SourcePointFromPoint(Position)
                    NewSelectionRectangle = Rectangle.FromLTRB(SelectionRectangle.Left, SourcePosition.Y, SourcePosition.X, SelectionRectangle.Bottom)
                    NewSelectionRectangle = ApplyMinimalSelection(ApplyAspectRatioAndCenterRectangle(NewSelectionRectangle))
                    NewSelectionRectangle.Offset(SelectionRectangle.Left - NewSelectionRectangle.Left, SelectionRectangle.Bottom - NewSelectionRectangle.Bottom)
                    SelectionRectangle = NewSelectionRectangle
                Case 2 ' Left Bottom
                    P1 = New Point(ClientImageOrigin.X, ClientImageOrigin.Y + ClientImageExtent.Height)
                    P2 = PointFromSourcePoint(New Point(SelectionRectangle.Right - MinimalSelectionSize.Width, SelectionRectangle.Top + MinimalSelectionSize.Height))
                    ApplyPointConstraint(Position, New Point(P1.X, P2.Y), New Point(P2.X, P1.Y))
                    SourcePosition = SourcePointFromPoint(Position)
                    NewSelectionRectangle = Rectangle.FromLTRB(SourcePosition.X, SelectionRectangle.Top, SelectionRectangle.Right, SourcePosition.Y)
                    NewSelectionRectangle = ApplyMinimalSelection(ApplyAspectRatioAndCenterRectangle(NewSelectionRectangle))
                    NewSelectionRectangle.Offset(SelectionRectangle.Right - NewSelectionRectangle.Right, SelectionRectangle.Top - NewSelectionRectangle.Top)
                    SelectionRectangle = NewSelectionRectangle
                Case 3 ' Right Bottom
                    P1 = PointFromSourcePoint(New Point(SelectionRectangle.Left + MinimalSelectionSize.Width, SelectionRectangle.Top + MinimalSelectionSize.Height))
                    P2 = ClientImageOrigin + ClientImageExtent
                    ApplyPointConstraint(Position, P1, P2)
                    SourcePosition = SourcePointFromPoint(Position)
                    NewSelectionRectangle = Rectangle.FromLTRB(SelectionRectangle.Left, SelectionRectangle.Top, SourcePosition.X, SourcePosition.Y)
                    NewSelectionRectangle = ApplyMinimalSelection(ApplyAspectRatioAndCenterRectangle(NewSelectionRectangle))
                    NewSelectionRectangle.Offset(SelectionRectangle.Left - NewSelectionRectangle.Left, SelectionRectangle.Top - NewSelectionRectangle.Top)
                    SelectionRectangle = NewSelectionRectangle
                Case 4 ' Move
                    Dim Move As Size = SourcePointFromPoint(e.Location) - SourcePointFromPoint(MouseDownPosition)
                    Dim PreSelection As Rectangle = MouseDownSelection
                    PreSelection.Offset(Move)
                    PreSelection.Offset(Math.Max(0, -PreSelection.Left), Math.Max(0, -PreSelection.Top))
                    PreSelection.Offset(-Math.Max(0, PreSelection.Right - Image.Width), -Math.Max(0, PreSelection.Bottom - Image.Height))
                    SelectionRectangle = PreSelection
            End Select
            Exit Sub
        Else
            If IsSpotPosition(ClientSelectionRectangle.Left, ClientSelectionRectangle.Top, e.X, e.Y) Then
                Cursor = Cursors.SizeNWSE
                Exit Sub
            End If
            If IsSpotPosition(ClientSelectionRectangle.Right, ClientSelectionRectangle.Top, e.X, e.Y) Then
                Cursor = Cursors.SizeNESW
                Exit Sub
            End If
            If IsSpotPosition(ClientSelectionRectangle.Left, ClientSelectionRectangle.Bottom, e.X, e.Y) Then
                Cursor = Cursors.SizeNESW
                Exit Sub
            End If
            If IsSpotPosition(ClientSelectionRectangle.Right, ClientSelectionRectangle.Bottom, e.X, e.Y) Then
                Cursor = Cursors.SizeNWSE
                Exit Sub
            End If
            If ClientSelectionRectangle.Contains(e.Location) Then
                Cursor = Cursors.SizeAll
                Exit Sub
            End If
        End If
        Cursor = Cursors.Default
    End Sub
    Private Sub ImageCropBox_MouseDown(sender As Object, e As System.Windows.Forms.MouseEventArgs) Handles Me.MouseDown
        DragSpotIndex = -1
        MouseDownSelection = SelectionRectangle
        MouseDownPosition = e.Location
        If IsSpotPosition(ClientSelectionRectangle.Left, ClientSelectionRectangle.Top, e.X, e.Y) Then
            DragSpotIndex = 0
            Capture = True
            Exit Sub
        End If
        If IsSpotPosition(ClientSelectionRectangle.Right, ClientSelectionRectangle.Top, e.X, e.Y) Then
            DragSpotIndex = 1
            Capture = True
            Exit Sub
        End If
        If IsSpotPosition(ClientSelectionRectangle.Left, ClientSelectionRectangle.Bottom, e.X, e.Y) Then
            DragSpotIndex = 2
            Capture = True
            Exit Sub
        End If
        If IsSpotPosition(ClientSelectionRectangle.Right, ClientSelectionRectangle.Bottom, e.X, e.Y) Then
            DragSpotIndex = 3
            Capture = True
            Exit Sub
        End If
        If ClientSelectionRectangle.Contains(e.Location) Then
            DragSpotIndex = 4
            Capture = True
            Exit Sub
        End If
    End Sub
    Private Sub ImageCropBox_MouseUp(sender As Object, e As System.Windows.Forms.MouseEventArgs) Handles Me.MouseUp
        Capture = False
    End Sub
    Private Sub ImageCropBox_MouseDoubleClick(sender As Object, e As System.Windows.Forms.MouseEventArgs) Handles Me.MouseDoubleClick
        If ClientSelectionRectangle.Contains(e.Location) Then
            RaiseEvent SelectionDoubleClick(Me, New System.EventArgs)
            Exit Sub
        End If
    End Sub
End Class
