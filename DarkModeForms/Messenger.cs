using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using static DarkModeForms.KeyValue;
using Timer = System.Windows.Forms.Timer;

namespace DarkModeForms
{
	/* Author: BlueMystic (bluemystic.play@gmail.com)  2024 */
	public static class Messenger
	{
		#region Events
		/// <summary>Manejador de Eventos para los Click en Botones</summary>
		private static Action<object, ValidateEventArgs> ValidateControlsHandler;

		/// <summary>Validates all Controls and allows to Cancel the changes.</summary>
		public static event Action<object, ValidateEventArgs> ValidateControls
		{
			add => ValidateControlsHandler += value;
			remove => ValidateControlsHandler -= value;
		}

		/// <summary>Previene multiples invocaciones entre llamadas a la misma instancia del evento</summary>
		private static void ResetEvents()
		{
			ValidateControlsHandler = null;
		}

		#endregion Events

		#region MessageBox

		private static MessageBoxDefaultButton _defaultButton = MessageBoxDefaultButton.Button1;

		public static DialogResult MessageBox(string Message)
			=> MessageBox(Message, string.Empty, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);

		/// <summary>Shows an Error Message.</summary>
		/// <param name="ex">an Exception error to show</param>
		/// <param name="ShowTrace"></param>
		/// <returns></returns>
		public static DialogResult MessageBox(Exception ex, bool ShowTrace = true) =>
			MessageBox(ex.Message + (ShowTrace ? "\r\n" + ex.StackTrace : ""), "Error!", icon: MessageBoxIcon.Error);

		/// <summary>Displays a message window, also known as a dialog box, which presents a message to the user.</summary>
		/// <param name="Message">The text to display in the message box.</param>
		/// <param name="title">The text to display in the title bar of the message box.</param>
		/// <param name="buttons">One of the MessageBoxButtons values that specifies which buttons to display in the message box.</param>
		/// <param name="icon">One of the 'Base64Icons.MsgIcon' values that specifies which icon to display in the message box.</param>
		/// <returns>It is a modal window, blocking other actions in the application until the user closes it.</returns>
		public static DialogResult MessageBox(
			string Message, string title, MessageBoxButtons buttons = MessageBoxButtons.OK,
			MessageBoxIcon icon = MessageBoxIcon.Information, bool pIsDarkMode = true)
		{
			//Debug.WriteLine(icon.ToString());

			MsgIcon Icon = MsgIcon.None;

			/*
			"..In current implementations there are ONLY four unique symbols with multiple values assigned to them."
			https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.messageboxicon?view=netframework-4.8
			*/
			switch (icon)
			{
				case MessageBoxIcon.Information: Icon = MsgIcon.Info; break;
				case MessageBoxIcon.Exclamation: Icon = MsgIcon.Warning; break;
				case MessageBoxIcon.Question: Icon = MsgIcon.Question; break;
				case MessageBoxIcon.Error: Icon = MsgIcon.Error; break;
				case MessageBoxIcon.None:
				default:
					break;
			}

			return MessageBox(Message, title, Icon, buttons, pIsDarkMode);
		}


		public static DialogResult MessageBox(string Message, string title, MessageBoxButtons buttons,
			MessageBoxIcon icon, MessageBoxDefaultButton DefaultButton, bool pIsDarkMode = true)
		{
			_defaultButton = DefaultButton;
			return MessageBox(Message, title, buttons, icon, pIsDarkMode);
		}

		public static DialogResult MessageBox(string Message, string title, MessageBoxButtons buttons = MessageBoxButtons.OK,
											  MsgIcon icon = MsgIcon.None, bool pIsDarkMode = true)
		{
			return MessageBox(Message, title, icon, buttons, pIsDarkMode);
		}

		public static DialogResult MessageBox(Form pOwner, string Message, string title, 
			MessageBoxButtons buttons, MsgIcon icon = MsgIcon.None, bool pIsDarkMode = true)
		{
			return MessageBox(Message, title, icon, buttons, pIsDarkMode, owner: pOwner);
		}

