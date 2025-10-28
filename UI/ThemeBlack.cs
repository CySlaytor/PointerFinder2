using System.Windows.Forms;
using DarkModeForms;

namespace PointerFinder2.UI
{
    public static class ThemeBlack
    {
        public static void Apply(Form form)
        {
            var dm = new DarkModeCS(form)
            {
                ColorMode = DarkModeCS.DisplayMode.SystemDefault
            };
        }
    }
}
