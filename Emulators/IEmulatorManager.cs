using PointerFinder2.Core;
using PointerFinder2.DataModels;
using System;
using System.Diagnostics;

namespace PointerFinder2.Emulators
{
    public interface IEmulatorManager
    {
        string EmulatorName { get; }
        string RetroAchievementsPrefix { get; }
        Process EmulatorProcess { get; }
        bool IsAttached { get; }
        nint ProcessHandle { get; }
        nint MemoryBasePC { get; }
        bool Attach();
        void Detach();
        byte[] ReadMemory(uint address, int count);
        uint? ReadUInt32(uint address);
        uint? RecalculateFinalAddress(PointerPath path);
        bool IsValidPointerTarget(uint value);
        string FormatDisplayAddress(uint address);
        uint UnnormalizeAddress(string address);
        AppSettings GetDefaultSettings();
    }
}