		/// <summary>Displays a message window, also known as a dialog box, which presents a message to the user.</summary>
		/// <param name="Message">The text to display in the message box.</param>
		/// <param name="title">The text to display in the title bar of the message box.</param>
		/// <param name="Icon">One of the 'Base64Icons.MsgIcon' values that specifies which icon to display in the message box.</param>
		/// <param name="buttons">One of the MessageBoxButtons values that specifies which buttons to display in the message box.</param>
		/// <returns>It is a modal window, blocking other actions in the application until the user closes it.</returns>
		public static DialogResult MessageBox(
			string Message, string title, MsgIcon Icon,
			MessageBoxButtons buttons = MessageBoxButtons.OK, bool pIsDarkMode = true,
			MessageBoxDefaultButton defaultButton = MessageBoxDefaultButton.Button1, Form owner = null)
		{
			Form form = new Form
			{
				FormBorderStyle = FormBorderStyle.FixedDialog,
				StartPosition = FormStartPosition.CenterParent,
				MaximizeBox = false,
				MinimizeBox = false,
				Text = title,
				Width = 340,
				Height = 170,
				KeyPreview = true, //<- allows the form to receive key events before they are passed to the controls
			};
			if (owner != null)
			{
				form.Owner = owner;
			}

			DarkModeCS DMode = new DarkModeCS(form)
			{ ColorMode = pIsDarkMode ? DarkModeCS.DisplayMode.DarkMode : DarkModeCS.DisplayMode.ClearMode };
			DMode.ApplyTheme(pIsDarkMode);

			Base64Icons _Icons = new Base64Icons();

			Font systemFont = SystemFonts.DefaultFont;
			int fontHeight = systemFont.Height;

			#region Bottom Panel & Buttons

			Panel bottomPanel = new Panel
			{
				Dock = DockStyle.Bottom,
				Height = 48,
				BackColor = DMode.OScolors.Surface,
				ForeColor = DMode.OScolors.TextActive
			};
			form.Controls.Add(bottomPanel);

			string CurrentLanguage = GetCurrentLanguage();
			var ButtonTranslations = GetButtonTranslations(CurrentLanguage); //<- "OK|Cancel|Yes|No|Continue|Retry|Abort|Ignore|Try Again"


			// Dialogs without Cancel should NOT have a Close (X) button in the Titlebar
			// Exception is first case, MessageBoxButtons.OK, which does have one  ;-)

			List<Button> CmdButtons = new List<Button>();
			switch (buttons)
			{
				case MessageBoxButtons.OK:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.OK,
						Text = ButtonTranslations["OK"],
						Height = fontHeight + 10,
						FlatStyle = FlatStyle.System
					});
					form.AcceptButton = CmdButtons[0];
					// Copy standard MessageBox behavior by closing the dialog window
					// and returning DialogResult.OK if the Escape key is pressed
					form.KeyPreview = true;
					form.KeyDown += (s, e) =>
					{ if (e.KeyCode == Keys.Escape) { form.Close(); } };
					// Handle the FormClosed event to return DialogResult.OK
					form.FormClosed += (s, e) =>
					{ form.DialogResult = DialogResult.OK; };
					break;

				case MessageBoxButtons.OKCancel:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.OK,
						Text = ButtonTranslations["OK"],
						Height = fontHeight + 10,
						FlatStyle = FlatStyle.System
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Cancel,
						Text = ButtonTranslations["Cancel"],
						FlatStyle = FlatStyle.System
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[1];
					break;

				case MessageBoxButtons.AbortRetryIgnore:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Abort,
						Text = ButtonTranslations["Abort"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Retry,
						Text = ButtonTranslations["Retry"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Ignore,
						Text = ButtonTranslations["Ignore"]
					});
					form.AcceptButton = CmdButtons[0];
					//form.CancelButton = CmdButtons[1];  // To match standard MessageBox behavior
					form.ControlBox = false;
					break;

