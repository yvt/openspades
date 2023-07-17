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

    /** Renderer is an interface to the display device. */
    class Renderer {

        /** Initializes the renderer for 3D scene rendering. */
        void Init() {}

        /** Shuts down the renderer. */
        void Shutdown() {}

        /**
         * Loads an image from the specified path or load one from
         * the cache, if exists.
         * @param path file-system path.
         */
        Image @RegisterImage(const string @path) {}

        /**
         * Loads an model from the specified path or load one from
         * the cache, if exists.
         * @param path file-system path.
         */
        Model @RegisterModel(const string @path) {}

        /**
         * Clear the cache of models and images loaded via `RegisterModel`
         * and `RegisterImage`. This method is merely a hint - the
         * implementation may partially or completely ignore the request.
         */
        void ClearCache() {}

        /** Creates an image from the specified bitmap. */
        Image @CreateImage(Bitmap @bitmap) {}

        /** Creates an model from the specified voxel model. */
        Model @CreateModel(VoxelModel @voxelmap) {}

        /** Sets a game world map. */
        GameMap @GameMap {
            set {}
        }

        /** Changes the fog end distance (not supported). */
        float FogDistance {
            set {}
        }

        /** Changes the color of fog. */
        Vector3 FogColor {
            set {}
        }

        /**
         * Sets a color that is used for drawing.
         * @deprecated Do not use this virtual property.
         *             Some methods treat this value as an alpha premultiplied,
         *             while others treat this value has a straight alpha.
         */
        Vector4 Color {
            set {}
        }

        /** Sets a opaque color that is used for drawing. */
        Vector3 ColorOpaque {
            set {}
        }

        /**
         * Sets a color that is used for drawing. The color value is
         * alpha premultiplied.
         */
        Vector4 ColorP {
            set {}
        }

        /**
         * Sets a color that is used for drawing. The color value is
         * alpha non-premultiplied (straight alpha).
         */
        Vector4 ColorNP {
            set {}
        }

        /** Starts a 3D scene rendering. */
        void StartScene(const SceneDefinition @) {}

        /**
         * Adds a dynamic light to the scene.
         * This should be called between StartScene and EndScene.
         */
        void AddLight(const DynamicLightParam @) {}

        /**
         * Adds a model to the scene.
         * This should be called between StartScene and EndScene.
         */
        void AddModel(Model @, const ModelRenderParam @) {}

        /**
         * Adds a line for debugging.
         * This should be called between StartScene and EndScene.
         */
        void AddDebugLine(const Vector3 @pt1, const Vector3 @pt2, const Vector4 @color) {}

        /**
         * Adds a sprite.
         * This should be called between StartScene and EndScene.
         * The color is specified using the property `Color` with
         * alpha values premultiplied.
         */
        void AddSprite(Image @, const Vector3 @origin, float radius, float rotateRadians) {}

        /**
         * Adds a long-sprite.
         * This should be called between StartScene and EndScene.
         * The color is specified using the property `Color` with
         * alpha values premultiplied.
         */
        void AddLongSprite(Image @, const Vector3 @pt1, const Vector3 @pt2, float radius) {}

        /** Ends a 3D scene rendering. */
        void EndScene() {}

        /**
         * Multiplies color of the screen by the specified color.
         * This should never be called between StartScene and EndScene.
         */
        void MultiplyScreenColor(Vector3 color) {}

        /**
         * Draws a 2D image.
         * This should never be called between StartScene and EndScene.
         */
        void DrawImage(Image @, const Vector2 @topLeft) {}

        /**
         * Draws a 2D image.
         * This should never be called between StartScene and EndScene.
         */
        void DrawImage(Image @, const AABB2 @outRect) {}

        /**
         * Draws a 2D image.
         * This should never be called between StartScene and EndScene.
         */
        void DrawImage(Image @, const AABB2 @outRect, const AABB2 @inRect) {}

        /**
         * Draws a 2D image.
         * This should never be called between StartScene and EndScene.
         */
        void DrawImage(Image @, const Vector2 @outTopLeft, const Vector2 @outTopRight,
                       const Vector2 @outBottomLeft, const AABB2 @inRect) {}

        /** Finalizes the drawing. */
        void FrameDone();

        /** Transfers the rendered image to the user's display. */
        void Flip();

        /** Reads the rendered image to a bitmap. */
        Bitmap @ReadBitmap();

        /** Retrieves the width of the screen, in pixels. */
        float ScreenWidth {
            get {}
        }

        /** Retrieves the height of the screen, in pixels. */
        float ScreenHeight {
            get {}
        }
    }

    class SceneDefinition {
        int viewportLeft, viewportTop;
        int viewportWidth, viewportHeight;

        float fovX, fovY;

        Vector3 viewOrigin;
        Vector3 viewAxisX;
        Vector3 viewAxisY;
        Vector3 viewAxisZ;

        float zNear, zFar;

        bool skipWorld;
        uint time;

        float depthOfFieldFocalLength;
        float depthOfFieldNearBlurStrength;
        float depthOfFieldFarBlurStrength;
        bool denyCameraBlur;
        float blurVignette;
    }

    class ModelRenderParam {
        Matrix4 matrix;
        Vector3 customColor;
        bool depthHack;
        bool castShadow;
        bool unlit;
    }

    class DynamicLightParam {
        DynamicLightType type;
        Vector3 origin;
        Vector3 point2;
        float radius;
        Vector3 color;

        Vector3 spotAxisX;
        Vector3 spotAxisY;
        Vector3 spotAxisZ;
        Image @image;
        float spotAngle;

        bool useLensFlare;
    }

}
