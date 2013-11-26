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

namespace spades {

	class PreferenceViewOptions {
		bool GameActive = false;
	}
	
	class PreferenceView: spades::ui::UIElement {
		private spades::ui::UIElement@ owner;
		
		private PreferenceTab@[] tabs;
		float ContentsLeft, ContentsWidth;
		float ContentsTop, ContentsHeight;
		
		int SelectedTabIndex = 0;
		
		spades::ui::EventHandler@ Closed;
		
		PreferenceView(spades::ui::UIElement@ owner, PreferenceViewOptions@ options) {
			super(owner.Manager);
			@this.owner = owner;
			this.Bounds = owner.Bounds;
			ContentsWidth = 800.f;
			ContentsLeft = (Manager.Renderer.ScreenWidth - ContentsWidth) * 0.5f;
			ContentsHeight = 550.f;
			ContentsTop = (Manager.Renderer.ScreenHeight - ContentsHeight) * 0.5f;
			
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.4f);
				label.Bounds = Bounds;
				AddChild(label);
			}
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.8f);
				label.Bounds = AABB2(0.f, ContentsTop - 13.f, Size.x, ContentsHeight + 27.f);
				AddChild(label);
			}
			
			AddTab(GameOptionsPanel(Manager, options), "Game Options");
			AddTab(ControlOptionsPanel(Manager, options), "Controls");
			
			{
				PreferenceTabButton button(Manager);
				button.Caption = "Back";
				button.Bounds = AABB2(
					ContentsLeft + 10.f, 
					ContentsTop + 10.f + float(tabs.length) * 32.f + 5.f
					, 150.f, 30.f);
				button.Alignment = Vector2(0.f, 0.5f);
				button.Activated = spades::ui::EventHandler(this.OnClosePressed);
				AddChild(button);
			}
			
			UpdateTabs();
		}
		
		private void AddTab(spades::ui::UIElement@ view, string caption) {
			PreferenceTab tab(this, view);
			int order = int(tabs.length);
			tab.TabButton.Bounds = AABB2(ContentsLeft + 10.f, ContentsTop + 10.f + float(order) * 32.f, 150.f, 30.f);
			tab.TabButton.Caption = caption;
			tab.View.Bounds = AABB2(ContentsLeft + 170.f, ContentsTop + 10.f, ContentsWidth - 180.f, ContentsHeight - 20.f);
			tab.View.Visible = false;
			tab.TabButton.Activated = spades::ui::EventHandler(this.OnTabButtonActivated);
			AddChild(tab.View);
			AddChild(tab.TabButton);
			tabs.insertLast(tab);
		}
		
		private void OnTabButtonActivated(spades::ui::UIElement@ sender) {
			for(uint i = 0; i < tabs.length; i++) {
				if(cast<spades::ui::UIElement>(tabs[i].TabButton) is sender) {
					SelectedTabIndex = i;
					UpdateTabs();
				}
			}
		}
		
		private void UpdateTabs() {
			for(uint i = 0; i < tabs.length; i++) {
				PreferenceTab@ tab = tabs[i];
				bool selected = SelectedTabIndex == int(i);
				tab.TabButton.Toggled = selected;
				tab.View.Visible = selected;
			}
		}
		
		private void OnClosePressed(spades::ui::UIElement@ sender) {
			Close();
		}
		
		private void OnClosed() {
			if(Closed !is null) Closed(this);
		}
		
		void HotKey(string key) {
			if(key == "Escape") {
				Close();
			} else {
				UIElement::HotKey(key);
			}
		}
		
		void Render() {
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Renderer@ r = Manager.Renderer;
			Image@ img = r.RegisterImage("Gfx/White.tga");
			
			r.Color = Vector4(1, 1, 1, 0.08f);
			r.DrawImage(img, 
				AABB2(pos.x, pos.y + ContentsTop - 15.f, size.x, 1.f));
			r.DrawImage(img, 
				AABB2(pos.x, pos.y + ContentsTop + ContentsHeight + 15.f, size.x, 1.f));
			r.Color = Vector4(1, 1, 1, 0.2f);
			r.DrawImage(img, 
				AABB2(pos.x, pos.y + ContentsTop - 14.f, size.x, 1.f));
			r.DrawImage(img, 
				AABB2(pos.x, pos.y + ContentsTop + ContentsHeight + 14.f, size.x, 1.f));
				
			UIElement::Render();
		}
		
		
		void Close() {
			owner.Enable = true;
			@this.Parent = null;
			OnClosed();
		}
		
		void Run() {
			owner.Enable = false;
			owner.Parent.AddChild(this);
		}
		
		
	}
	
	class PreferenceTabButton: spades::ui::Button {
		PreferenceTabButton(spades::ui::UIManager@ manager) {
			super(manager);
			Alignment = Vector2(0.f, 0.5f);
		}
		/*
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			
			Vector4 color = Vector4(0.2f, 0.2f, 0.2f, 0.5f);
			if(Toggled or (Pressed and Hover)) {
				color = Vector4(0.7f, 0.7f, 0.7f, 0.9f);
			}else if(Hover) {
				color = Vector4(0.4f, 0.4f, 0.4f, 0.7f);
			}
			
			Font@ font = this.Font;
			string text = this.Caption;
			Vector2 txtSize = font.Measure(text);
			Vector2 txtPos;
			txtPos.y = pos.y + (size.y - txtSize.y) * 0.5f;
			
			font.DrawShadow(text, txtPos, 1.f, 
				color, Vector4(0.f, 0.f, 0.f, 0.4f));
		}*/
		
	}
	
	class PreferenceTab {
		spades::ui::UIElement@ View;
		PreferenceTabButton@ TabButton;
		
		PreferenceTab(PreferenceView@ parent, spades::ui::UIElement@ view) {
			@View = view;
			@TabButton = PreferenceTabButton(parent.Manager);
			TabButton.Toggle = true;
		}
	}
	
	class ConfigField: spades::ui::Field {
		ConfigItem@ config;
		ConfigField(spades::ui::UIManager manager, string configName) {
			super(manager);
			@config = ConfigItem(configName);
			this.Text = config.StringValue;
		}
		
		void OnChanged() {
			Field::OnChanged();
			config = this.Text;
		}
	}
	
	class ConfigSimpleToggleButton: spades::ui::SimpleButton {
		ConfigItem@ config;
		int value;
		ConfigSimpleToggleButton(spades::ui::UIManager manager, string caption, string configName, int value) {
			super(manager);
			@config = ConfigItem(configName);
			this.Caption = caption;
			this.value = value;
			this.Toggle = true;
			this.Toggled = config.IntValue == value;
		}
		
		void OnActivated() {
			SimpleButton::OnActivated();
			this.Toggled = true;
			config = value;
		}
		
		void Render() {
			this.Toggled = config.IntValue == value;
			SimpleButton::Render();
		}
	}
	
	class StandardPreferenceLayouterModel: spades::ui::ListViewModel {
		private spades::ui::UIElement@[]@ items;
		StandardPreferenceLayouterModel(spades::ui::UIElement@[]@ items) {
			@this.items = items;
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
	class StandardPreferenceLayouter {
		spades::ui::UIElement@ Parent;
		private float FieldX = 190.f;
		private float FieldWidth = 370.f;
		private spades::ui::UIElement@[] items;
		
		StandardPreferenceLayouter(spades::ui::UIElement@ parent) {
			@Parent = parent;
		}
		
		private spades::ui::UIElement@ CreateItem() {
			spades::ui::UIElement elem(Parent.Manager);
			elem.Size = Vector2(300.f, 32.f);
			items.insertLast(elem);
			return elem;
		}
		
		void AddHeading(string text) {
			spades::ui::UIElement@ container = CreateItem();
			
			spades::ui::Label label(Parent.Manager);
			label.Text = text;
			label.Alignment = Vector2(0.f, 1.f);
			label.TextScale = 1.3f;
			label.Bounds = AABB2(10.f, 0.f, 300.f, 32.f);
			container.AddChild(label);
		}
		
		void AddInputField(string caption, string configName, bool enabled = true) {
			spades::ui::UIElement@ container = CreateItem();
			
			spades::ui::Label label(Parent.Manager);
			label.Text = caption;
			label.Alignment = Vector2(0.f, 0.5f);
			label.Bounds = AABB2(10.f, 0.f, 300.f, 32.f);
			container.AddChild(label);
			
			ConfigField field(Parent.Manager, configName);
			field.Bounds = AABB2(FieldX, 1.f, FieldWidth, 30.f);
			field.MaxLength = 15;
			field.Enable = enabled;
			container.AddChild(field);
			
		}
		
		void AddControl(string caption, string configName, bool enabled = true) {
			AddInputField(caption, configName, enabled);
		
			// TODO
		}
		
		void AddToggleField(string caption, string configName, bool enabled = true) {
			spades::ui::UIElement@ container = CreateItem();
			
			spades::ui::Label label(Parent.Manager);
			label.Text = caption;
			label.Alignment = Vector2(0.f, 0.5f);
			label.Bounds = AABB2(10.f, 0.f, 300.f, 32.f);
			container.AddChild(label);
			
			{
				ConfigSimpleToggleButton field(Parent.Manager, "ON", configName, 1);
				field.Bounds = AABB2(FieldX, 1.f, FieldWidth * 0.5f, 30.f);
				field.Enable = enabled;
				container.AddChild(field);
			}
			{
				ConfigSimpleToggleButton field(Parent.Manager, "OFF", configName, 0);
				field.Bounds = AABB2(FieldX + FieldWidth * 0.5f, 1.f, FieldWidth * 0.5f, 30.f);
				field.Enable = enabled;
				container.AddChild(field);
			}
			
		}
		
		void FinishLayout() {
			spades::ui::ListView list(Parent.Manager);
			@list.Model = StandardPreferenceLayouterModel(items);
			list.RowHeight = 32.f;
			list.Bounds = AABB2(0.f, 0.f, 580.f, 530.f);
			Parent.AddChild(list);
		}
	}
	
	class GameOptionsPanel: spades::ui::UIElement {
		GameOptionsPanel(spades::ui::UIManager@ manager, PreferenceViewOptions@ options) {
			super(manager);
			
			StandardPreferenceLayouter layouter(this);
			layouter.AddHeading("Player Information");
			layouter.AddInputField("Player Name", "cg_playerName", not options.GameActive);
			
			layouter.AddHeading("Effects");
			layouter.AddToggleField("Blood", "cg_blood");
			layouter.AddToggleField("Ejecting Brass", "cg_ejectBrass");
			layouter.AddToggleField("Ragdoll", "cg_ragdoll");
			
			layouter.AddHeading("Misc");
			layouter.FinishLayout();
			// cg_fov, cg_minimapSize
		}
	}
	
	class ControlOptionsPanel: spades::ui::UIElement {
		ControlOptionsPanel(spades::ui::UIManager@ manager, PreferenceViewOptions@ options) {
			super(manager);
			
			StandardPreferenceLayouter layouter(this);
			layouter.AddHeading("Weapons/Tools");
			layouter.AddControl("Attack", "cg_keyAttack");
			layouter.AddControl("Alt. Attack", "cg_keyAltAttack");
			layouter.AddToggleField("Hold Aim Down Sight", "cg_holdAimDownSight");
			layouter.AddControl("Reload", "cg_keyReloadWeapon");
			layouter.AddControl("Capture Color", "cg_keyCaptureColor");
			layouter.AddControl("Equip Spade", "cg_keyToolSpade");
			layouter.AddControl("Equip Block", "cg_keyToolBlock");
			layouter.AddControl("Equip Weapon", "cg_keyToolWeapon");
			layouter.AddControl("Equip Grenade", "cg_keyToolGrenade");
			layouter.AddToggleField("Switch Tool by Wheel", "cg_switchToolByWheel");
			// TODO: mouse sensitivity: cg_mouseSensitivity, cg_zoomedMouseSensScale
			
			layouter.AddHeading("Movement");
			layouter.AddControl("Forward", "cg_keyMoveForward");
			layouter.AddControl("Backpedal", "cg_keyMoveBackward");
			layouter.AddControl("Move Left", "cg_keyMoveLeft");
			layouter.AddControl("Move Right", "cg_keyMoveRight");
			layouter.AddControl("Crouch", "cg_keyCrouch");
			layouter.AddControl("Sneak", "cg_keySneak");
			layouter.AddControl("Jump", "cg_keyJump");
			layouter.AddControl("Sprint", "cg_keySprint");
			
			layouter.AddHeading("Misc");
			layouter.AddControl("Minimap Scale", "cg_keyChangeMapScale");
			layouter.AddControl("Toggle Map", "cg_keyToggleMapZoom");
			layouter.AddControl("Flashlight", "cg_keyFlashlight");
			layouter.AddControl("Global Chat", "cg_keyGlobalChat");
			layouter.AddControl("Team Chat", "cg_keyTeamChat");
			layouter.AddControl("Limbo Menu", "cg_keyLimbo");
			layouter.AddControl("Save Map", "cg_keySaveMap");
			layouter.AddControl("Save Sceneshot", "cg_keySceneshot");
			layouter.AddControl("Save Screenshot", "cg_keyScreenshot");
			
			layouter.FinishLayout();
		}
	}
	
}
