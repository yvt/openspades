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

#include "Flags.as"

namespace spades {
	

	class StartupScreenUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		Font@ font;
		StartupScreenHelper@ helper;
		
		spades::ui::UIManager@ manager;
		
		StartupScreenMainMenu@ mainMenu;
		
		
		bool shouldExit = false;
		StartupScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, Font@ font, StartupScreenHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.font = font;
			@this.helper = helper;
			
			SetupRenderer();
			
			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = font;
			
			@mainMenu = StartupScreenMainMenu(this);
			mainMenu.Bounds = manager.RootElement.Bounds;
			manager.RootElement.AddChild(mainMenu);
			
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
	
	
	
	class StartupScreenMainMenu: spades::ui::UIElement {
		
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;
		
		spades::ui::ListView@ serverList;
		spades::ui::CheckBox@ bypassStartupWindowCheck;
		
		StartupScreenGraphicsTab@ graphicsTab;
		
		private ConfigItem cl_showStartupWindow("cl_showStartupWindow", "-1");
		
		
		StartupScreenMainMenu(StartupScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;
			
			@this.Font = ui.font;
			
			float width = Manager.Renderer.ScreenWidth;
			float height = Manager.Renderer.ScreenHeight;
			{
				spades::ui::Button button(Manager);
				button.Caption = "Start";
				button.Bounds = AABB2(width - 170.f, 20.f, 150.f, 30.f);
				button.Activated = EventHandler(this.OnStartPressed);
				AddChild(button);
			}
			{
				spades::ui::CheckBox button(Manager);
				button.Caption = "Skip this screen next time";
				button.Bounds = AABB2(360.f, 62.f, width - 380.f, 20.f);
				AddChild(button);
				@bypassStartupWindowCheck = button;
				button.Activated = EventHandler(this.OnBypassStartupWindowCheckChanged);
			}
			
			AABB2 clientArea(10.f, 100.f, width - 20.f, height - 110.f);
			StartupScreenGraphicsTab graphicsTab(ui, clientArea.max - clientArea.min);
			spades::ui::UIElement audioTab(Manager);
			spades::ui::UIElement profileTab(Manager);
			graphicsTab.Bounds = clientArea;
			audioTab.Bounds = clientArea;
			profileTab.Bounds = clientArea;
			AddChild(graphicsTab);
			AddChild(audioTab);
			AddChild(profileTab);
			audioTab.Visible = false;
			profileTab.Visible = false;
			
			@this.graphicsTab = graphicsTab;
			
			{
				spades::ui::SimpleTabStrip tabStrip(Manager);
				AddChild(tabStrip);
				tabStrip.Bounds = AABB2(10.f, 70.f, width - 20.f, 24.f);
				tabStrip.AddItem("Graphics", graphicsTab);
				tabStrip.AddItem("Audio", audioTab);
				tabStrip.AddItem("System Info", profileTab);
				
			}
			
			LoadConfig();
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
		}
		
		private void OnBypassStartupWindowCheckChanged(spades::ui::UIElement@ sender) {
			cl_showStartupWindow.IntValue = (bypassStartupWindowCheck.Toggled ? 0 : 1);
		}
		
		
		private void OnQuitPressed(spades::ui::UIElement@ sender) {
			ui.shouldExit = true;
		}
		
		private void OnSetupPressed(spades::ui::UIElement@ sender) {
			PreferenceView al(this, PreferenceViewOptions());
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
			@elm.MouseEntered = EventHandler(this.OnMouseHover);
		}
	}
	
	mixin class LabelAddable {
	
		private void AddLabel(float x, float y, float h, string text) {
			spades::ui::Label label(Manager);
			Font@ font = ui.font;
			Vector2 siz = font.Measure(text);
			label.Text = text;
			label.Alignment = Vector2(0.f, 0.5f);
			label.Bounds = AABB2(x, y, siz.x, h);
			AddChild(label);
		}
		
	}
	class StartupScreenConfigViewModel: spades::ui::ListViewModel {
		spades::ui::UIElement@[] items;
		StartupScreenConfigViewModel() {
		}
		int NumRows { 
			get { return int(items.length); }
		}
		spades::ui::UIElement@ CreateElement(int row) {
			return items[row];
		}
		void RecycleElement(spades::ui::UIElement@ elem) {
		}
	}
	
	interface StartupScreenGenericConfig {
		string GetValue();
		void SetValue(string);
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
	
	interface StartupScreenConfigItem {
		void LoadConfig();
		/** Returns an empty string when there's no problem. */
		string CheckValueCapability(string);
		void SetHelpTextHandler(HelpTextHandler@);
	}
	
	
	class StartupScreenConfigSelectItem: spades::ui::UIElement, LabelAddable, StartupScreenConfigItem {
		
