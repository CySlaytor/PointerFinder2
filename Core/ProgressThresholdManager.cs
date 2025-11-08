namespace PointerFinder2.Core
{
    // Created a new helper class to encapsulate the logic for dynamic progress reporting.
    // This removes duplicated code from the scanner strategy classes and makes the update frequency easier to manage.
    public class ProgressThresholdManager
    {
        private long _nextUpdateThreshold;
        private readonly long _initialThreshold;

        public ProgressThresholdManager(long initialThreshold = 100)
        {
            _initialThreshold = initialThreshold;
            _nextUpdateThreshold = initialThreshold;
        }

        public bool ShouldUpdate(long currentCount)
        {
            if (currentCount >= _nextUpdateThreshold)
            {
                // Dynamically increase the update interval as more results are found.
                if (currentCount < 10000)
                    _nextUpdateThreshold = currentCount + 100;
                else if (currentCount < 100000)
                    _nextUpdateThreshold = currentCount + 1000;
                else
                    _nextUpdateThreshold = currentCount + 10000;
                return true;
            }
            return false;
        }

        public void Reset()
        {
            _nextUpdateThreshold = _initialThreshold;
        }
    }
}