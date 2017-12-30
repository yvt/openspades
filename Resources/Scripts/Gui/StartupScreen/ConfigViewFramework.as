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

#include "../UIFramework/UIControls.as"

namespace spades {

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

}
