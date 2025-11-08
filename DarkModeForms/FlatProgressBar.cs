using System;
using System.Drawing;
using System.Windows.Forms;

namespace DarkModeForms
{
    /// <summary>
    /// A custom ProgressBar that allows for a flat appearance with a customizable bar color.
    /// This control uses user-painting to override the default system-drawn progress bar.
    /// </summary>
    public class FlatProgressBar : ProgressBar
    {
        private Color _progressBarColor = Color.Green;

        /// <summary>
        /// Gets or sets the color of the progress bar meter.
        /// </summary>
        public Color ProgressBarColor
        {
            get => _progressBarColor;
            set
            {
                _progressBarColor = value;
                this.Invalidate(); // Redraw the control when the color changes
            }
        }

        public FlatProgressBar()
        {
            // Set the control style to UserPaint to enable custom drawing.
            this.SetStyle(ControlStyles.UserPaint, true);
        }

        protected override void OnPaintBackground(PaintEventArgs pevent)
        {
            // Do nothing here to prevent the default background from being drawn, which helps reduce flicker.
        }

        protected override void OnResize(EventArgs e)
        {
            // Invalidate the control to force a repaint when its size changes.
            this.Invalidate();
            base.OnResize(e);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            // Use 'using' statements to ensure disposable resources like brushes are properly handled.
            using (var backBrush = new SolidBrush(this.BackColor))
            using (var barBrush = new SolidBrush(this.ProgressBarColor))
            {
                Rectangle rect = this.ClientRectangle;

                // Draw the background of the progress bar.
                g.FillRectangle(backBrush, rect);

                // Calculate the width of the progress meter based on the current value and range.
                // We use the properties from the base ProgressBar class.
                if (this.Maximum > this.Minimum)
                {
                    float percent = (float)(this.Value - this.Minimum) / (float)(this.Maximum - this.Minimum);
                    rect.Width = (int)((float)rect.Width * percent);

                    // Draw the progress meter, but only if it has a width greater than zero.
                    if (rect.Width > 0)
                    {
                        g.FillRectangle(barBrush, rect);
                    }
                }
            }

            // Draw a simple border around the control.
            Draw3DBorder(g);
        }

        private void Draw3DBorder(Graphics g)
        {
            int penWidth = (int)Pens.White.Width;

            // Use a single color for the border that contrasts with the dark theme background.
            using (Pen borderPen = new Pen(Color.DarkGray))
            {
                g.DrawLine(borderPen,
                    new Point(this.ClientRectangle.Left, this.ClientRectangle.Top),
                    new Point(this.ClientRectangle.Width - penWidth, this.ClientRectangle.Top));

                g.DrawLine(borderPen,
                    new Point(this.ClientRectangle.Left, this.ClientRectangle.Top),
                    new Point(this.ClientRectangle.Left, this.ClientRectangle.Height - penWidth));

                g.DrawLine(borderPen,
                    new Point(this.ClientRectangle.Left, this.ClientRectangle.Height - penWidth),
                    new Point(this.ClientRectangle.Width - penWidth, this.ClientRectangle.Height - penWidth));

                g.DrawLine(borderPen,
                    new Point(this.ClientRectangle.Width - penWidth, this.ClientRectangle.Top),
                    new Point(this.ClientRectangle.Width - penWidth, this.ClientRectangle.Height - penWidth));
            }
        }
    }
}