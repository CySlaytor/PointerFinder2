using PointerFinder2.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PointerFinder2.UI
{
    // A tool to parse, merge, and clean up complex code notes with hierarchical pointer chains.
    public partial class CodeNoteHierarchyFixerForm : Form
    {
        private const string INPUT_PLACEHOLDER = "Paste your raw code note here...";
        private const string OUTPUT_PLACEHOLDER = "Fixed output will appear here...";
        private bool _isPlaceholderActive = true;

        public CodeNoteHierarchyFixerForm()
        {
            InitializeComponent();
            SetupPlaceholder();
            SetOutputPlaceholder();
        }

        // Added Load event handler to restore window position and size.
        private void CodeNoteHierarchyFixerForm_Load(object sender, EventArgs e)
        {
            if (Settings.Default.CodeNoteHierarchyFixerSize.Width > 0 && Settings.Default.CodeNoteHierarchyFixerSize.Height > 0)
            {
                this.StartPosition = FormStartPosition.Manual;
                Point location = Settings.Default.CodeNoteHierarchyFixerLocation;
                Size size = Settings.Default.CodeNoteHierarchyFixerSize;
                bool isVisible = false;
                foreach (Screen screen in Screen.AllScreens)
                {
                    if (screen.WorkingArea.IntersectsWith(new Rectangle(location, size)))
                    {
                        isVisible = true;
                        break;
                    }
                }
                if (isVisible)
                {
                    this.Location = location;
                    this.Size = size;
                }
            }
        }

        // Added FormClosing event handler to save window position and size.
        private void CodeNoteHierarchyFixerForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            Settings.Default.CodeNoteHierarchyFixerLocation = this.Location;
            Settings.Default.CodeNoteHierarchyFixerSize = this.Size;
            Settings.Default.Save();
        }

        // Represents a single line/node in the code note's hierarchy.
        private class CodeNoteNode
        {
            // The content of the line without leading indentation, used as the key for merging.
            public string Content { get; set; }
            // Child nodes in the hierarchy.
            public List<CodeNoteNode> Children { get; } = new List<CodeNoteNode>();
            // The original index, used to maintain order for non-mergeable items.
            public int OriginalIndex { get; set; }
        }

        private void btnFix_Click(object sender, EventArgs e)
        {
            try
            {
                string inputText = _isPlaceholderActive ? "" : richTextInput.Text;
                if (string.IsNullOrWhiteSpace(inputText))
                {
                    SetOutputPlaceholder();
                    return;
                }

                var lines = inputText.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);

                char indentChar = DetectIndentStyle(lines);

                // 1. Parse the flat text into a tree structure.
                var parsedTree = ParseLinesToTree(lines);

                // 2. Recursively merge nodes with identical content.
                var mergedTree = MergeTree(parsedTree);

                // 3. Rebuild the text from the cleaned tree structure, using the detected style.
                var rebuiltText = RebuildTextFromTree(mergedTree, 0, indentChar);

                // 4. Final text is the trimmed result.
                var finalText = rebuiltText.Trim();

                richTextOutput.ForeColor = SystemColors.WindowText;
                richTextOutput.Text = finalText;
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An error occurred during processing: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private char DetectIndentStyle(string[] lines)
        {
            // If any line uses multiple '+' for indentation, prefer that style.
            if (lines.Any(line => line.TrimStart().StartsWith("++")))
            {
                return '+';
            }
            // Otherwise, default to dots.
            return '.';
        }

        private List<CodeNoteNode> ParseLinesToTree(string[] lines)
        {
            var rootNodes = new List<CodeNoteNode>();
            var stack = new Stack<(int indent, List<CodeNoteNode> parentChildrenList)>();
            stack.Push((-1, rootNodes));
            CodeNoteNode lastNode = null;
            int lineIndex = 0;

            foreach (var line in lines)
            {
                string trimmedLine = line.Trim();
                if (string.IsNullOrEmpty(trimmedLine) || trimmedLine.StartsWith("---"))
                {
                    lineIndex++;
                    continue;
                }

                int indent = 0;
                string content;

                string untrimmedLine = line.TrimEnd();

                // Count leading characters that define indentation level
                int prefixLength = untrimmedLine.TakeWhile(c => c == '.' || c == '+').Count();

                if (prefixLength > 0)
                {
                    // For plus-based indents like "++0x...", the indent is one less than the count
                    if (untrimmedLine[prefixLength - 1] == '+' && untrimmedLine.Length > prefixLength && untrimmedLine[prefixLength] != ' ')
                    {
                        indent = prefixLength - 1;
                        content = untrimmedLine.Substring(indent).Trim();
                    }
                    else
                    {
                        indent = prefixLength;
                        content = untrimmedLine.Substring(indent).Trim();
                    }
                }
                else
                {
                    indent = 0;
                    content = untrimmedLine.Trim();
                }

                if (indent < 0) indent = 0;

                bool isAttachedNote = (indent == 0) && (lastNode != null) &&
                                      !(content.StartsWith("+0x") || content.StartsWith("["));

                if (isAttachedNote)
                {
                    lastNode.Children.Add(new CodeNoteNode { Content = content, OriginalIndex = lineIndex });
                }
                else
                {
                    while (stack.Peek().indent >= indent)
                    {
                        stack.Pop();
                    }

                    var node = new CodeNoteNode { Content = content, OriginalIndex = lineIndex };
                    stack.Peek().parentChildrenList.Add(node);
                    stack.Push((indent, node.Children));
                    lastNode = node;
                }
                lineIndex++;
            }
            return rootNodes;
        }

        private List<CodeNoteNode> MergeTree(List<CodeNoteNode> nodes)
        {
            var finalNodes = new List<CodeNoteNode>();
            var mergeableGroups = new Dictionary<string, List<CodeNoteNode>>();
            var nonMergeableNodes = new List<CodeNoteNode>();

            // First pass: Group all mergeable nodes by their core offset key (e.g., "+0x174").
            foreach (var node in nodes)
            {
                var match = Regex.Match(node.Content, @"^([\+\-]0x[0-9a-fA-F]+)");
                if (match.Success)
                {
                    string key = match.Groups[1].Value;
                    if (!mergeableGroups.ContainsKey(key))
                    {
                        mergeableGroups[key] = new List<CodeNoteNode>();
                    }
                    mergeableGroups[key].Add(node);
                }
                else
                {
                    nonMergeableNodes.Add(node);
                }
            }

            // Process mergeable groups.
            foreach (var group in mergeableGroups.Values)
            {
                // Use the first node of the group as the representative for content and original order.
                var representative = group.OrderBy(n => n.OriginalIndex).First();
                var newNode = new CodeNoteNode { Content = representative.Content, OriginalIndex = representative.OriginalIndex };

                // Combine all children from all nodes in the group.
                var allChildren = group.SelectMany(n => n.Children).ToList();
                if (allChildren.Any())
                {
                    newNode.Children.AddRange(MergeTree(allChildren));
                }
                finalNodes.Add(newNode);
            }

            // Process non-mergeable nodes, which just pass through but have their children merged.
            foreach (var node in nonMergeableNodes)
            {
                if (node.Children.Any())
                {
                    var mergedChildren = MergeTree(node.Children);
                    node.Children.Clear();
                    node.Children.AddRange(mergedChildren);
                }
                finalNodes.Add(node);
            }

            // Sort the final list to maintain the original file order.
            return finalNodes.OrderBy(n => n.OriginalIndex).ToList();
        }

        // Integrated spacing logic directly into the rebuild function.
        private string RebuildTextFromTree(List<CodeNoteNode> nodes, int indentLevel, char indentChar)
        {
            var sb = new StringBuilder();
            foreach (var node in nodes)
            {
                // Add a blank line before any top-level node except the very first one.
                if (indentLevel == 0 && sb.Length > 0)
                {
                    sb.AppendLine();
                }

                // Attached notes (like comments) always use '.' for their indentation.
                bool isAttachedNote = !(node.Content.StartsWith("+0x") || node.Content.StartsWith("[") || node.Content.StartsWith("-0x"));
                char currentChar = isAttachedNote ? '.' : indentChar;

                string prefix = indentLevel > 0 ? new string(currentChar, indentLevel) : "";

                sb.AppendLine($"{prefix}{node.Content}");

                if (node.Children.Any())
                {
                    sb.Append(RebuildTextFromTree(node.Children, indentLevel + 1, indentChar));
                }
            }
            return sb.ToString();
        }

        private async void btnCopy_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrWhiteSpace(richTextOutput.Text)) return;
            Clipboard.SetText(richTextOutput.Text);

            var originalText = btnCopy.Text;
            var originalColor = btnCopy.BackColor;
            btnCopy.Text = "✔️ Copied!";
            btnCopy.BackColor = Color.SeaGreen;
            btnCopy.Enabled = false;

            await Task.Delay(2000);

            if (!this.IsDisposed)
            {
                btnCopy.Text = originalText;
                btnCopy.BackColor = originalColor;
                btnCopy.Enabled = true;
            }
        }

        private void SetupPlaceholder()
        {
            richTextInput.Enter += (s, e) => RemovePlaceholder();
            richTextInput.Leave += (s, e) => AddPlaceholder();
            SetPlaceholder();
        }

        private void SetOutputPlaceholder()
        {
            richTextOutput.Text = OUTPUT_PLACEHOLDER;
            richTextOutput.ForeColor = Color.Gray;
        }

        private void SetPlaceholder()
        {
            richTextInput.Text = INPUT_PLACEHOLDER;
            richTextInput.ForeColor = Color.Gray;
            _isPlaceholderActive = true;
        }

        private void RemovePlaceholder()
        {
            if (_isPlaceholderActive)
            {
                richTextInput.Text = "";
                richTextInput.ForeColor = SystemColors.WindowText;
                _isPlaceholderActive = false;
            }
        }

        private void AddPlaceholder()
        {
            if (!_isPlaceholderActive && string.IsNullOrWhiteSpace(richTextInput.Text))
            {
                SetPlaceholder();
            }
        }
    }
}