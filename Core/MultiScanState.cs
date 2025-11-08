using PointerFinder2.DataModels;
using System;
using System.Collections.Generic;
using System.Linq;

namespace PointerFinder2.Core
{
    // Created a new class to encapsulate the logic and storage for multi-state captures.
    // This improves decoupling between MainForm and StateCaptureForm, as the form no longer
    // directly manipulates MainForm's internal array.
    public class MultiScanState
    {
        private readonly ScanState[] _capturedStates = new ScanState[4];

        public ScanState this[int index]
        {
            get
            {
                if (index < 0 || index >= _capturedStates.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(index));
                }
                return _capturedStates[index];
            }
        }

        public int LastCapturedSlotIndex { get; private set; } = -1;

        public int CapturedCount => _capturedStates.Count(s => s != null);

        public void CaptureState(int slotIndex, ScanState state)
        {
            if (slotIndex < 0 || slotIndex >= _capturedStates.Length) return;
            _capturedStates[slotIndex] = state;
            UpdateLastCapturedIndex();
        }

        public void ReleaseState(int slotIndex)
        {
            if (slotIndex < 0 || slotIndex >= _capturedStates.Length) return;
            _capturedStates[slotIndex] = null;
            UpdateLastCapturedIndex();
        }

        public void ClearAll()
        {
            for (int i = 0; i < _capturedStates.Length; i++)
            {
                _capturedStates[i] = null;
            }
            LastCapturedSlotIndex = -1;
        }

        public List<ScanState> GetCapturedStates()
        {
            return _capturedStates.Where(s => s != null).ToList();
        }

        private void UpdateLastCapturedIndex()
        {
            LastCapturedSlotIndex = -1;
            for (int i = _capturedStates.Length - 1; i >= 0; i--)
            {
                if (_capturedStates[i] != null)
                {
                    LastCapturedSlotIndex = i;
                    break;
                }
            }
        }
    }
}