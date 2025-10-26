namespace PointerFinder2.DataModels
{
    // Holds all user-configurable settings for generating code notes.
    public class CodeNoteSettings
    {
        public string Prefix { get; set; }
        public string Suffix { get; set; }
        public bool AlignSuffixes { get; set; }
        //Added the missing property.
        public bool SuffixOnLastLineOnly { get; set; }

        // A helper method to create a settings object from the current global settings.
        public static CodeNoteSettings GetFromGlobalSettings()
        {
            return new CodeNoteSettings
            {
                Prefix = GlobalSettings.CodeNotePrefix,
                Suffix = GlobalSettings.CodeNoteSuffix,
                AlignSuffixes = GlobalSettings.CodeNoteAlignSuffixes,
                //Removed obsolete properties and added the new one.
                SuffixOnLastLineOnly = GlobalSettings.CodeNoteSuffixOnLastLineOnly
            };
        }
    }
}