				case MessageBoxButtons.YesNoCancel:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Yes,
						Text = ButtonTranslations["Yes"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.No,
						Text = ButtonTranslations["No"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Cancel,
						Text = ButtonTranslations["Cancel"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[2];
					break;

				case MessageBoxButtons.YesNo:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Yes,
						Text = ButtonTranslations["Yes"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.No,
						Text = ButtonTranslations["No"]
					});
					form.AcceptButton = CmdButtons[0];
					//form.CancelButton = CmdButtons[1];  // To match standard MessageBox behavior
					form.ControlBox = false;
					break;

				case MessageBoxButtons.RetryCancel:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Retry,
						Text = ButtonTranslations["Retry"],
						FlatStyle = FlatStyle.System
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Cancel,
						Text = ButtonTranslations["Cancel"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[1];
					break;

					/*case MessageBoxButtons.CancelTryContinue:
						CmdButtons.Add(new Button()
						{
							Anchor = AnchorStyles.Top | AnchorStyles.Right,
							DialogResult = DialogResult.Cancel,
							Text = ButtonTranslations["Cancel"]
						});
						CmdButtons.Add(new Button()
						{
							Anchor = AnchorStyles.Top | AnchorStyles.Right,
							DialogResult = DialogResult.TryAgain,
							Text = ButtonTranslations["Try Again"]
						});
						CmdButtons.Add(new Button()
						{
							Anchor = AnchorStyles.Top | AnchorStyles.Right,
							DialogResult = DialogResult.Continue,
							Text = ButtonTranslations["Continue"]
						});
						form.AcceptButton = CmdButtons[2];  // Not sure about this one...
						form.CancelButton = CmdButtons[0];  // But "Cancel" should be used here
						break;*/
			}

			int Padding = 4;
			int LastPos = form.ClientSize.Width;

			// Fall back to DefaultFont if MessageBoxFont is null
			systemFont = SystemFonts.MessageBoxFont ?? SystemFonts.DefaultFont;

			// Create the Graphics object once, not mutiple times in the for loop
			using (Graphics g = form.CreateGraphics())
			{
				// Add the buttons in reverse order, to match the order in a standard MessageBox
				for (int c = CmdButtons.Count - 1; c >= 0; c--)
				{
					Button _button = CmdButtons[c];
					_button.FlatAppearance.BorderColor = (form.AcceptButton == _button) ? DMode.OScolors.Accent : DMode.OScolors.Control;

					// Add the button to the Panel
					bottomPanel.Controls.Add(_button);

					// Set the button TabIndex order properly
					_button.TabIndex = c;

					_button.Font = systemFont;

					// Measure the width of the button text
					SizeF textSize = g.MeasureString(_button.Text, systemFont);
					_button.Size = new Size((int)textSize.Width + 20, systemFont.Height + 10); // Adding some padding

					_button.Location = new Point(LastPos - (_button.Width + Padding), (bottomPanel.Height - _button.Height) / 2);
					LastPos = _button.Left;
				}
			}

			// Select (focus) the default button
			int b = (int)_defaultButton;
			if (b > 0)
			{
				b >>= 8;
				if (b < CmdButtons.Count)
				{
					CmdButtons[b].Select();
					CmdButtons[b].FlatStyle = FlatStyle.Flat;
					CmdButtons[b].FlatAppearance.BorderColor = DMode.OScolors.AccentLight;
				}
			}

			#endregion Bottom Panel & Buttons

			#region Icon

			Rectangle picBox = new Rectangle(2, 10, 0, 0);
			if (Icon != MsgIcon.None)
			{
				PictureBox picIcon = new PictureBox { SizeMode = PictureBoxSizeMode.Zoom, Size = new Size(64, 64) };
				picIcon.Image = _Icons.GetIcon(Icon);
				form.Controls.Add(picIcon);

				picBox.Size = new Size(64, 64);
				picIcon.SetBounds(picBox.X, picBox.Y, picBox.Width, picBox.Height);
				picIcon.BringToFront();
			}

			#endregion Icon

			#region Prompt Text

			Label lblPrompt = new Label
			{
				Text = Message,
				AutoSize = true,
				//BackColor = Color.Fuchsia,
				ForeColor = DMode.OScolors.TextActive,
				TextAlign = ContentAlignment.MiddleLeft,  // Align left like standard Winforms MessageBox
				Location = new Point(picBox.X + picBox.Width + 4, picBox.Y),
				MaximumSize = new Size(form.ClientSize.Width - (picBox.X + picBox.Width) + 8, 0),
				MinimumSize = new Size(form.ClientSize.Width - (picBox.X + picBox.Width) + 8, 64)
			};
			lblPrompt.BringToFront();
			form.Controls.Add(lblPrompt);

			#endregion Prompt Text

			form.ClientSize = new Size(340,
				bottomPanel.Height +
				lblPrompt.Height +
				20
			);

			#region Keyboard Shortcuts

			string localMessage = Message;
			string localTitle = title;

			form.KeyDown += (object sender, KeyEventArgs e) =>
			{
				//- Keyboard shortcut for CTRL + C: Copy the message and title to the clipboard
				if (e.Control && e.KeyCode == Keys.C)
				{
					string clipboardText = $"Title: {localTitle}\r\nMessage: {localMessage}";
					Clipboard.SetText(clipboardText);
					e.Handled = true;
				}
			};

			#endregion


			return form.ShowDialog();
		}

		#endregion MessageBox

		#region InputBox

		/// <summary>Muestra un mensaje en un cuadro de diálogo, solicitando al usuario el ingreso de datos varios.</summary>
		/// <example>Modo de Uso del <see cref="InputBox"/> method.
		/// <code>
		///  List<KeyValue> props = new List<KeyValue>
		///  {
		///    new KeyValue("[FieldName]", "[Default Value]", KeyValue.ValueTypes.String),
		///  };
		/// if (Util.InputBox("[WindowTitle]", "[Prompt]", ref props, Base64Icons.MsgIcon.Edit) == DialogResult.OK)
		/// {
		///    Console.WriteLine(props[0].Value);
		/// }
		/// </code>
		/// </example>
		/// <param name="title">Expresión de tipo String que se muestra en la barra de título del cuadro de diálogo.</param>
		/// <param name="promptText">Expresión de tipo String que se muestra como mensaje en el cuadro de diálogo.</param>
		/// <param name="Fields">[REFERENCIA] Campos de tipo 'KeyValue' que el usuario puede cambiar.</param>
		/// <param name="Icon">Icon to Show in the lower left corner</param>
		/// <param name="buttons">[OPTIONAL] Texts for Buttons. Used for Translations. Default: 'OK|Cancel|Yes|No|Continue'</param>
		/// <returns>OK si el usuario acepta. By BlueMystic @2024</returns>
		public static DialogResult InputBox(
			string title, string promptText, ref List<KeyValue> Fields,
			MsgIcon Icon = 0, MessageBoxButtons buttons = MessageBoxButtons.OK, bool pIsDarkMode = true)
		{
			Form form = new Form
			{
				FormBorderStyle = FormBorderStyle.FixedDialog,
				StartPosition = FormStartPosition.CenterParent,
				MaximizeBox = false,
				MinimizeBox = false,
				Text = title,
				Width = 340,
				Height = 170
			};

			DarkModeCS DMode = new DarkModeCS(form) { ColorMode = pIsDarkMode ? DarkModeCS.DisplayMode.DarkMode : DarkModeCS.DisplayMode.ClearMode };
			DMode.ApplyTheme(pIsDarkMode);

			// Error Management & Icon Library:
			ErrorProvider Err = new ErrorProvider();
			Base64Icons _Icons = new Base64Icons();

			#region Bottom Panel

			Panel bottomPanel = new Panel
			{
				Dock = DockStyle.Bottom,
				Height = 48,
				BackColor = DMode.OScolors.Surface,
				ForeColor = DMode.OScolors.TextActive
			};
			form.Controls.Add(bottomPanel);

			#endregion Bottom Panel

			#region Icon

			if (Icon != MsgIcon.None)
			{
				PictureBox picIcon = new PictureBox { SizeMode = PictureBoxSizeMode.Zoom, Size = new Size(48, 48) };
				picIcon.Image = _Icons.GetIcon(Icon);
				bottomPanel.Controls.Add(picIcon);

				picIcon.SetBounds(0, 2, 48, 48);
				picIcon.BringToFront();
			}

			#endregion Icon

			#region Buttons

			string CurrentLanguage = GetCurrentLanguage();
			var ButtonTranslations = GetButtonTranslations(CurrentLanguage); //<- "OK|Cancel|Yes|No|Continue|Retry|Abort"

			List<Button> CmdButtons = new List<Button>();
			switch (buttons)
			{
				case MessageBoxButtons.OK:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.OK,
						Text = ButtonTranslations["OK"]
					});
					form.AcceptButton = CmdButtons[0];
					break;

				case MessageBoxButtons.OKCancel:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.OK,
						Text = ButtonTranslations["OK"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Cancel,
						Text = ButtonTranslations["Cancel"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[1];
					break;

				case MessageBoxButtons.AbortRetryIgnore:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Retry,
						Text = ButtonTranslations["Retry"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Abort,
						Text = ButtonTranslations["Abort"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[1];
					break;

				case MessageBoxButtons.YesNoCancel:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Yes,
						Text = ButtonTranslations["Yes"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.No,
						Text = ButtonTranslations["No"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Cancel,
						Text = ButtonTranslations["Cancel"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[2];
					break;

				case MessageBoxButtons.YesNo:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Yes,
						Text = ButtonTranslations["Yes"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.No,
						Text = ButtonTranslations["No"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[1];
					break;

				case MessageBoxButtons.RetryCancel:
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Retry,
						Text = ButtonTranslations["Retry"]
					});
					CmdButtons.Add(new Button
					{
						Anchor = AnchorStyles.Top | AnchorStyles.Right,
						DialogResult = DialogResult.Cancel,
						Text = ButtonTranslations["Cancel"]
					});
					form.AcceptButton = CmdButtons[0];
					form.CancelButton = CmdButtons[1];
					break;

					/*case MessageBoxButtons.CancelTryContinue:
						CmdButtons.Add(new Button()
						{
							Anchor = AnchorStyles.Top | AnchorStyles.Right,
							DialogResult = DialogResult.Cancel,
							Text = ButtonTranslations["Cancel"]
						});
						CmdButtons.Add(new Button()
						{
							Anchor = AnchorStyles.Top | AnchorStyles.Right,
							DialogResult = DialogResult.Continue,
							Text = ButtonTranslations["Continue"]
						});
						form.AcceptButton = CmdButtons[0];
						form.CancelButton = CmdButtons[1];
						break;*/
			}

			int Padding = 4;
			int LastPos = form.ClientSize.Width;

			foreach (var _button in CmdButtons)
			{
				_button.FlatAppearance.BorderColor = (form.AcceptButton == _button) ? DMode.OScolors.Accent : DMode.OScolors.Control;
				bottomPanel.Controls.Add(_button);

				_button.Location = new Point(LastPos - (_button.Width + Padding), (bottomPanel.Height - _button.Height) / 2);
				LastPos = _button.Left;

				//if (_button == form.AcceptButton)
				//{
				//_button.Click += (s, e) =>
				//{
				//  CancelEventArgs args = new CancelEventArgs();
				//  ValidateControls(null, args);

				//  //2.  If the Client cancelled the change, revert to the previous value:
				//  if (args.Cancel) {  }
				//};
				//}
			}

			#endregion Buttons

			#region Prompt Text

			Label lblPrompt = new Label();
			if (!string.IsNullOrWhiteSpace(promptText))
			{
				lblPrompt.Dock = DockStyle.Top;
				lblPrompt.Text = promptText; //Font = new Font(form.Font, FontStyle.Bold),
				lblPrompt.AutoSize = false;
				lblPrompt.Height = 24;
				lblPrompt.TextAlign = ContentAlignment.MiddleCenter;
			}
			else
			{
				lblPrompt.Location = new Point(0, 0);
				lblPrompt.Width = 0;
				lblPrompt.Height = 0;
			}
			form.Controls.Add(lblPrompt);

			#endregion Prompt Text

			#region Controls for KeyValues

			TableLayoutPanel Contenedor = new TableLayoutPanel
			{
				Size = new Size(form.ClientSize.Width - 20, 50),
				AutoSizeMode = AutoSizeMode.GrowAndShrink,
				BackColor = DMode.OScolors.Background,
				AutoSize = true,
				ColumnCount = 2,
				Location = new Point(10, lblPrompt.Location.Y + lblPrompt.Height + 4)
			};
			form.Controls.Add(Contenedor);
			Contenedor.ColumnStyles.Clear();
			Contenedor.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			Contenedor.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute));
			Contenedor.ColumnStyles[1].Width = form.ClientSize.Width - 120;
			Contenedor.RowStyles.Clear();

			int ChangeDelayMS = 1000; //<- Delay for Change event in Miliseconds
			int currentRow = 0;
			foreach (KeyValue field in Fields)
			{
				// Create Label and TextBox controls
				Label field_label = new Label
				{
					Text = field.Key,
					AutoSize = false,
					Dock = DockStyle.Fill,
					TextAlign = ContentAlignment.MiddleCenter
				};
				Control field_Control = null;

				BorderStyle BStyle = (DMode.IsDarkMode ? BorderStyle.FixedSingle : BorderStyle.Fixed3D);

				if (field.ValueType == ValueTypes.String)
				{
					field_Control = new TextBox
					{
						Text = field.Value,
						Dock = DockStyle.Fill,
						TextAlign = HorizontalAlignment.Center
					};
					((TextBox)field_Control).TextChanged += (sender, args) =>
					{
						AddTextChangedDelay((TextBox)field_Control, ChangeDelayMS, text =>
						{
							field.Value = ((TextBox)sender).Text;

							//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
							((TextBox)sender).Text = Convert.ToString(field.Value);
							Err.SetError(field_Control, field.ErrorText);
						});
					};
				}
				if (field.ValueType == ValueTypes.Multiline)
				{
					field_Control = new TextBox
					{
						Text = field.Value,
						Dock = DockStyle.Fill,
						TextAlign = HorizontalAlignment.Left,
						Multiline = true,
						ScrollBars = ScrollBars.Vertical
					};
					((TextBox)field_Control).TextChanged += (sender, args) =>
					{
						AddTextChangedDelay((TextBox)field_Control, ChangeDelayMS, text =>
						{
							field.Value = ((TextBox)sender).Text;

							//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
							((TextBox)sender).Text = Convert.ToString(field.Value);
							Err.SetError(field_Control, field.ErrorText);
						});
					};
				}
				if (field.ValueType == ValueTypes.Password)
				{
					field_Control = new TextBox
					{
						Text = field.Value,
						Dock = DockStyle.Fill,
						UseSystemPasswordChar = true,
						TextAlign = HorizontalAlignment.Center
					};
					((TextBox)field_Control).TextChanged += (sender, args) =>
					{
						AddTextChangedDelay((TextBox)field_Control, ChangeDelayMS, text =>
						{
							field.Value = ((TextBox)sender).Text;

							//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
							((TextBox)sender).Text = Convert.ToString(field.Value);
							Err.SetError(field_Control, field.ErrorText);
						});
					};
				}
				if (field.ValueType == ValueTypes.Integer)
				{
					field_Control = new NumericUpDown
					{
						Minimum = int.MinValue,
						Maximum = int.MaxValue,
						TextAlign = HorizontalAlignment.Center,
						Value = Convert.ToInt32(field.Value),
						ThousandsSeparator = true,
						Dock = DockStyle.Fill,
						DecimalPlaces = 0
					};
					((NumericUpDown)field_Control).ValueChanged += (sender, args) =>
					{
						AddTextChangedDelay((NumericUpDown)field_Control, ChangeDelayMS, text =>
						{
							field.Value = ((NumericUpDown)sender).Value.ToString();

							//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
							((NumericUpDown)sender).Value = Convert.ToInt32(field.Value);
							Err.SetError(field_Control, field.ErrorText);
						});
					};
				}
				if (field.ValueType == ValueTypes.Decimal)
				{
					field_Control = new NumericUpDown
					{
						Minimum = int.MinValue,
						Maximum = int.MaxValue,
						TextAlign = HorizontalAlignment.Center,
						Value = Convert.ToDecimal(field.Value),
						ThousandsSeparator = false,
						Dock = DockStyle.Fill,
						DecimalPlaces = 2
					};
					((NumericUpDown)field_Control).ValueChanged += (sender, args) =>
					{
						AddTextChangedDelay((NumericUpDown)field_Control, ChangeDelayMS, text =>
						{
							field.Value = ((NumericUpDown)sender).Value.ToString();

							//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
							((NumericUpDown)sender).Value = Convert.ToDecimal(field.Value);
							Err.SetError(field_Control, field.ErrorText);
						});
					};
				}
				if (field.ValueType == ValueTypes.Date)
				{
					field_Control = new DateTimePicker
					{
						Value = Convert.ToDateTime(field.Value),
						Dock = DockStyle.Fill,
						Format = DateTimePickerFormat.Short,

						CalendarForeColor = DMode.OScolors.TextActive,
						CalendarMonthBackground = DMode.OScolors.Control,
						CalendarTitleBackColor = DMode.OScolors.Surface,
						CalendarTitleForeColor = DMode.OScolors.TextActive
					};
					((DateTimePicker)field_Control).ValueChanged += (sender, args) =>
					{
						field.Value = ((DateTimePicker)sender).Value.ToString();
						//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
						((DateTimePicker)sender).Value = Convert.ToDateTime(field.Value);
						Err.SetError(field_Control, field.ErrorText);
						Err.SetIconAlignment(field_Control, ErrorIconAlignment.MiddleLeft);
					};
				}
				if (field.ValueType == ValueTypes.Time)
				{
					field_Control = new DateTimePicker
					{
						Value = Convert.ToDateTime(field.Value),
						Dock = DockStyle.Fill,
						Format = DateTimePickerFormat.Time
					};
					((DateTimePicker)field_Control).ValueChanged += (sender, args) =>
					{
						field.Value = ((DateTimePicker)sender).Value.ToString();
						//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo
						((DateTimePicker)sender).Value = Convert.ToDateTime(field.Value);
						Err.SetError(field_Control, field.ErrorText);
						Err.SetIconAlignment(field_Control, ErrorIconAlignment.MiddleLeft);
					};
				}
				if (field.ValueType == ValueTypes.Boolean)
				{
					field_Control = new CheckBox
					{
						Checked = Convert.ToBoolean(field.Value),
						Dock = DockStyle.Fill,
						Text = field.Key
					};
					((CheckBox)field_Control).CheckedChanged += (sender, args) =>
					{
						field.Value = ((CheckBox)sender).Checked.ToString();
						//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo

						((CheckBox)sender).Checked = Convert.ToBoolean(field.Value);
						Err.SetError(field_Control, field.ErrorText);
					};
				}
				if (field.ValueType == ValueTypes.Dynamic)
				{
					field_Control = new FlatComboBox
					{
						DataSource = field.DataSet,
						ValueMember = "Value",
						DisplayMember = "Key",
						Dock = DockStyle.Fill,
						BackColor = DMode.OScolors.Control,
						ButtonColor = DMode.OScolors.Surface,
						ForeColor = DMode.OScolors.TextActive,
						SelectedValue = field.Value,
						DropDownStyle = ComboBoxStyle.DropDownList,
						FlatStyle = (DMode.IsDarkMode ? FlatStyle.Flat : FlatStyle.Standard)
					};

					((ComboBox)field_Control).SelectedValueChanged += (sender, args) =>
					{
						field.Value = ((ComboBox)sender).SelectedValue.ToString();

						//aqui 'KeyValue' valida el nuevo valor y puede cancelarlo, o mostrar error
						((ComboBox)sender).SelectedValue = Convert.ToString(field.Value);
						Err.SetError(field_Control, field.ErrorText);
					};
				}

				// Add controls to appropriate cells:
				Contenedor.Controls.Add(field_label, 0, currentRow); // Column 0 for labels
				if (field.ValueType == ValueTypes.Multiline)
				{
					Contenedor.Controls.Add(field_Control, 1, currentRow);
					const int spanRow = 6;
					for (int i = 0; i < spanRow; i++)
					{
						currentRow++;
						Contenedor.RowCount++;
						Contenedor.RowStyles.Add(new RowStyle(SizeType.Absolute, field_Control.Height));
					}
					Contenedor.SetRowSpan(field_Control, spanRow);
				}
				else
				{
					Contenedor.Controls.Add(field_Control, 1, currentRow); // Column 1 for text boxes
				}

				Err.SetIconAlignment(field_Control, ErrorIconAlignment.MiddleLeft);

				//Fix for ComboBox Null SelectedValue:
				if (field_Control is ComboBox)
				{
					((ComboBox)field_Control).CreateControl();
					((ComboBox)field_Control).SelectedValue = field.Value;
				}

				field_Control.TabIndex = currentRow;

				// Increment row index for the next pair
				currentRow++;
			}

			Contenedor.Width = form.ClientSize.Width - 20;

			#endregion Controls for KeyValues

			form.ClientSize = new Size(340,
				bottomPanel.Height +
				lblPrompt.Height +
				Contenedor.Height +
				20
			);
			form.FormClosing += (sender, e) =>
			{
				//Control Validations
				if (form.ActiveControl == form.AcceptButton)
				{
					ValidateEventArgs cArgs = new ValidateEventArgs(null);

					ValidateControlsHandler?.Invoke(form, cArgs); //<- Dispara el Evento

					e.Cancel = cArgs.Cancel;
					if (!e.Cancel)
					{
						form.DialogResult = form.AcceptButton.DialogResult;
					}
					//ResetEvents(); //<- Previene multiples llamadas
				}
			};

			return form.ShowDialog();
		}

		#endregion InputBox

		#region Private Stuff

		private static Dictionary<Control, Timer> timers;

		private static void AddTextChangedDelay<TControl>(TControl control, int milliseconds, Action<TControl> action) where TControl : Control
		{
			if (timers == null)
			{
				timers = new Dictionary<Control, Timer>();
			}

			if (timers.ContainsKey(control))
			{
				timers[control].Stop();
				timers.Remove(control);
			}

			var timer = new Timer();
			timer.Interval = milliseconds;
			timer.Tick += (sender, e) =>
			{
				timer.Stop();
				timers.Remove(control);
				action(control);
			};
			timer.Start();
			timers.Add(control, timer);
		}

		/// <summary>Returns the Current Language ID of the PC.</summary>
		/// <param name="pDefault">Default to return if Current lang is not supported.</param>
		public static string GetCurrentLanguage(string pDefault = "en")
		{
			string _ret = pDefault;
			string CurrentLanguage = CultureInfo.CurrentCulture.TwoLetterISOLanguageName;
			if (IsCurrentLanguageSupported(new List<string> { "en", "es", "fr", "de", "ru", "ko", "pt" }, CurrentLanguage))
			{
				_ret = CurrentLanguage;
			}
			if (CurrentLanguage.ToLowerInvariant().Equals("zh"))
			{
				var LangVariable = CultureInfo.CurrentCulture.Name;
				if (string.Equals(LangVariable, "zh-CN") || string.Equals(LangVariable, "zh-SG") || string.Equals(LangVariable, "zh-Hans"))
				{
					_ret = "zh-Hans";
				}
				else if (string.Equals(LangVariable, "zh-TW") || string.Equals(LangVariable, "zh-HK") || string.Equals(LangVariable, "zh-MO") || string.Equals(LangVariable, "zh-Hant"))
				{
					_ret = "zh-Hant";
				}
				else
				{
					_ret = "zh-Hans";
				}
			}
			return _ret;
		}

		/// <summary>Return the Translations for the desired language (if supported).</summary>
		/// <param name="pLanguage">Supported Languages: [en, es, fr, de, ru, ko, pt]</param>
		/// <returns>Keys: OK, Cancel, Yes, No, Continue, Retry, Abort, Ignore, Try Again</returns>
		private static Dictionary<string, string> GetButtonTranslations(string pLanguage)
		{
			Dictionary<string, string> _ret = null;

			Dictionary<string, string> ButtonTranslations = new Dictionary<string, string> {
				{ "en", "OK|Cancel|Yes|No|Continue|Retry|Abort|Ignore|Try Again" },
				{ "es", "Aceptar|Cancelar|Sí|No|Continuar|Reintentar|Abortar|Ignorar|Intentar" },
				{ "fr", "Accepter|Annuler|Oui|Non|Continuer|Réessayer|Abandonner|Ignorer|Essayer" },
				{ "de", "Akzeptieren|Abbrechen|Ja|Nein|Weiter|Wiederholen|Abbrechen|Ignorieren|Versuchen" },
				{ "ru", "Принять|Отменить|Да|Нет|Продолжить|Повторить|Прервать|Игнорировать|Пытаться" },
				{ "ko", "확인|취소|예|아니오|계속|다시 시도|중단|무시|써 보다" },
				{ "pt", "Aceitar|Cancelar|Sim|Não|Continuar|Tentar novamente|Abortar|Ignorar|Tentar" },
				{ "zh-Hans", "确定|取消|是|否|继续|重试|中止|忽略|尝试" },
				{ "zh-Hant", "確定|取消|是|否|繼續|重試|中止|忽略|嘗試" }
				/* Add here you own language button translations */
			  };

			string raw = ButtonTranslations[pLanguage];
			if (!string.IsNullOrEmpty(raw))
			{
				var Words = raw.Split('|').ToList();

				_ret = new Dictionary<string, string> {
					{ "OK", Words[0] },
					{ "Cancel", Words[1] },
					{ "Yes", Words[2] },
					{ "No", Words[3] },
					{ "Continue", Words[4] },
					{ "Retry", Words[5] },
					{ "Abort", Words[6] },
					{ "Ignore", Words[7] },
					{ "Try Again", Words[8] }
				};
			}

			return _ret;
		}

		private static bool IsCurrentLanguageSupported(List<string> languages, string currentLanguage)
		{
			if (languages == null || currentLanguage == null)
			{
				throw new ArgumentNullException(languages == null ? nameof(languages) : nameof(currentLanguage));
			}

			// Convert both languages to lowercase for case-insensitive comparison
			currentLanguage = currentLanguage.ToLowerInvariant();

			// Check if the current language is directly present in the list
			if (languages.Contains(currentLanguage))
			{
				return true;
			}

			// Handle alternative language codes (e.g., "pt-BR" for Brazilian Portuguese)
			if (currentLanguage.Length >= 2)
			{
				string baseLanguage = currentLanguage.Substring(0, 2);
				return languages.Contains(baseLanguage);
			}

			return false;
		}

		#endregion Private Stuff
	}

	/// <summary>Constants for the Default Icons.</summary>
	public enum MsgIcon
	{
		None = 0,
		Info,
		Success,
		Warning,
		Error,
		Question,
		Lock,
		User,
		Forbidden,
		AddNew,
		Cancel,
		Edit,
		List
	}

	/// <summary>Stores Data for Dynamic Fields on the InputBox Dialog.</summary>
	public class KeyValue
	{
		#region Private Members

		private string _value;

		#endregion Private Members

		#region Contructors

		public KeyValue()
		{
		}

		public KeyValue(string pKey, string pValue, ValueTypes pType = 0, List<KeyValue> pDataSet = null)
		{
			Key = pKey;
			Value = pValue;
			ValueType = pType;
			DataSet = pDataSet;
		}

		#endregion Contructors

		#region Public Properties

		/// <summary>Types of Data acepted by this class.</summary>
		public enum ValueTypes
		{
			String = 0,
			Integer = 1,
			Decimal = 2,
			Date = 3,
			Time,
			Boolean,
			Dynamic,
			Password,
			Multiline
		}

		public string Key { get; set; }

		public string Value
		{
			get => _value;
			set
			{
				var newValue = value;

				//1. We Raise the 'Validate' Event to the Client informing both the
				//   New and Old Values for Client Side Validation:
				OnValidate(ref newValue); //<- Validate can Cancel the new Value

				if (_value != newValue)
				{
					_value = newValue;
				}
			}
		}

		/// <summary>Tipo de datos para el Control.</summary>
		public ValueTypes ValueType { get; set; } = ValueTypes.String;

		/// <summary>[OPTIONAL] Data for when 'ValueType' is 'Dynamic'.</summary>
		public List<KeyValue> DataSet { get; set; }

		/// <summary>[OPTIONAL] If this is not Empty, an Error icon will show next to the control.</summary>
		public string ErrorText { get; set; } = string.Empty;

		#endregion Public Properties

		#region Public Events

		public class ValidateEventArgs : EventArgs
		{
			public ValidateEventArgs(string newValue)
			{
				NewValue = newValue;
				Cancel = false;
			}

			public string NewValue { get; }
			public string OldValue { get; set; }

			public bool Cancel { get; set; }
			public string ErrorText { get; set; } = string.Empty;
		}

		/// <summary>Permite Validar el Valor del Control:
		/// <para>- Puede Cancelar el Cambio.</para>
		/// <para>- Puede Mostrar un Mensaje de error.</para>
		/// </summary>
		public event EventHandler<ValidateEventArgs> Validate;

		protected virtual void OnValidate(ref string newValue)
		{
			var validateHandler = Validate;
			if (validateHandler != null)
			{
				//1. We Raise the 'Validate' Event to the Client informing both the
				//   New and Old Values for Client Side Validation:
				var args = new ValidateEventArgs(newValue) { OldValue = _value };
				validateHandler(this, args);

				//2.  If the Client cancelled the change, revert to the previous value:
				if (args.Cancel) { newValue = _value; }

				//3. The Client may chose to show an Error Text:
				ErrorText = args.ErrorText;
			}
		}

		#endregion Public Events

		#region Public Methods

		public override string ToString()
		{
			return string.Format("{0} - {1}", Key, Value);
		}

		#endregion Public Methods
	}
}
