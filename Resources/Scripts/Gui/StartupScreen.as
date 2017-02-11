/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "DropDownList.as"
#include "MessageBox.as"
#include "UpdateCheckView.as"

namespace spades {


	class StartupScreenUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		FontManager@ fontManager;
		StartupScreenHelper@ helper;

		spades::ui::UIManager@ manager;

		StartupScreenMainMenu@ mainMenu;

		bool shouldExit = false;

		StartupScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, FontManager@ fontManager, StartupScreenHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.fontManager = fontManager;
			@this.helper = helper;

			SetupRenderer();

			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = fontManager.GuiFont;
			Init();
		}

		private void Init() {
			@mainMenu = StartupScreenMainMenu(this);
			mainMenu.Bounds = manager.RootElement.Bounds;
			manager.RootElement.AddChild(mainMenu);
		}

		void Reload() {
			// Delete StartupScreenMainMenu
			@manager.RootElement.GetChildren()[0].Parent = null;

			// Reload entire the startup screen while preserving the state as much as possible
			auto@ state = mainMenu.GetState();
			Init();
			mainMenu.SetState(state);
		}

		void SetupRenderer() {
			if(manager !is null)
				manager.KeyPanic();
		}

		void MouseEvent(float x, float y) {
			manager.MouseEvent(x, y);
		}

		void WheelEvent(float x, float y) {
			manager.WheelEvent(x, y);
		}

		void KeyEvent(string key, bool down) {
			manager.KeyEvent(key, down);
		}

		void TextInputEvent(string text) {
			manager.TextInputEvent(text);
		}

		void TextEditingEvent(string text, int start, int len) {
			manager.TextEditingEvent(text, start, len);
		}

		bool AcceptsTextInput() {
			return manager.AcceptsTextInput;
		}

		AABB2 GetTextInputRect() {
			return manager.TextInputRect;
		}

		void RunFrame(float dt) {
			renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 1.f);
			renderer.DrawImage(renderer.RegisterImage("Gfx/White.tga"),
				AABB2(0.f, 0.f, renderer.ScreenWidth, renderer.ScreenHeight));

			// draw title logo
			Image@ img = renderer.RegisterImage("Gfx/Title/LogoSmall.png");
			renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 1.f);
			renderer.DrawImage(img, AABB2(10.f, 10.f, img.Width, img.Height));

			manager.RunFrame(dt);
			manager.Render();

			renderer.FrameDone();
			renderer.Flip();
		}

		void Closing() {
			shouldExit = true;
		}

		bool WantsToBeClosed() {
			return shouldExit;
		}
	}

	class StartupScreenMainMenuState {
		int ActiveTabIndex;
	}

	class StartupScreenMainMenu: spades::ui::UIElement {

		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;

		spades::ui::ListView@ serverList;
		spades::ui::CheckBox@ bypassStartupWindowCheck;

		StartupScreenGraphicsTab@ graphicsTab;
		StartupScreenAudioTab@ audioTab;
		StartupScreenGenericTab@ genericTab;
		StartupScreenSystemInfoTab@ systemInfoTab;
		StartupScreenAdvancedTab@ advancedTab;

		private ConfigItem cl_showStartupWindow("cl_showStartupWindow", "-1");
		private bool advancedTabVisible = false;

		StartupScreenMainMenu(StartupScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;

			@this.Font = ui.fontManager.GuiFont;

			float width = Manager.Renderer.ScreenWidth;
			float height = Manager.Renderer.ScreenHeight;
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("StartupScreen", "Start");
				button.Bounds = AABB2(width - 170.f, 20.f, 150.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnStartPressed);
				AddChild(button);
			}
			{
				spades::ui::CheckBox button(Manager);
				button.Caption = _Tr("StartupScreen", "Skip this screen next time");
				button.Bounds = AABB2(360.f, 62.f, width - 380.f, 20.f); // note: this is updated later soon
				AddChild(button);
				@bypassStartupWindowCheck = button;
				@button.Activated = spades::ui::EventHandler(this.OnBypassStartupWindowCheckChanged);
			}
			{
				UpdateCheckView view(Manager, ui.helper.PackageUpdateManager);
				view.Bounds = AABB2(0.f, height - 40.f, width, 40.f);
				@view.OpenUpdateInfoURL = spades::ui::EventHandler(this.OnShowUpdateDetailsPressed);
				AddChild(view);
			}

			AABB2 clientArea(10.f, 100.f, width - 20.f, height - 150.f);
			StartupScreenGraphicsTab graphicsTab(ui, clientArea.max - clientArea.min);
			StartupScreenAudioTab audioTab(ui, clientArea.max - clientArea.min);
			StartupScreenGenericTab genericTab(ui, clientArea.max - clientArea.min);
			StartupScreenSystemInfoTab profileTab(ui, clientArea.max - clientArea.min);
			StartupScreenAdvancedTab advancedTab(ui, clientArea.max - clientArea.min);
			graphicsTab.Bounds = clientArea;
			audioTab.Bounds = clientArea;
			genericTab.Bounds = clientArea;
			profileTab.Bounds = clientArea;
			advancedTab.Bounds = clientArea;
			AddChild(graphicsTab);
			AddChild(audioTab);
			AddChild(genericTab);
			AddChild(profileTab);
			AddChild(advancedTab);
			audioTab.Visible = false;
			profileTab.Visible = false;
			genericTab.Visible = false;
			advancedTab.Visible = false;

			@this.graphicsTab = graphicsTab;
			@this.audioTab = audioTab;
			@this.advancedTab = advancedTab;
			@this.systemInfoTab = profileTab;
			@this.genericTab = genericTab;

			{
				spades::ui::SimpleTabStrip tabStrip(Manager);
				AddChild(tabStrip);
				tabStrip.Bounds = AABB2(10.f, 70.f, width - 20.f, 24.f);
				tabStrip.AddItem(_Tr("StartupScreen", "Graphics"), graphicsTab);
				tabStrip.AddItem(_Tr("StartupScreen", "Audio"), audioTab);
				tabStrip.AddItem(_Tr("StartupScreen", "Generic"), genericTab);
				tabStrip.AddItem(_Tr("StartupScreen", "System Info"), profileTab);
				tabStrip.AddItem(_Tr("StartupScreen", "Advanced"), advancedTab);
				@tabStrip.Changed = spades::ui::EventHandler(this.OnTabChanged);

				// Reposition the "Skip this screen next time" check box
				spades::ui::UIElement@[]@ tabStripItems = tabStrip.GetChildren();
				float right = tabStripItems[tabStripItems.length - 1].Bounds.max.x +
					tabStrip.Bounds.min.x + 10.f;
				bypassStartupWindowCheck.Bounds = AABB2(right, 62.f, width - right - 20.f, 20.f);
			}

			LoadConfig();
		}

		private void OnTabChanged(spades::ui::UIElement@) {
			bool v = advancedTab.Visible;
			if(advancedTabVisible and not v) {
				LoadConfig();
			}
			advancedTabVisible = v;
		}

		void LoadConfig() {
			switch(cl_showStartupWindow.IntValue) {
			case -1:
				bypassStartupWindowCheck.Toggled = false;
				break;
			case 0:
				bypassStartupWindowCheck.Toggled = true;
				break;
			default:
				bypassStartupWindowCheck.Toggled = false;
				break;
			}

			this.graphicsTab.LoadConfig();
			this.audioTab.LoadConfig();
			this.genericTab.LoadConfig();
			this.advancedTab.LoadConfig();
		}

		StartupScreenMainMenuState@ GetState() {
			StartupScreenMainMenuState state;
			if (this.graphicsTab.Visible) {
				state.ActiveTabIndex = 0;
			} else if (this.audioTab.Visible) {
				state.ActiveTabIndex = 1;
			} else if (this.genericTab.Visible) {
				state.ActiveTabIndex = 2;
			} else if (this.systemInfoTab.Visible) {
				state.ActiveTabIndex = 3;
			} else if (this.advancedTab.Visible) {
				state.ActiveTabIndex = 4;
			}
			return state;
		}

		void SetState(StartupScreenMainMenuState@ state) {
			this.graphicsTab.Visible = state.ActiveTabIndex == 0;
			this.audioTab.Visible = state.ActiveTabIndex == 1;
			this.genericTab.Visible = state.ActiveTabIndex == 2;
			this.systemInfoTab.Visible = state.ActiveTabIndex == 3;
			this.advancedTab.Visible = state.ActiveTabIndex == 4;
		}

		private void OnBypassStartupWindowCheckChanged(spades::ui::UIElement@ sender) {
			cl_showStartupWindow.IntValue = (bypassStartupWindowCheck.Toggled ? 0 : 1);
		}

		private void OnShowUpdateDetailsPressed(spades::ui::UIElement@ sender) {
			if (ui.helper.OpenUpdateInfoURL()) {
				return;
			}

			string msg = _Tr("StartupScreen", "An unknown error has occurred while opening the update info website.");
			msg += "\n\n" + ui.helper.PackageUpdateManager.LatestVersionInfoPageURL;
			AlertScreen al(Parent, msg, 100.f);
			al.Run();
		}

		private void OnQuitPressed(spades::ui::UIElement@ sender) {
			ui.shouldExit = true;
		}

		private void OnSetupPressed(spades::ui::UIElement@ sender) {
			PreferenceView al(this, PreferenceViewOptions(), ui.fontManager);
			al.Run();
		}

		private void Start() {
			helper.Start();
			ui.shouldExit = true; // we have to exit from the startup screen to start the game
		}

		private void OnStartPressed(spades::ui::UIElement@ sender) {
			Start();
		}

		void HotKey(string key) {
			if(IsEnabled and key == "Enter") {
				Start();
			} else if(IsEnabled and key == "Escape") {
				ui.shouldExit = true;
			} else {
				UIElement::HotKey(key);
			}
		}

		void Render() {
			UIElement::Render();

		}
	}

	funcdef void HelpTextHandler(string text);
	class ChainedEventHandler {
		spades::ui::EventHandler@ h;
		spades::ui::EventHandler@ h2;
		ChainedEventHandler(spades::ui::EventHandler@ h, spades::ui::EventHandler@ h2) {
			@this.h = h;
			@this.h2 = h2;
		}
		void Handler(spades::ui::UIElement@ e) {
			h(e); h2(e);
		}
	}
	class HelpHandler {
		private HelpTextHandler@ handler;
		private spades::ui::TextViewer@ helpView;
		private string text;
		HelpHandler(spades::ui::TextViewer@ helpView, string text) {
			this.text = text;
			@this.helpView = helpView;
		}
		HelpHandler(HelpTextHandler@ handler, string text) {
			this.text = text;
			@this.handler = handler;
		}
		private void OnMouseHover(spades::ui::UIElement@ elm) {
			if(helpView !is null) {
				helpView.Text = text;
			}
			if(handler !is null) {
				handler(text);
			}
		}
		void Watch(spades::ui::UIElement@ elm) {
			@elm.MouseEntered = spades::ui::EventHandler(this.OnMouseHover);
		}
		void WatchDeep(spades::ui::UIElement@ elm) {
			if(elm.MouseEntered !is null) {
				ChainedEventHandler chain(elm.MouseEntered, spades::ui::EventHandler(this.OnMouseHover));
				@elm.MouseEntered = spades::ui::EventHandler(chain.Handler);
			}else{
				Watch(elm);
			}
			spades::ui::UIElementIterator it(elm);
			while(it.MoveNext()) {
				WatchDeep(it.Current);
			}
		}
	}

	mixin class LabelAddable {

		private void AddLabel(float x, float y, float h, string text) {
			spades::ui::Label label(Manager);
			Font@ font = ui.fontManager.GuiFont;
			Vector2 siz = font.Measure(text);
			label.Text = text;
			label.Alignment = Vector2(0.f, 0.5f);
			label.Bounds = AABB2(x, y, siz.x, h);
			AddChild(label);
		}

	}

	class StartupScreenConfigViewModel: spades::ui::ListViewModel {
		spades::ui::UIElement@[] items;
		spades::ui::UIElement@[]@ items2;
		StartupScreenConfigViewModel() {
		}
		int NumRows {
			get {
				if(items2 !is null)
					return int(items2.length);
				return int(items.length);
			}
		}

		void Filter(string text) {
			if(text.length == 0) {
				@items2 = null;
				return;
			}
			spades::ui::UIElement@[] newItems;
			for(uint i = 0, count = items.length; i < count; i++) {
				StartupScreenConfigItemEditor@ editor =
					cast<StartupScreenConfigItemEditor>(items[i]);
				if(editor is null) continue;
				string label = editor.GetLabel();
				if(StringContainsCaseInsensitive(label, text)) {
					newItems.insertLast(items[i]);
				}
			}
			@items2 = newItems;
		}
		spades::ui::UIElement@ CreateElement(int row) {
			if(items2 !is null)
				return items2[row];
			return items[row];
		}
		void RecycleElement(spades::ui::UIElement@ elem) {
		}
	}

	interface StartupScreenGenericConfig {
		string GetValue();
		void SetValue(string);
		/** Returns an empty string when there's no problem. */
		string CheckValueCapability(string);
	}

	class StartupScreenConfig: StartupScreenGenericConfig {
		private StartupScreenUI@ ui;
		private ConfigItem@ cfg;
		private string cfgName;
		StartupScreenConfig(StartupScreenUI@ ui, string cfg) {
			@this.ui = ui;
			@this.cfg = ConfigItem(cfg);
			cfgName = cfg;
		}
		string GetValue() {
			return cfg.StringValue;
		}
		void SetValue(string v) {
			cfg.StringValue = v;
		}
		string CheckValueCapability(string v) {
			return ui.helper.CheckConfigCapability(cfgName, v);
		}
	}

	class StartupScreenConfigSetter {
		StartupScreenGenericConfig@ c;
		string value;
		StartupScreenConfigSetter(StartupScreenGenericConfig@ c, string value) {
			@this.c = c;
			this.value = value;
		}
		void Set(spades::ui::UIElement@) {
			c.SetValue(this.value);
		}
	}

	interface StartupScreenConfigItemEditor {
		void LoadConfig();
		StartupScreenGenericConfig@ GetConfig();
		void SetHelpTextHandler(HelpTextHandler@);
		string GetLabel();
	}


	class StartupScreenConfigSelectItemEditor: spades::ui::UIElement, LabelAddable, StartupScreenConfigItemEditor, StartupScreenConfigItemEditor {

		protected StartupScreenUI@ ui;
		private string[]@ descs;
		private string[]@ values;
		protected StartupScreenGenericConfig@ config;
		protected spades::ui::RadioButton@[] buttons;
		private string label;

		StartupScreenConfigSelectItemEditor(StartupScreenUI@ ui,
			StartupScreenGenericConfig@ cfg,
			string values, string descs) {
			super(ui.manager);
			@this.ui = ui;
			@this.descs = descs.split("|");
			@config = cfg;
			@this.values = values.split("|");

			{
				string desc = this.descs[0];
				int idx = desc.findFirst(":");
				if(idx >= 0) {
					desc = desc.substr(0, idx);
				}
				AddLabel(0, 0, 24.f, desc);
				this.label = desc;
			}


			for(uint i = 0; i < this.values.length; i++) {
				spades::ui::RadioButton b(Manager);
				string desc = this.descs[i + 1];
				int idx = desc.findFirst(":");
				if(idx >= 0) {
					desc = desc.substr(0, idx);
				}
				b.Caption = desc;

				b.GroupName = "hoge";
				StartupScreenConfigSetter setter(config, this.values[i]);
				@b.Activated = spades::ui::EventHandler(setter.Set);
				buttons.insertLast(b);
				this.AddChild(b);
			}
		}

		string GetLabel() {
			return label;
		}

		void LoadConfig() {
			string val = config.GetValue();
			for(uint i = 0, count = values.length; i < count; i++) {
				buttons[i].Toggled = (values[i] == val);
				buttons[i].Enable = CheckValueCapability(values[i]).length == 0;
			}
		}
		string CheckValueCapability(string v) {
			return config.CheckValueCapability(v);
		}
		StartupScreenGenericConfig@ GetConfig() {
			return config;
		}
		void SetHelpTextHandler(HelpTextHandler@ handler) {
			for(uint i = 0, count = values.length; i < count; i++) {
				string desc = descs[i + 1];
				int idx = desc.findFirst(":");
				if(idx < 0) {
					desc = descs[0];
					idx = desc.findFirst(":");
				}
				desc = desc.substr(uint(idx + 1));
				HelpHandler(handler, desc).Watch(buttons[i]);
			}
		}

		void set_Bounds(AABB2 v) {
			UIElement::set_Bounds(v);
			Vector2 size = this.Size;
			float h = 24.f;
			float x = size.x;
			for(uint i = buttons.length; i > 0; i--) {
				spades::ui::RadioButton@ b = buttons[i - 1];
				float w = ui.fontManager.GuiFont.Measure(b.Caption).x + 26.f;
				x -= w + 2.f;
				b.Bounds = AABB2(x, 0.f, w, h);
			}
		}
	}

	class StartupScreenConfigCheckItemEditor: spades::ui::UIElement, StartupScreenConfigItemEditor {

		protected StartupScreenUI@ ui;
		private string desc;
		private string valueOff;
		private string valueOn;
		private StartupScreenGenericConfig@ config;
		private spades::ui::CheckBox@ button;
		private string label;

		StartupScreenConfigCheckItemEditor(StartupScreenUI@ ui,
			StartupScreenGenericConfig@ cfg,
			string valueOff, string valueOn, string label, string desc) {
			super(ui.manager);
			@this.ui = ui;
			@config = cfg;
			this.valueOff = valueOff;
			this.valueOn = valueOn;
			this.desc = desc;

			spades::ui::CheckBox b(Manager);
			b.Caption = label;
			this.label = label;
			@b.Activated = spades::ui::EventHandler(this.StateChanged);
			@button = b;
			this.AddChild(b);
		}

		string GetLabel() {
			return label;
		}

		void LoadConfig() {
			string val = config.GetValue();
			button.Toggled = (val != valueOff);
			button.Enable = CheckValueCapability(valueOn).length == 0;
		}
		string CheckValueCapability(string v) {
			return config.CheckValueCapability(v);
		}
		StartupScreenGenericConfig@ GetConfig() {
			return config;
		}
		private void StateChanged(spades::ui::UIElement@) {
			config.SetValue(button.Toggled ? valueOn : valueOff);
		}
		void SetHelpTextHandler(HelpTextHandler@ handler) {
			HelpHandler(handler, desc).Watch(button);
		}

		void set_Bounds(AABB2 v) {
			UIElement::set_Bounds(v);
			Vector2 size = this.Size;
			float h = 24.f;
			button.Bounds = AABB2(0.f, 0.f, size.x, h);
		}
	}

	class StartupScreenConfigSliderItemEditor: spades::ui::UIElement, StartupScreenConfigItemEditor, LabelAddable {

		private StartupScreenUI@ ui;
		private string desc;
		private double stepSize;
		private StartupScreenGenericConfig@ config;
		private spades::ui::Slider@ slider;
		private spades::ui::Label@ valueLabel;
		private ConfigNumberFormatter@ formatter;
		private string label;

		StartupScreenConfigSliderItemEditor(StartupScreenUI@ ui,
			StartupScreenGenericConfig@ cfg,
			double minValue, double maxValue, double stepSize, string label, string desc,
			ConfigNumberFormatter@ formatter) {
			super(ui.manager);
			@this.ui = ui;
			@config = cfg;
			this.desc = desc;
			this.stepSize = stepSize;
			this.label = label;

			AddLabel(0, 0, 24.f, label);

			spades::ui::Slider b(Manager);
			b.MinValue = minValue;
			b.MaxValue = maxValue;

			// compute large change
			int steps = int((maxValue - minValue) / stepSize);
			steps = (steps + 9) / 10;
			b.LargeChange = float(steps) * stepSize;
			b.SmallChange = stepSize;

			@b.Changed = spades::ui::EventHandler(this.StateChanged);
			@slider = b;
			this.AddChild(b);

			spades::ui::Label l(Manager);
			@valueLabel = l;
			l.Alignment = Vector2(1.f, 0.5f);
			this.AddChild(l);

			@this.formatter = formatter;
		}

		private void UpdateLabel() {
			double v = slider.Value;
			string s = formatter.Format(v);
			valueLabel.Text = s;
		}

		string GetLabel() {
			return label;
		}

		void LoadConfig() {
			string val = config.GetValue();
			double v = ParseDouble(val);
			// don't use `ScrollTo` here; this calls `Changed` so
			// out-of-range value will be clamped and saved back
			slider.Value = Clamp(v, slider.MinValue, slider.MaxValue);
			// FIXME: button.Enable = CheckValueCapability(valueOn).length == 0;
			UpdateLabel();
		}
		string CheckValueCapability(string v) {
			return ""; //FIXME: config.CheckValueCapability(v);
		}
		StartupScreenGenericConfig@ GetConfig() {
			return config;
		}

		void DoRounding() {
			double v = double(slider.Value - slider.MinValue);
			v = floor((v / stepSize) + 0.5) * stepSize;
			v += double(slider.MinValue);
			slider.Value = v;
		}
		private void StateChanged(spades::ui::UIElement@) {
			DoRounding();
			config.SetValue(ToString(slider.Value));
			UpdateLabel();
		}
		void SetHelpTextHandler(HelpTextHandler@ handler) {
			HelpHandler(handler, desc).WatchDeep(slider);
		}

		void set_Bounds(AABB2 v) {
			UIElement::set_Bounds(v);
			Vector2 size = this.Size;
			float h = 24.f;
			slider.Bounds = AABB2(100.f, 2.f, size.x - 180.f, h - 4.f);
			valueLabel.Bounds = AABB2(size.x - 80.f, 0.f, 80.f, h);
		}
	}


	class StartupScreenConfigFieldItemEditor: spades::ui::UIElement, StartupScreenConfigItemEditor, LabelAddable {

		private StartupScreenUI@ ui;
		private string desc;
		private double stepSize;
		private StartupScreenGenericConfig@ config;
		private spades::ui::Field@ field;
		private string label;

		StartupScreenConfigFieldItemEditor(StartupScreenUI@ ui,
			StartupScreenGenericConfig@ cfg,
			string label, string desc) {
			super(ui.manager);
			@this.ui = ui;
			@config = cfg;
			this.desc = desc;
			this.label = label;

			AddLabel(0, 0, 24.f, label);

			spades::ui::Field b(Manager);

			@b.Changed = spades::ui::EventHandler(this.StateChanged);
			@field = b;
			this.AddChild(b);

		}

		string GetLabel() {
			return label;
		}

		void LoadConfig() {
			string val = config.GetValue();
			field.Text = val;
		}
		string CheckValueCapability(string v) {
			return config.CheckValueCapability(v);
		}
		StartupScreenGenericConfig@ GetConfig() {
			return config;
		}

		private void StateChanged(spades::ui::UIElement@) {
			config.SetValue(field.Text);
		}
		void SetHelpTextHandler(HelpTextHandler@ handler) {
			HelpHandler(handler, desc).WatchDeep(field);
		}

		void set_Bounds(AABB2 v) {
			UIElement::set_Bounds(v);
			Vector2 size = this.Size;
			float h = 24.f;
			field.Bounds = AABB2(240.f, 0.f, size.x - 240.f, h);
		}
	}

	// Maps multiple configs to {"0", "1", ...} or "custom"
	class StartupScreenComplexConfig: StartupScreenGenericConfig {
		StartupScreenConfigItemEditor@[] editors;
		StartupScreenComplexConfigPreset@[] presets;

		void AddEditor(StartupScreenConfigItemEditor@ editor) {
			Assert(presets.length == 0);
			editors.insertLast(editor);
		}
		void AddPreset(StartupScreenComplexConfigPreset@ preset) {
			Assert(preset.Values.length == editors.length);
			presets.insertLast(preset);
		}

		string GetValue() {
			string[] values;

			for(uint i = 0; i < editors.length; i++) {
				values.insertLast(editors[i].GetConfig().GetValue());
			}
			for(uint i = 0; i < presets.length; i++) {
				string[]@ pval = presets[i].Values;
				uint j = 0;
				uint jc = pval.length;
				for(; j < jc; j++) {
					if(values[j] != pval[j]) {
						break;
					}
				}
				if(j == jc) {
					return ToString(int(i));
				}
			}
			return "custom";
		}
		void SetValue(string v) {
			if(v == "custom") {
				return;
			}
			uint pId = uint(ParseInt(v));
			string[]@ pval = presets[pId].Values;
			for(uint i = 0; i < pval.length; i++) {
				editors[i].GetConfig().SetValue(pval[i]);
			}
		}
		string CheckValueCapability(string v) {
			if(v == "custom") {
				return "";
			}
			uint pId = uint(ParseInt(v));
			string[]@ pval = presets[pId].Values;
			string ret = "";
			for(uint i = 0; i < pval.length; i++) {
				ret += editors[i].GetConfig().CheckValueCapability(pval[i]);
			}
			return ret;
		}

		// used for StartupScreenConfigSelectItemEditor's ctor param
		string GetValuesString() {
			string[] lst;
			for(uint i = 0; i < presets.length; i++) {
				lst.insertLast(ToString(int(i)));
			}
			lst.insertLast("custom");
			return join(lst, "|");
		}
		// used for StartupScreenConfigSelectItemEditor's ctor param
		string GetDescriptionsString() {
			string[] lst;
			for(uint i = 0; i < presets.length; i++) {
				lst.insertLast(presets[i].Name);
			}
			lst.insertLast(_Tr("StartupScreen", "Custom"));
			return join(lst, "|");
		}
	}

	class StartupScreenComplexConfigPreset {
		string Name;
		string[] Values;
		StartupScreenComplexConfigPreset(string name, string values) {
			Name = name;
			Values = values.split("|");
		}
	}

	class StartupScreenConfigComplexItemEditor: StartupScreenConfigSelectItemEditor {
		private string dlgTitle;

		StartupScreenConfigComplexItemEditor(StartupScreenUI@ ui,
			StartupScreenComplexConfig@ cfg,
			string label, string desc) {
			super(ui, cfg, cfg.GetValuesString(),
				label + ":" + desc + "|" + cfg.GetDescriptionsString());
			dlgTitle = label;
			@buttons[buttons.length - 1].Activated = spades::ui::EventHandler(this.CustomClicked);
		}

		private void CustomClicked(spades::ui::UIElement@) {
			RunDialog();
		}

		private void DialogDone(spades::ui::UIElement@) {
			LoadConfig();
		}

		private void RunDialog() {
			StartupScreenComplexConfigDialog dlg(ui.manager, cast<StartupScreenComplexConfig>(config));
			@dlg.DialogDone = spades::ui::EventHandler(this.DialogDone);
			dlg.Title = dlgTitle;
			dlg.RunDialog();
		}
	}

	class StartupScreenConfigView: spades::ui::ListViewBase {
		private StartupScreenConfigViewModel vmodel;

		StartupScreenConfigView(spades::ui::UIManager@ manager) {
			super(manager);
			this.RowHeight = 30.f;
		}
		void Finalize() {
			@this.Model = vmodel;
		}
		void AddRow(spades::ui::UIElement@ elm) {
			vmodel.items.insertLast(elm);
		}

		void Filter(string text) {
			vmodel.Filter(text);
			Reload();
		}

		void SetHelpTextHandler(HelpTextHandler@ handler) {
			spades::ui::UIElement@[]@ elms = vmodel.items;
			for(uint i = 0; i < elms.length; i++) {
				StartupScreenConfigItemEditor@ item = cast<StartupScreenConfigItemEditor>(elms[i]);
				if(item !is null) {
					item.SetHelpTextHandler(handler);
				}
			}
		}
		void LoadConfig() {
			spades::ui::UIElement@[]@ elms = vmodel.items;
			for(uint i = 0; i < elms.length; i++) {
				StartupScreenConfigItemEditor@ item = cast<StartupScreenConfigItemEditor>(elms[i]);
				if(item !is null) {
					item.LoadConfig();
				}
			}
		}
	}

	class StartupScreenComplexConfigDialog: spades::ui::UIElement {
		StartupScreenComplexConfig@ config;
		float ContentsTop = 50.f;
		float ContentsHeight;
		string Title;

		spades::ui::TextViewer@ helpView;
		StartupScreenConfigView@ configView;

		spades::ui::EventHandler@ DialogDone;

		private spades::ui::UIElement@ oldRoot;

		StartupScreenComplexConfigDialog(spades::ui::UIManager@ manager,
			StartupScreenComplexConfig@ config) {
			super(manager);
			@this.config = config;
			Vector2 size = manager.RootElement.Size;
			ContentsHeight = size.y - ContentsTop * 2.f;

			float mainWidth = size.x - 250.f;
			{
				spades::ui::TextViewer e(Manager);
				e.Bounds = AABB2(mainWidth, ContentsTop + 30.f, size.x - mainWidth - 10.f, ContentsHeight - 60.f);
				AddChild(e);
				@helpView = e;
			}

			{
				StartupScreenConfigView cfg(Manager);

				StartupScreenConfigItemEditor@[]@ editors = config.editors;

				for(uint i = 0; i < editors.length; i++)
					cfg.AddRow(cast<spades::ui::UIElement>(editors[i]));

				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(10.f, ContentsTop + 30.f, mainWidth - 20.f, ContentsHeight - 60.f);
				AddChild(cfg);
				@configView = cfg;
			}

			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("StartupScreen", "Close");
				button.Bounds = AABB2(size.x - 160.f, ContentsTop + ContentsHeight - 30.f, 150.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.CloseActivated);
				AddChild(button);
			}

		}

		private void CloseActivated(spades::ui::UIElement@) {
			if(oldRoot !is null) {
				oldRoot.Enable = true;
				@oldRoot = null;
			}
			@this.Parent = null;
			DialogDone(this);
		}

		private void HandleHelpText(string s) {
			helpView.Text = s;
		}

		void RunDialog() {
			spades::ui::UIElement@ root = Manager.RootElement;

			spades::ui::UIElementIterator iterator(root);
			while(iterator.MoveNext()) {
				spades::ui::UIElement@ e = iterator.Current;
				if(e.Enable and e.Visible) {
					@oldRoot = e;
					e.Enable = false;
					break;
				}
			}

			this.Bounds = root.Bounds;
			root.AddChild(this);

			configView.LoadConfig();
		}

		void Render() {
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Renderer@ r = Manager.Renderer;
			Image@ img = r.RegisterImage("Gfx/White.tga");

			r.ColorNP = Vector4(0, 0, 0, 0.8f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y, size.x, ContentsTop - 15.f));
			r.DrawImage(img,
				AABB2(pos.x, ContentsTop + ContentsHeight + 15.f, size.x, ContentsTop - 15.f));

			r.ColorNP = Vector4(0, 0, 0, 0.95f);
			r.DrawImage(img,
				AABB2(pos.x, ContentsTop - 15.f, size.x, ContentsHeight + 30.f));

			r.ColorNP = Vector4(1, 1, 1, 0.08f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + ContentsTop - 15.f, size.x, 1.f));
			r.DrawImage(img,
				AABB2(pos.x, pos.y + ContentsTop + ContentsHeight + 15.f, size.x, 1.f));
			r.ColorNP = Vector4(1, 1, 1, 0.2f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + ContentsTop - 14.f, size.x, 1.f));
			r.DrawImage(img,
				AABB2(pos.x, pos.y + ContentsTop + ContentsHeight + 14.f, size.x, 1.f));

			Font@ font = Font;
			r.ColorNP = Vector4(0.8f, 0.8f, 0.8f, 1.f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + ContentsTop, size.x, 20.f));
			font.Draw(Title, Vector2(pos.x + 10.f, pos.y + ContentsTop), 1.f, Vector4(0.f, 0.f, 0.f, 1.f));

			UIElement::Render();
		}

	}

	class StartupScreenGraphicsTab: spades::ui::UIElement, LabelAddable {
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;

		StartupScreenGraphicsDisplayResolutionEditor@ resEdit;

		spades::ui::CheckBox@ fullscreenCheck;
		spades::ui::RadioButton@ driverOpenGL;
		spades::ui::RadioButton@ driverSoftware;

		spades::ui::TextViewer@ helpView;
		StartupScreenConfigView@ configViewGL;
		StartupScreenConfigView@ configViewSoftware;

		private ConfigItem r_renderer("r_renderer");
		private ConfigItem r_fullscreen("r_fullscreen");

		StartupScreenGraphicsTab(StartupScreenUI@ ui, Vector2 size) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;

			float mainWidth = size.x - 250.f;

			{
				spades::ui::TextViewer e(Manager);
				e.Bounds = AABB2(mainWidth + 10.f, 0.f, size.x - mainWidth - 10.f, size.y);
				@e.Font = ui.fontManager.GuiFont;
				e.Text = _Tr("StartupScreen", "Graphics Settings");
				AddChild(e);
				@helpView = e;
			}

			AddLabel(0.f, 0.f, 24.f, _Tr("StartupScreen", "Resolution"));
			{
				StartupScreenGraphicsDisplayResolutionEditor e(ui);
				e.Bounds = AABB2(100.f, 0.f, 124.f, 24.f);
				AddChild(e);
				@resEdit = e;
			}

			{
				spades::ui::CheckBox e(Manager);
				e.Caption = _Tr("StartupScreen", "Fullscreen Mode");
				e.Bounds = AABB2(230.f, 0.f, 200.f, 24.f);
				HelpHandler(helpView,
					_Tr("StartupScreen", "By running in fullscreen mode OpenSpades occupies the "
					"screen, making it easier for you to concentrate on playing the game.")).Watch(e);
				@e.Activated = spades::ui::EventHandler(this.OnFullscreenCheck);
				AddChild(e);
				@fullscreenCheck = e;
			}

			AddLabel(0.f, 30.f, 24.f, _Tr("StartupScreen", "Backend"));
			{
				spades::ui::RadioButton e(Manager);
				e.Caption = _Tr("StartupScreen", "OpenGL");
				e.Bounds = AABB2(100.f, 30.f, 140.f, 24.f);
				e.GroupName = "driver";
				HelpHandler(helpView,
					_Tr("StartupScreen", "OpenGL renderer uses your computer's graphics "
						"accelerator to generate the game screen.")).Watch(e);
				@e.Activated = spades::ui::EventHandler(this.OnDriverOpenGL);
				AddChild(e);
				@driverOpenGL = e;
			}
			{
				spades::ui::RadioButton e(Manager);
				e.Caption = _Tr("StartupScreen", "Software");
				e.Bounds = AABB2(250.f, 30.f, 140.f, 24.f);
				e.GroupName = "driver";
				HelpHandler(helpView,
				_Tr("StartupScreen", "Software renderer uses CPU to generate the game "
					"screen. Its quality and performance might be inferior to OpenGL "
					"renderer, but it works even with an unsupported GPU.")).Watch(e);
				@e.Activated = spades::ui::EventHandler(this.OnDriverSoftware);
				AddChild(e);
				@driverSoftware = e;
			}

			{
				StartupScreenConfigView cfg(Manager);

				cfg.AddRow(StartupScreenConfigSelectItemEditor(ui,
					StartupScreenGraphicsAntialiasConfig(ui), "0|2|4|fxaa",
					_Tr("StartupScreen",
					"Antialias:Enables a technique to improve the appearance of high-contrast edges.\n\n"
					"MSAA: Performs antialiasing by generating an intermediate high-resolution image. "
					"Looks best, but doesn't cope with some settings.\n\n"
					"FXAA: Performs antialiasing by smoothing artifacts out as a post-process.|"
					"Off|MSAA 2x|4x|FXAA")));

				cfg.AddRow(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_radiosity"), "0", "1", _Tr("StartupScreen", "Global Illumination"),
					_Tr("StartupScreen",
					"Enables a physically based simulation of light path for more realistic lighting.")));

				cfg.AddRow(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_hdr"), "0", "1", _Tr("StartupScreen", "Linear HDR Rendering"),
					_Tr("StartupScreen",
					"Uses a number representation which allows wider dynamic range during rendering process. "
					"Additionally, this allows color calculation whose value is in linear correspondence with actual energy, "
					"that is, physically accurate blending can be achieved.")));

				{
					StartupScreenComplexConfig cplx;
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_cameraBlur"), "0", "1", _Tr("StartupScreen", "Camera Blur"),
					_Tr("StartupScreen", "Blurs the screen when you turn quickly.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_lens"), "0", "1", _Tr("StartupScreen", "Lens Effect"),
					_Tr("StartupScreen", "Simulates distortion caused by a real camera lens.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_bloom"), "0", "1", _Tr("StartupScreen", "Lens Scattering Filter"),
					_Tr("StartupScreen", "Simulates light being scattered by dust on the camera lens.")));
					// r_lens is currently no-op
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_lensFlare"), "0", "1", _Tr("StartupScreen", "Lens Flare"),
					_Tr("StartupScreen", "The Sun causes lens flare.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_lensFlareDynamic"), "0", "1", _Tr("StartupScreen", "Flares for Dynamic Lights"),
					_Tr("StartupScreen", "Enables lens flare for light sources other than the Sun.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_colorCorrection"), "0", "1", _Tr("StartupScreen", "Color Correction"),
					_Tr("StartupScreen", "Applies cinematic color correction to make the image look better.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_depthOfField"), "0", "1", _Tr("StartupScreen", "Depth of Field"),
					_Tr("StartupScreen", "Blurs out-of-focus objects.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_ssao"), "0", "1", _Tr("StartupScreen", "Screen Space Ambient Occlusion"),
					_Tr("StartupScreen", "Simulates soft shadows that occur between nearby objects.")));

					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Low"), "0|0|0|0|0|0|0|0"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Medium"), "1|0|0|1|0|1|0|0"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "High"), "1|1|1|1|1|1|1|0"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Ultra"), "1|1|1|1|1|1|1|1"));

					cfg.AddRow(StartupScreenConfigComplexItemEditor(ui, cplx,
						_Tr("StartupScreen", "Post-process"),
						_Tr("StartupScreen", "Post-process modifies the image to make it look better and "
							"more realistic.")));
				}

				cfg.AddRow(StartupScreenConfigSelectItemEditor(ui,
					StartupScreenConfig(ui, "r_softParticles"), "0|1|2",
					_Tr("StartupScreen",
					"Particles|"
					"Low:Artifact occurs when a particle intersects other objects.|"
					"Medium:Particle intersects objects smoothly.|"
					"High:Particle intersects objects smoothly, and some objects casts "
					"their shadow to particles.")));

				{
					StartupScreenComplexConfig cplx;
					// r_mapSoftShadow is currently no-op
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_dlights"), "0", "1", _Tr("StartupScreen", "Dynamic Lights"),
					_Tr("StartupScreen",
					"Gives some objects an ability to emit light to give them "
					"an energy-emitting impression.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_modelShadows"), "0", "1", _Tr("StartupScreen", "Shadows"),
					_Tr("StartupScreen", "Non-static object casts a shadow.")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_fogShadow"), "0", "1", _Tr("StartupScreen", "Volumetric Fog"),
					_Tr("StartupScreen", "Simulates shadow being casted to the fog particles using a "
					"super highly computationally demanding algorithm. ")));
					cplx.AddEditor(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "r_physicalLighting"), "0", "1", _Tr("StartupScreen", "Physically Based Lighting"),
					_Tr("StartupScreen", "Uses more accurate approximation techniques to decide the brightness of objects.")));

					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Low"), "1|0|0|0"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Medium"), "1|1|0|0"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "High"), "1|1|0|1"));

					cfg.AddRow(StartupScreenConfigComplexItemEditor(ui, cplx,
						_Tr("StartupScreen", "Direct Lights"),
						_Tr("StartupScreen", "Controls how light encounting a material and atmosphere directly "
										   "affects its appearance.")));
				}

				{
					StartupScreenComplexConfig cplx;

					cplx.AddEditor(StartupScreenConfigSelectItemEditor(ui,
						StartupScreenConfig(ui, "r_water"), "0|1|2|3",
						_Tr("StartupScreen",
						"Water Shader|"
						"None:Water is rendered in the same way that normal blocks are done.|"
						"Level 1:Refraction and the reflected Sun are simulated.|"
						"Level 2:Waving water is simulated as well as reflection and refraction.|"
						"Level 3:Reflections and refractions are rendered at the highest quality using screen-space techniques.")));

					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Low"), "0"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Med"), "1"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "High"), "2"));
					cplx.AddPreset(StartupScreenComplexConfigPreset(_Tr("StartupScreen", "Ultra"), "3"));

					cfg.AddRow(StartupScreenConfigComplexItemEditor(ui, cplx,
						_Tr("StartupScreen", "Shader Effects"), _Tr("StartupScreen", "Special effects.")));
				}


				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(0.f, 60.f, mainWidth, size.y - 60.f);
				AddChild(cfg);
				@configViewGL = cfg;
			}

			{
				StartupScreenConfigView cfg(Manager);

				cfg.AddRow(StartupScreenConfigSelectItemEditor(ui,
					StartupScreenConfig(ui, "r_swUndersampling"), "0|1|2",
					_Tr("StartupScreen",
					"Fast Mode:Reduces the image resolution to make the rendering faster.|"
					"Off|2x|4x")));


				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(0.f, 60.f, mainWidth, size.y - 60.f);
				AddChild(cfg);
				@configViewSoftware = cfg;
			}

		}

		private void HandleHelpText(string text) {
			helpView.Text = text;
		}

		private void OnDriverOpenGL(spades::ui::UIElement@){ r_renderer.StringValue = "gl"; LoadConfig(); }
		private void OnDriverSoftware(spades::ui::UIElement@){ r_renderer.StringValue = "sw"; LoadConfig(); }

		private void OnFullscreenCheck(spades::ui::UIElement@)
			{ r_fullscreen.IntValue = fullscreenCheck.Toggled ? 1 : 0; }

		void LoadConfig() {
			resEdit.LoadConfig();
			if(r_renderer.StringValue == "sw") {
				driverSoftware.Check();
				configViewGL.Visible = false;
				configViewSoftware.Visible = true;
			}else{
				driverOpenGL.Check();
				configViewGL.Visible = true;
				configViewSoftware.Visible = false;
			}
			fullscreenCheck.Toggled = r_fullscreen.IntValue != 0;
			driverOpenGL.Enable = ui.helper.CheckConfigCapability("r_renderer", "gl").length == 0;
			driverSoftware.Enable = ui.helper.CheckConfigCapability("r_renderer", "sw").length == 0;
			configViewGL.LoadConfig();
			configViewSoftware.LoadConfig();

		}



	}

	class StartupScreenComboBoxDropdownButton: spades::ui::SimpleButton {
		StartupScreenComboBoxDropdownButton(spades::ui::UIManager@ manager) {
			super(manager);
		}
		void Render() {
			SimpleButton::Render();

			Renderer@ renderer = Manager.Renderer;
			Image@ image = renderer.RegisterImage("Gfx/UI/ScrollArrow.png");
			AABB2 bnd = ScreenBounds;
			Vector2 p = (bnd.min + bnd.max) * 0.5f + Vector2(-8.f, 8.f);
			renderer.DrawImage(image, AABB2(p.x, p.y, 16.f, -16.f));
		}
	}

	class StartupScreenGraphicsDisplayResolutionEditor: spades::ui::UIElement {
		spades::ui::Field@ widthField;
		spades::ui::Field@ heightField;
		spades::ui::Button@ dropdownButton;
		private ConfigItem r_videoWidth("r_videoWidth");
		private ConfigItem r_videoHeight("r_videoHeight");
		StartupScreenHelper@ helper;

		StartupScreenGraphicsDisplayResolutionEditor(StartupScreenUI@ ui) {
			super(ui.manager);
			@helper = ui.helper;
			{
				spades::ui::Field e(Manager);
				AddChild(e);
				e.Bounds = AABB2(0, 0, 45.f, 24.f);
				e.DenyNonAscii = true;
				@e.Changed = spades::ui::EventHandler(this.ValueEditHandler);
				@widthField = e;
			}
			{
				spades::ui::Field e(Manager);
				AddChild(e);
				e.Bounds = AABB2(53, 0, 45.f, 24.f);
				e.DenyNonAscii = true;
				@e.Changed = spades::ui::EventHandler(this.ValueEditHandler);
				@heightField = e;
			}
			{
				StartupScreenComboBoxDropdownButton e(Manager);
				AddChild(e);
				e.Bounds = AABB2(100, 0, 24.f, 24.f);
				@e.Activated = spades::ui::EventHandler(this.ShowDropdown);
				@dropdownButton = e;
			}
		}

		void LoadConfig() {
			widthField.Text = ToString(r_videoWidth.IntValue);
			heightField.Text = ToString(r_videoHeight.IntValue);
		}

		void SaveConfig() {
			int w = ParseInt(widthField.Text);
			int h = ParseInt(heightField.Text);
			if(w < 640 or h < 480 or w > 8192 or h > 8192) return;
			r_videoWidth.IntValue = w;
			r_videoHeight.IntValue = h;
		}

		private void ValueEditHandler(spades::ui::UIElement@) {
			SaveConfig();
		}

		private void DropdownHandler(int index) {
			if(index >= 0) {
				widthField.Text = ToString(helper.GetVideoModeWidth(index));
				heightField.Text = ToString(helper.GetVideoModeHeight(index));

				SaveConfig();
			}
		}

		private void ShowDropdown(spades::ui::UIElement@) {
			string[] items = {};
			int cnt = helper.GetNumVideoModes();
			for(int i = 0; i < cnt; i++) {
				int w = helper.GetVideoModeWidth(i);
				int h = helper.GetVideoModeHeight(i);
				string s = ToString(w) + "x" + ToString(h);
				items.insertLast(s);
			}
			spades::ui::ShowDropDownList(this, items, spades::ui::DropDownListHandler(this.DropdownHandler));
		}
		void Render() {
			Font@ font = this.Font;
			font.Draw("x", Vector2(45.f, 0.f) + ScreenPosition, 1.f, Vector4(1.f, 1.f, 1.f, 1.f));
			UIElement::Render();
		}
	}

	class StartupScreenGraphicsAntialiasConfig: StartupScreenGenericConfig {
		private StartupScreenUI@ ui;
		private ConfigItem@ msaaConfig;
		private ConfigItem@ fxaaConfig;
		StartupScreenGraphicsAntialiasConfig(StartupScreenUI@ ui) {
			@this.ui = ui;
			@msaaConfig = ConfigItem("r_multisamples");
			@fxaaConfig = ConfigItem("r_fxaa");
		}
		string GetValue() {
			if(fxaaConfig.IntValue != 0) {
				return "fxaa";
			}else{
				int v = msaaConfig.IntValue;
				if(v < 2) return "0";
				else return msaaConfig.StringValue;
			}
		}
		void SetValue(string v) {
			if(v == "fxaa") {
				msaaConfig.StringValue = "0";
				fxaaConfig.StringValue = "1";
			} else if (v == "0" || v == "1") {
				msaaConfig.StringValue = "0";
				fxaaConfig.StringValue = "0";
			} else {
				msaaConfig.StringValue = v;
				fxaaConfig.StringValue = "0";
			}
		}
		string CheckValueCapability(string v) {
			if(v == "fxaa") {
				return ui.helper.CheckConfigCapability("r_multisamples", "0") +
					   ui.helper.CheckConfigCapability("r_fxaa", "1");
			} else if (v == "0" || v == "1") {
				return ui.helper.CheckConfigCapability("r_multisamples", "0") +
					   ui.helper.CheckConfigCapability("r_fxaa", "0");
			} else {
				return ui.helper.CheckConfigCapability("r_multisamples", v) +
					   ui.helper.CheckConfigCapability("r_fxaa", "0");
			}

		}
	}


	class StartupScreenAudioTab: spades::ui::UIElement, LabelAddable {
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;

		spades::ui::RadioButton@ driverOpenAL;
		spades::ui::RadioButton@ driverYSR;
		spades::ui::RadioButton@ driverNull;

		spades::ui::TextViewer@ helpView;
		StartupScreenConfigView@ configViewOpenAL;
		StartupScreenConfigView@ configViewYSR;

		private ConfigItem s_audioDriver("s_audioDriver");
		private ConfigItem s_eax("s_eax");

		StartupScreenAudioTab(StartupScreenUI@ ui, Vector2 size) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;

			float mainWidth = size.x - 250.f;

			{
				spades::ui::TextViewer e(Manager);
				e.Bounds = AABB2(mainWidth + 10.f, 0.f, size.x - mainWidth - 10.f, size.y);
				@e.Font = ui.fontManager.GuiFont;
				e.Text = _Tr("StartupScreen", "Audio Settings");
				AddChild(e);
				@helpView = e;
			}


			AddLabel(0.f, 0.f, 24.f, _Tr("StartupScreen", "Backend"));
			{
				spades::ui::RadioButton e(Manager);
				e.Caption = _Tr("StartupScreen", "OpenAL");
				e.Bounds = AABB2(100.f, 0.f, 100.f, 24.f);
				e.GroupName = "driver";
				HelpHandler(helpView,
					_Tr("StartupScreen", "Uses an OpenAL-capable sound card to process sound. "
					"In most cases where there isn't such a sound card, software emulation is "
					"used.")).Watch(e);
				@e.Activated = spades::ui::EventHandler(this.OnDriverOpenAL);
				AddChild(e);
				@driverOpenAL = e;
			}
			{
				spades::ui::RadioButton e(Manager);
				e.Caption = _Tr("StartupScreen", "YSR");
				e.Bounds = AABB2(210.f, 0.f, 100.f, 24.f);
				e.GroupName = "driver";
				HelpHandler(helpView,
					_Tr("StartupScreen", "YSR is an experimental 3D HDR sound engine optimized "
					"for OpenSpades. It features several enhanced features including "
					"automatic load control, dynamics compressor, HRTF-based "
					"3D audio, and high quality reverb.")).Watch(e);
				@e.Activated = spades::ui::EventHandler(this.OnDriverYSR);
				AddChild(e);
				@driverYSR = e;
			}
			{
				spades::ui::RadioButton e(Manager);
				//! The name of audio driver that outputs no audio.
				e.Caption = _Tr("StartupScreen", "Null");
				e.Bounds = AABB2(320.f, 0.f, 100.f, 24.f);
				e.GroupName = "driver";
				HelpHandler(helpView,
					_Tr("StartupScreen", "Disables audio output.")).Watch(e);
				@e.Activated = spades::ui::EventHandler(this.OnDriverNull);
				AddChild(e);
				@driverNull = e;
			}

			{
				StartupScreenConfigView cfg(Manager);

				cfg.AddRow(StartupScreenConfigSliderItemEditor(ui,
					StartupScreenConfig(ui, "s_maxPolyphonics"), 16.0, 256.0, 8.0,
					_Tr("StartupScreen", "Polyphonics"), _Tr("StartupScreen",
					"Specifies how many sounds can be played simultaneously. "
					"Higher value needs more processing power, so setting this too high might "
					"cause an overload (especially with a software emulation)."),
					ConfigNumberFormatter(0, " poly")));

				cfg.AddRow(StartupScreenConfigCheckItemEditor(ui,
					StartupScreenConfig(ui, "s_eax"), "0", "1",
					_Tr("StartupScreen", "EAX"), _Tr("StartupScreen",
					"Enables extended features provided by the OpenAL driver to create "
					"more ambience.")));

				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(0.f, 30.f, mainWidth, size.y - 30.f);
				AddChild(cfg);
				@configViewOpenAL = cfg;
			}

			{
				StartupScreenConfigView cfg(Manager);

				cfg.AddRow(StartupScreenConfigSliderItemEditor(ui,
					StartupScreenConfig(ui, "s_maxPolyphonics"), 16.0, 256.0, 8.0,
					_Tr("StartupScreen", "Polyphonics"), _Tr("StartupScreen",
					"Specifies how many sounds can be played simultaneously. "
					"No matter what value is set, YSR might reduce the number of sounds "
					"when an overload is detected."),
					ConfigNumberFormatter(0, " poly")));


				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(0.f, 30.f, mainWidth, size.y - 30.f);
				AddChild(cfg);
				@configViewYSR = cfg;
			}

		}

		private void HandleHelpText(string text) {
			helpView.Text = text;
		}

		private void OnDriverOpenAL(spades::ui::UIElement@){ s_audioDriver.StringValue = "openal"; LoadConfig(); }
		private void OnDriverYSR(spades::ui::UIElement@){ s_audioDriver.StringValue = "ysr"; LoadConfig(); }
		private void OnDriverNull(spades::ui::UIElement@){ s_audioDriver.StringValue = "null"; LoadConfig(); }

		void LoadConfig() {
			if(s_audioDriver.StringValue == "ysr") {
				driverYSR.Check();
				configViewOpenAL.Visible = false;
				configViewYSR.Visible = true;
			}else if(s_audioDriver.StringValue == "openal"){
				driverOpenAL.Check();
				configViewOpenAL.Visible = true;
				configViewYSR.Visible = false;
			}else if(s_audioDriver.StringValue == "null"){
				driverNull.Check();
				configViewOpenAL.Visible = false;
				configViewYSR.Visible = false;
			}
			driverOpenAL.Enable = ui.helper.CheckConfigCapability("s_audioDriver", "openal").length == 0;
			driverYSR.Enable = ui.helper.CheckConfigCapability("s_audioDriver", "ysr").length == 0;
			driverNull.Enable = ui.helper.CheckConfigCapability("s_audioDriver", "null").length == 0;
			configViewOpenAL.LoadConfig();
			configViewYSR.LoadConfig();

		}



	}

	class StartupScreenGenericTab: spades::ui::UIElement, LabelAddable {
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;

		StartupScreenLocaleEditor@ locale;

		StartupScreenGenericTab(StartupScreenUI@ ui, Vector2 size) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;

			string label = _Tr("StartupScreen", "Language");
			if (label != "Language") {
				label += " (Language)";
			}
			AddLabel(0.f, 0.f, 24.f, _Tr("StartupScreen", label));
			{
				StartupScreenLocaleEditor e(ui);
				AddChild(e);
				e.Bounds = AABB2(160.f, 0.f, 400.f, 24.f);
				@locale = e;
			}

			AddLabel(0.f, 30.f, 30.f, _Tr("StartupScreen", "Tools"));
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("StartupScreen", "Reset All Settings");
				button.Bounds = AABB2(160.f, 30.f, 350.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnResetSettingsPressed);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				string osType = helper.OperatingSystemType;
				if (osType == "Windows") {
					button.Caption = _Tr("StartupScreen", "Open Config Folder in Explorer");
				} else if (osType == "Mac") {
					button.Caption = _Tr("StartupScreen", "Reveal Config Folder in Finder");
				} else {
					button.Caption = _Tr("StartupScreen", "Browse Config Folder");
				}
				button.Bounds = AABB2(160.f, 66.f, 350.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnBrowseUserDirectoryPressed);
				AddChild(button);
			}
		}

		void LoadConfig() {
			locale.LoadConfig();
		}

		private void OnBrowseUserDirectoryPressed(spades::ui::UIElement@) {
			if (helper.BrowseUserDirectory()) {
				return;
			}

			string msg = _Tr("StartupScreen", "An unknown error has occurred while opening the config directory.");
			AlertScreen al(Parent, msg, 100.f);
			al.Run();
		}

		private void OnResetSettingsPressed(spades::ui::UIElement@) {
			string msg = _Tr("StartupScreen", "Are you sure to reset all settings? They include (but are not limited to):") + "\n" +
				"- " + _Tr("StartupScreen", "All graphics/audio settings") + "\n" +
				"- " + _Tr("StartupScreen", "All key bindings") + "\n" +
				"- " + _Tr("StartupScreen", "Your player name") + "\n" +
				"- " + _Tr("StartupScreen", "Other advanced settings only accessible through '{0}' tab and in-game commands",
					_Tr("StartupScreen", "Advanced"));
			ConfirmScreen al(Parent, msg, 200.f);
			@al.Closed = spades::ui::EventHandler(OnResetSettingsConfirmed);
			al.Run();
		}

		private void OnResetSettingsConfirmed(spades::ui::UIElement@ sender) {
			if (!cast<ConfirmScreen>(sender).Result) {
				return;
			}

			ResetAllSettings();

			// Reload the startup screen so the language config takes effect
			ui.Reload();
		}

		private void ResetAllSettings() {
			string[]@ names = GetAllConfigNames();

			for(uint i = 0, count = names.length; i < count; i++) {
				ConfigItem item(names[i]);
				item.StringValue = item.DefaultValue;
			}

			// Some of default values may be infeasible for the user's system.
			helper.FixConfigs();
		}

	}

	class StartupScreenDropdownListDropdownButton: spades::ui::SimpleButton {
		StartupScreenDropdownListDropdownButton(spades::ui::UIManager@ manager) {
			super(manager);
			Alignment = Vector2(0.f, 0.5f);
		}
		void Render() {
			SimpleButton::Render();

			Renderer@ renderer = Manager.Renderer;
			Image@ arrowImg = renderer.RegisterImage("Gfx/UI/ScrollArrow.png");

			AABB2 bnd = ScreenBounds;
			Vector2 p = (bnd.min + bnd.max) * 0.5f + Vector2(-8.f, 8.f);
			renderer.DrawImage(arrowImg, AABB2(bnd.max.x - 16.f, p.y, 16.f, -16.f));
		}
	}

	class StartupScreenLocaleEditor: spades::ui::UIElement, LabelAddable {StartupScreenUI@ ui;
		StartupScreenHelper@ helper;
		private ConfigItem core_locale("core_locale");

		spades::ui::Button@ dropdownButton;

		StartupScreenLocaleEditor(StartupScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;
			{
				StartupScreenDropdownListDropdownButton e(Manager);
				AddChild(e);
				e.Bounds = AABB2(0.f, 0.f, 400.f, 24.f);
				@e.Activated = spades::ui::EventHandler(this.ShowDropdown);
				@dropdownButton = e;
			}
		}

		void LoadConfig() {
			string locale = core_locale.StringValue;
			string name = _Tr("StartupScreen", "Unknown ({0})", locale);
			if (locale == "") {
				name = _Tr("StartupScreen", "System default");
			}

			int cnt = helper.GetNumLocales();
			for(int i = 0; i < cnt; i++) {
				if (locale == helper.GetLocale(i)) {
					name = helper.GetLocaleDescriptionNative(i) + " / " + helper.GetLocaleDescriptionEnglish(i);
				}
			}

			dropdownButton.Caption = name;
		}

		private void ShowDropdown(spades::ui::UIElement@) {
			string[] items = {_Tr("StartupScreen", "System default")};
			int cnt = helper.GetNumLocales();
			for(int i = 0; i < cnt; i++) {
				string s = helper.GetLocaleDescriptionNative(i) + " / " + helper.GetLocaleDescriptionEnglish(i);
				items.insertLast(s);
			}
			spades::ui::ShowDropDownList(this, items, spades::ui::DropDownListHandler(this.DropdownHandler));
		}

		private void DropdownHandler(int index) {
			if(index >= 0) {
				if (index == 0) {
					core_locale = "";
				} else {
					core_locale = helper.GetLocale(index - 1);
				}

				// Reload the startup screen so the language config takes effect
				ui.Reload();
			}
		}
	}

	class StartupScreenSystemInfoTab: spades::ui::UIElement, LabelAddable {
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;

		spades::ui::TextViewer@ helpView;

		StartupScreenSystemInfoTab(StartupScreenUI@ ui, Vector2 size) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;

			{
				spades::ui::TextViewer e(Manager);
				e.Bounds = AABB2(0.f, 0.f, size.x, size.y - 30.f);
				@e.Font = ui.fontManager.GuiFont;
				AddChild(e);
				for(int i = 0, count = helper.GetNumReportLines(); i < count; i++) {
					string text = helper.GetReportLineText(i);
					Vector4 col = helper.GetReportLineColor(i);
					e.AddLine(text, true, col);
				}
				@helpView = e;
			}

			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("StartupScreen", "Copy to Clipboard");
				button.Bounds = AABB2(size.x - 180.f, size.y - 30.f, 180.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnCopyReport);
				AddChild(button);
			}

		}
		private void OnCopyReport(spades::ui::UIElement@){
			 Manager.Copy(helper.GetReport());
		}



	}


	class StartupScreenAdvancedTab: spades::ui::UIElement, LabelAddable {
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;

		spades::ui::Field@ filter;

		spades::ui::TextViewer@ helpView;
		StartupScreenConfigView@ configView;

		StartupScreenAdvancedTab(StartupScreenUI@ ui, Vector2 size) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;

			float mainWidth = size.x - 250.f;

			{
				spades::ui::TextViewer e(Manager);
				e.Bounds = AABB2(mainWidth + 10.f, 0.f, size.x - mainWidth - 10.f, size.y);
				@e.Font = ui.fontManager.GuiFont;
				e.Text = _Tr("StartupScreen", "Advanced Settings");
				e.Visible = false;
				AddChild(e);
				@helpView = e;
			}


			{
				spades::ui::Field e(Manager);
				e.Placeholder = _Tr("StartupScreen", "Filter");
				e.Bounds = AABB2(0.f, 0.f, size.x, 24.f);
				@e.Changed = spades::ui::EventHandler(this.OnFilterChanged);
				AddChild(e);
				@filter = e;
			}

			{
				StartupScreenConfigView cfg(Manager);

				string[]@ names = GetAllConfigNames();

				for(uint i = 0, count = names.length; i < count; i++) {

					cfg.AddRow(StartupScreenConfigFieldItemEditor(ui,
						StartupScreenConfig(ui, names[i]), names[i], ""));
				}

				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(0.f, 30.f, size.x, size.y - 30.f);
				AddChild(cfg);
				@configView = cfg;
			}
		}

		private void HandleHelpText(string text) {
			helpView.Text = text;
		}

		private void OnFilterChanged(spades::ui::UIElement@){
			configView.Filter(filter.Text);
		}

		void LoadConfig() {
			configView.LoadConfig();

		}



	}


	StartupScreenUI@ CreateStartupScreenUI(Renderer@ renderer, AudioDevice@ audioDevice,
		FontManager@ fontManager, StartupScreenHelper@ helper) {
		return StartupScreenUI(renderer, audioDevice, fontManager, helper);
	}
}
