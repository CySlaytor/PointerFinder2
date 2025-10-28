namespace PointerFinder2.UI
{
    public class BaseForm : Form
    {
        public BaseForm()
        {
            ThemeBlack.Apply(this);
            this.TopMost = false;
        }
    }
}
