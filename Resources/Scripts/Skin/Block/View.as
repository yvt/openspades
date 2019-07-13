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
    class ViewBlockSkin : IToolSkin, IViewToolSkin, IBlockSkin {
        private float sprintState;
        private float raiseState;
        private Vector3 teamColor;
        private Matrix4 eyeMatrix;
        private Vector3 swing;
        private Vector3 leftHand;
        private Vector3 rightHand;
        private Vector3 blockColor;
        private float readyState;

        private float sprintStateSmooth;

        float SprintState {
            set { sprintState = value; }
        }

        float RaiseState {
            set { raiseState = value; }
        }

        Vector3 TeamColor {
            set { teamColor = value; }
        }

        bool IsMuted {
            set {
                // nothing to do
            }
        }

        Matrix4 EyeMatrix {
            set { eyeMatrix = value; }
        }

        Vector3 Swing {
            set { swing = value; }
        }

        Vector3 LeftHandPosition {
            get { return leftHand; }
        }
        Vector3 RightHandPosition {
            get { return rightHand; }
        }

        Vector3 BlockColor {
            set { blockColor = value; }
        }

        float ReadyState {
            set { readyState = value; }
        }

        private Renderer @renderer;
        private AudioDevice @audioDevice;
        private Model @model;
        private Image @sightImage;

        ViewBlockSkin(Renderer @r, AudioDevice @dev) {
            @renderer = r;
            @audioDevice = dev;
            @model = renderer.RegisterModel("Models/Weapons/Block/Block2.kv6");
            @sightImage = renderer.RegisterImage("Gfx/Sight.tga");
        }

        void Update(float dt) {
            float sprintStateSS = sprintState * sprintState;
            if (sprintStateSS > sprintStateSmooth) {
                sprintStateSmooth += (sprintStateSS - sprintStateSmooth) * (1.f - pow(0.001, dt));
            } else {
                sprintStateSmooth = sprintStateSS;
            }
        }

        void AddToScene() {
            if (readyState < .99f) {
                // not ready
                leftHand = Vector3(0.5f, 0.5f, 0.6f);
                rightHand = Vector3(-0.5f, 0.5f, 0.6f);
                return;
            }

            Matrix4 mat = CreateScaleMatrix(0.033f);

            if (sprintStateSmooth > 0.f) {
                mat = CreateRotateMatrix(Vector3(0.f, 0.f, 1.f), sprintStateSmooth * -0.3f) * mat;
                mat = CreateTranslateMatrix(Vector3(0.1f, -0.4f, -0.05f) * sprintStateSmooth) * mat;
            }

            mat = CreateTranslateMatrix(-0.3f, 0.7f, 0.3f) * mat;
            mat = CreateTranslateMatrix(swing) * mat;

            mat = CreateTranslateMatrix(Vector3(-0.1f, -0.3f, 0.2f) * (1.f - raiseState)) * mat;

            leftHand = mat * Vector3(5.f, -1.f, 4.f);
            rightHand = mat * Vector3(-5.5f, 3.f, -5.f);

            ModelRenderParam param;
            param.matrix = eyeMatrix * mat;
            param.customColor = blockColor;
            param.depthHack = true;
            renderer.AddModel(model, param);
        }

        void Draw2D() {
            renderer.ColorNP = (Vector4(1.f, 1.f, 1.f, 1.f));
            renderer.DrawImage(sightImage,
                               Vector2((renderer.ScreenWidth - sightImage.Width) * 0.5f,
                                       (renderer.ScreenHeight - sightImage.Height) * 0.5f));
        }
    }

    IBlockSkin @CreateViewBlockSkin(Renderer @r, AudioDevice @dev) { return ViewBlockSkin(r, dev); }
}