		private StartupScreenUI@ ui;
		private string[]@ descs;
		private string[]@ values;
		private StartupScreenGenericConfig@ config;
		private spades::ui::RadioButton@[] buttons;
		
		StartupScreenConfigSelectItem(StartupScreenUI@ ui,
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
				@b.Activated = EventHandler(setter.Set);
				buttons.insertLast(b);
				this.AddChild(b);
			}
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
				float w = ui.font.Measure(b.Caption).x + 20.f;
				x -= w;
				b.Bounds = AABB2(x, 0.f, w, h);
			}
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
		
		void SetHelpTextHandler(HelpTextHandler@ handler) {
			spades::ui::UIElement@[]@ elms = vmodel.items;
			for(uint i = 0; i < elms.length; i++) {
				StartupScreenConfigItem@ item = cast<StartupScreenConfigItem>(elms[i]);
				if(item !is null) {
					item.SetHelpTextHandler(handler);
				}
			}
		}
		void LoadConfig() {
			spades::ui::UIElement@[]@ elms = vmodel.items;
			for(uint i = 0; i < elms.length; i++) {
				StartupScreenConfigItem@ item = cast<StartupScreenConfigItem>(elms[i]);
				if(item !is null) {
					item.LoadConfig();
				}
			}
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
	
	class StartupScreenGraphicsTab: spades::ui::UIElement, LabelAddable {
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;
		
		spades::ui::RadioButton@ driverOpenGL;
		spades::ui::RadioButton@ driverSoftware;
		
		spades::ui::TextViewer@ helpView;
		StartupScreenConfigView@ configView;
		
		private ConfigItem r_renderer("r_renderer");
		
		StartupScreenGraphicsTab(StartupScreenUI@ ui, Vector2 size) {
			super(ui.manager);
			@this.ui = ui;
			@helper = ui.helper;
			
			float mainWidth = size.x - 250.f;
			
			{
				spades::ui::TextViewer e(Manager);
				e.Bounds = AABB2(mainWidth + 10.f, 0.f, size.x - mainWidth - 10.f, size.y);
				@e.Font = ui.font;
				e.Text = "Graphics Settings";
				AddChild(e);
				@helpView = e;
			}
			AddLabel(0.f, 0.f, 20.f, "Backend");
			{
				spades::ui::RadioButton e(Manager);
				e.Caption = "OpenGL";
				e.Bounds = AABB2(80.f, 0.f, 100.f, 20.f);
				e.GroupName = "driver";
				HelpHandler(helpView, "OpenGL renderer uses your computer's graphics accelerator to generate the game screen.").Watch(e);
				@e.Activated = EventHandler(this.OnDriverOpenGL);
				AddChild(e);
				@driverOpenGL = e;
				
				// TODO: disable for unsupported video card
			}
			{
				spades::ui::RadioButton e(Manager);
				e.Caption = "Software";
				e.Bounds = AABB2(190.f, 0.f, 100.f, 20.f);
				e.GroupName = "driver";
				HelpHandler(helpView, "Software renderer uses CPU to generate the game screen. Its quality and performance might be inferior to OpenGL renderer, but it works even with an unsupported GPU.").Watch(e);
				@e.Activated = EventHandler(this.OnDriverSoftware);
				AddChild(e);
				@driverSoftware = e;
			}
			
			{
				StartupScreenConfigView cfg(Manager);
				cfg.AddRow(StartupScreenConfigSelectItem(ui, 
					StartupScreenGraphicsAntialiasConfig(ui), "0|2|4|fxaa",
					"Antialias:Enables a technique to improve the appearance of high-constrast edges.|" +
					"Off|MSAA 2x|4x|FXAA"));
				cfg.AddRow(StartupScreenConfigSelectItem(ui, 
					StartupScreenConfig(ui, "r_radiosity"), "0|1",
					"Global Illumination:Enables a physically based technique for more realistic lighting.|" +
					"Off|On"));
				cfg.Finalize();
				cfg.SetHelpTextHandler(HelpTextHandler(this.HandleHelpText));
				cfg.Bounds = AABB2(0.f, 30.f, mainWidth, size.y - 30.f);
				AddChild(cfg);
				@configView = cfg;
			}
			
			
		}
		
		private void HandleHelpText(string text) {
			helpView.Text = text;
		}
		
		private void OnDriverOpenGL(spades::ui::UIElement@){ r_renderer.StringValue = "gl"; }
		private void OnDriverSoftware(spades::ui::UIElement@){ r_renderer.StringValue = "sw"; }
		
		void LoadConfig() {
			if(r_renderer.StringValue == "sw") {
				driverSoftware.Check();
			}else{
				driverOpenGL.Check();
			}
			configView.LoadConfig();
		}
		
		
		
	}
	
	StartupScreenUI@ CreateStartupScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, 
		Font@ font, StartupScreenHelper@ helper) {
		return StartupScreenUI(renderer, audioDevice, font, helper);
	}
}
