using PointerFinder2.DataModels;
using System;
using System.IO;
using System.Media;
using System.Reflection; 
using System.Windows.Forms;

namespace PointerFinder2.Core
{
    // Manages custom and default sound playback.
    // It prioritizes external .wav files in a "Sounds" folder for user customization.
    // If external files aren't found, it falls back to default sounds embedded in the application.
    public static class SoundManager
    {
        private static readonly SoundPlayer _successPlayer;
        private static readonly SoundPlayer _failPlayer;
        private static readonly SoundPlayer _notifyPlayer;

        // The constructor loads all sounds on startup.
        static SoundManager()
        {
            _successPlayer = LoadPlayer("success.wav", "PointerFinder2.Sounds.success.wav");
            _failPlayer = LoadPlayer("fail.wav", "PointerFinder2.Sounds.fail.wav");
            _notifyPlayer = LoadPlayer("notify.wav", "PointerFinder2.Sounds.notify.wav");
        }

        // A helper that tries to load an external sound first, then falls back to an embedded one.
        private static SoundPlayer LoadPlayer(string externalFileName, string embeddedResourceName)
        {
            // 1. Check for an external custom sound file.
            string externalPath = Path.Combine(Application.StartupPath, "Sounds", externalFileName);

            if (File.Exists(externalPath))
            {
                try
                {
                    var player = new SoundPlayer(externalPath);
                    player.Load();
                    return player;
                }
                catch (Exception ex)
                {
                    DebugLogForm.Instance.Log($"[SoundManager] Failed to load custom sound '{externalFileName}': {ex.Message}");
                }
            }

            // 2. Fall back to the embedded resource.
            var assembly = Assembly.GetExecutingAssembly();
            Stream resourceStream = assembly.GetManifestResourceStream(embeddedResourceName);

            if (resourceStream != null)
            {
                try
                {
                    var player = new SoundPlayer(resourceStream);
                    player.Load();
                    return player;
                }
                catch (Exception ex)
                {
                    DebugLogForm.Instance.Log($"[SoundManager] Failed to load embedded sound '{embeddedResourceName}': {ex.Message}");
                }
            }
            return null;
        }

        // Plays the success sound.
        public static void PlaySuccess()
        {
            if (GlobalSettings.UseWindowsDefaultSound)
            {
                SystemSounds.Asterisk.Play();
                return;
            }
            _successPlayer?.Play();
        }

        // Plays the fail/no-results sound.
        public static void PlayFail()
        {
            if (GlobalSettings.UseWindowsDefaultSound)
            {
                SystemSounds.Hand.Play();
                return;
            }
            _failPlayer?.Play();
        }

        // Plays the general notification sound.
        public static void PlayNotify()
        {
            if (GlobalSettings.UseWindowsDefaultSound)
            {
                SystemSounds.Asterisk.Play();
                return;
            }
            _notifyPlayer?.Play();
        }
    }
}