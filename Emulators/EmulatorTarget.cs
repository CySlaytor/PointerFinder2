namespace PointerFinder2.Emulators
{
    // An enumeration to uniquely identify each supported emulator target.
    // This is primarily used for managing separate sections in the settings.ini file.
    public enum EmulatorTarget
    {
        PCSX2,
        DuckStation,
        RALibretroNDS,
        Dolphin
    }
}