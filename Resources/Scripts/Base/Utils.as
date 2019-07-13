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
    // AngelScript doesn't seem to support user-defined template functions...

    uint Min(uint a, uint b) { return (a < b) ? a : b; }

    uint Max(uint a, uint b) { return (a > b) ? a : b; }

    int Min(int a, int b) { return (a < b) ? a : b; }

    int Max(int a, int b) { return (a > b) ? a : b; }

    float Min(float a, float b) { return (a < b) ? a : b; }

    float Max(float a, float b) { return (a > b) ? a : b; }

    float Clamp(float val, float lower, float upper) { return Min(Max(val, lower), upper); }

    int Clamp(int val, int lower, int upper) { return Min(Max(val, lower), upper); }

    uint Clamp(uint val, uint lower, uint upper) { return Min(Max(val, lower), upper); }

    /** Renders an image in the way CSS border-image does. */
    void DrawSliceImage(Renderer @renderer, Image @image, float x, float y, float w, float h,
                        float border) {
        float iw = image.Width;
        float ih = image.Height;
        renderer.DrawImage(image, AABB2(x, y, border, border), AABB2(0.f, 0.f, border, border));
        renderer.DrawImage(image, AABB2(x + w - border, y, border, border),
                           AABB2(iw - border, 0.f, border, border));
        renderer.DrawImage(image, AABB2(x, y + h - border, border, border),
                           AABB2(0.f, ih - border, border, border));
        renderer.DrawImage(image, AABB2(x + w - border, y + h - border, border, border),
                           AABB2(iw - border, ih - border, border, border));
        renderer.DrawImage(image, AABB2(x + border, y, w - border - border, border),
                           AABB2(border, 0.f, iw - border - border, border));
        renderer.DrawImage(image, AABB2(x + border, y + h - border, w - border - border, border),
                           AABB2(border, ih - border, iw - border - border, border));
        renderer.DrawImage(image, AABB2(x, y + border, border, h - border - border),
                           AABB2(0.f, border, border, ih - border - border));
        renderer.DrawImage(image, AABB2(x + w - border, y + border, border, h - border - border),
                           AABB2(iw - border, border, border, ih - border - border));
        renderer.DrawImage(image,
                           AABB2(x + border, y + border, w - border - border, h - border - border),
                           AABB2(border, border, iw - border - border, ih - border - border));
    }

    /** Returns the byte index for a certain character index. */
    int GetByteIndexForString(string s, int charIndex, int start = 0) {
        int len = s.length;
        while (start < len && charIndex > 0) {
            int c = s[start];
            if ((c & 0x80) == 0) {
                charIndex--;
                start += 1;
            } else if ((c & 0xe0) == 0xc0) {
                charIndex--;
                start += 2;
            } else if ((c & 0xf0) == 0xe0) {
                charIndex--;
                start += 3;
            } else if ((c & 0xf8) == 0xf0) {
                charIndex--;
                start += 4;
            } else if ((c & 0xfc) == 0xf8) {
                charIndex--;
                start += 5;
            } else if ((c & 0xfe) == 0xfc) {
                charIndex--;
                start += 6;
            } else {
                // invalid!
                charIndex--;
                start++;
            }
        }

        if (start > len)
            start = len;
        return start;
    }

    /** Returns the byte index for a certain character index. */
    int GetCharIndexForString(string s, int byteIndex, int start = 0) {
        int len = s.length;
        int charIndex = 0;
        while (start < len && start < byteIndex && byteIndex > 0) {
            int c = s[start];
            if ((c & 0x80) == 0) {
                charIndex++;
                start += 1;
            } else if ((c & 0xe0) == 0xc0) {
                charIndex++;
                start += 2;
            } else if ((c & 0xf0) == 0xe0) {
                charIndex++;
                start += 3;
            } else if ((c & 0xf8) == 0xf0) {
                charIndex++;
                start += 4;
            } else if ((c & 0xfc) == 0xf8) {
                charIndex++;
                start += 5;
            } else if ((c & 0xfe) == 0xfc) {
                charIndex++;
                start += 6;
            } else {
                // invalid!
                charIndex++;
                start++;
            }
        }

        return charIndex;
    }
}
