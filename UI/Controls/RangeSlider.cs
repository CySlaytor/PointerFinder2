using System;
using System.Drawing;
using System.Windows.Forms;

namespace PointerFinder2.UI.Controls
{
    // A custom TrackBar control with two thumbs to select a range of values.
    public class RangeSlider : Control
    {
        #region --- Properties ---
        private int _minimum = 0;
        public int Minimum
        {
            get => _minimum;
            set { _minimum = value; Invalidate(); }
        }

        private int _maximum = 100;
        public int Maximum
        {
            get => _maximum;
            set { _maximum = value; Invalidate(); }
        }

        private int _value1 = 20;
        public int Value1
        {
            get => _value1;
            set { _value1 = value; Invalidate(); ValueChanged?.Invoke(this, EventArgs.Empty); }
        }

        private int _value2 = 80;
        public int Value2
        {
            get => _value2;
            set { _value2 = value; Invalidate(); ValueChanged?.Invoke(this, EventArgs.Empty); }
        }

        public int MaxRange { get; set; } = 100;
        public int ThumbStep { get; set; } = 1;
        public int TrackStep { get; set; } = 10;

        public event EventHandler ValueChanged;
        #endregion

        #region --- Private Fields for Drawing and Interaction ---
        private Rectangle thumb1Rect, thumb2Rect, trackRect, selectedTrackRect;
        private bool isDragging1 = false;
        private bool isDragging2 = false;
        private bool isDraggingTrack = false;
        private int dragStartMouseX;
        private int dragStartValue1;
        private const int thumbWidth = 10;
        private const int trackHeight = 4;
        #endregion

        public RangeSlider()
        {
            DoubleBuffered = true;
            Height = 25;
        }

        #region --- Public Methods ---
        public void SetRange(int v1, int v2)
        {
            _value1 = Math.Max(Minimum, Math.Min(v1, Maximum));
            _value2 = Math.Min(Maximum, Math.Max(v2, Minimum));
            if (_value1 > _value2) _value1 = _value2;

            Invalidate();
            ValueChanged?.Invoke(this, EventArgs.Empty);
        }
        #endregion

        #region --- Drawing Logic ---
        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            e.Graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;

            trackRect = new Rectangle(
                thumbWidth / 2,
                (Height - trackHeight) / 2,
                Width - thumbWidth,
                trackHeight);

            int pos1 = (int)(((double)(Value1 - Minimum) / (Maximum - Minimum)) * trackRect.Width);
            int pos2 = (int)(((double)(Value2 - Minimum) / (Maximum - Minimum)) * trackRect.Width);

            thumb1Rect = new Rectangle(pos1, (Height - 20) / 2, thumbWidth, 20);
            thumb2Rect = new Rectangle(pos2, (Height - 20) / 2, thumbWidth, 20);

            using (var brush = new SolidBrush(Color.FromArgb(100, 100, 100)))
            {
                e.Graphics.FillRectangle(brush, trackRect);
            }

            selectedTrackRect = new Rectangle(
                thumb1Rect.Right,
                trackRect.Y,
                thumb2Rect.X - thumb1Rect.Right,
                trackRect.Height);

            using (var brush = new SolidBrush(Color.DodgerBlue))
            {
                e.Graphics.FillRectangle(brush, selectedTrackRect);
            }

            using (var brush = new SolidBrush(Color.Silver))
            {
                e.Graphics.FillRectangle(brush, thumb1Rect);
                e.Graphics.FillRectangle(brush, thumb2Rect);
            }
            using (var pen = new Pen(Color.FromArgb(50, 50, 50)))
            {
                e.Graphics.DrawRectangle(pen, thumb1Rect);
                e.Graphics.DrawRectangle(pen, thumb2Rect);
            }
        }
        #endregion

        #region --- Mouse Interaction Logic ---
        protected override void OnMouseDown(MouseEventArgs e)
        {
            base.OnMouseDown(e);
            if (thumb1Rect.Contains(e.Location))
            {
                isDragging1 = true;
            }
            else if (thumb2Rect.Contains(e.Location))
            {
                isDragging2 = true;
            }
            else if (selectedTrackRect.Contains(e.Location))
            {
                isDraggingTrack = true;
                dragStartMouseX = e.X;
                dragStartValue1 = Value1;
            }
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            bool ctrlPressed = ModifierKeys == Keys.Control;

            if (isDragging1)
            {
                int newValue = GetValueFromPosition(e.X);
                if (!ctrlPressed) newValue = SnapToStep(newValue, ThumbStep);
                newValue = Math.Min(newValue, Value2);
                if (!ctrlPressed && Value2 - newValue > MaxRange)
                {
                    newValue = Value2 - MaxRange;
                }
                Value1 = Math.Max(Minimum, newValue);
            }
            else if (isDragging2)
            {
                int newValue = GetValueFromPosition(e.X);
                if (!ctrlPressed) newValue = SnapToStep(newValue, ThumbStep);
                newValue = Math.Max(newValue, Value1);
                if (!ctrlPressed && newValue - Value1 > MaxRange)
                {
                    newValue = Value1 + MaxRange;
                }
                Value2 = Math.Min(Maximum, newValue);
            }
            else if (isDraggingTrack)
            {
                int currentMouseValue = GetValueFromPosition(e.X);
                int startMouseValue = GetValueFromPosition(dragStartMouseX);
                int deltaValue = currentMouseValue - startMouseValue;

                if (!ctrlPressed)
                {
                    deltaValue = SnapToStep(deltaValue, TrackStep);
                }

                int range = Value2 - Value1;
                int newV1 = dragStartValue1 + deltaValue;
                int newV2 = newV1 + range;

                if (newV1 < Minimum) { newV1 = Minimum; newV2 = newV1 + range; }
                if (newV2 > Maximum) { newV2 = Maximum; newV1 = newV2 - range; }

                SetRange(newV1, newV2);
            }
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            base.OnMouseUp(e);
            isDragging1 = false;
            isDragging2 = false;
            isDraggingTrack = false;
        }
        #endregion

        #region --- Helper Methods ---
        private int GetValueFromPosition(int x)
        {
            if (trackRect.Width == 0) return Minimum;
            double percentage = (double)(x - trackRect.X) / trackRect.Width;
            int value = (int)(percentage * (Maximum - Minimum)) + Minimum;
            return Math.Max(Minimum, Math.Min(value, Maximum));
        }

        private int SnapToStep(int value, int step)
        {
            if (step <= 0) return value;
            return (int)(Math.Round((double)value / step) * step);
        }
        #endregion
    }
}