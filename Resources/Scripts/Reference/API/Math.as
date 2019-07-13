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

// FIXME: should be written about math functions of AngelScript add-on

namespace spades {

    /** Does a linear interpolation. */
    float Mix(float a, float b, float f) {}

    /** Does a linear interpolation. */
    Vector2 Mix(Vector2 a, Vector2 b, float f) {}

    /** Does a linear interpolation. */
    Vector3 Mix(Vector3 a, Vector3 b, float f) {}

    /** Applies an smooth-step function. */
    float SmoothStep(float value) {}

    /** Retrieves a random value that lies in [0, 1]. */
    float GetRandom() {}

    /** Represents a 3-component integral vector. */
    class IntVector3 {
        int x, y, z;

        /** Default constructor. */
        IntVector3() {}

        /** Initializes new instance of IntVector3 with default values given. */
        IntVector3(const IntVector3 @other) {}

        /** Initializes new instance of IntVector3 with default values given. */
        IntVector3(int x, int y, int z) {}

        /** Initializes new instance of IntVector3 with default values given. */
        IntVector3(const Vector3 @other) {}

        /** Adds the given vector to this one, and returns this. */
        IntVector3 @opAddAssign(const IntVector3 @other) {}

        /** Subtracts the given vector from this one, and returns this. */
        IntVector3 @opSubAssign(const IntVector3 @other) {}

        /** Multiplies this vector by the given one, and returns this one. */
        IntVector3 @opMulAssign(const IntVector3 @other) {}

        /** Divides this vector by the given one, and returns this one. */
        IntVector3 @opDivAssign(const IntVector3 @other) {}

        /** Adds the given vector to this one, and returns a new vector. */
        IntVector3 opAdd(const IntVector3 @other) const {}

        /** Subtracts the given vector from this one, and returns a new vector. */
        IntVector3 opSub(const IntVector3 @other) const {}

        /** Multiplies this vector by the given one, and returns a new vector. */
        IntVector3 opMul(const IntVector3 @other) const {}

        /** Divides this vector by the given one, and returns a new vector. */
        IntVector3 opDiv(const IntVector3 @other) const {}

        /** Returns the negated vector of this. */
        IntVector3 opNeg() const {}

        /** Computes a manhattan length of this vector. */
        int ManhattanLength {
            get const {}
        }

        /** Computes a chebyshev length of this vector. */
        int ChebyshevLength {
            get const {}
        }
    }

    /** Computes a dot-product of two vectors. */
    int Dot(const IntVector3 @, const IntVector3 @);

    /** Represents a 2-component real vector. */
    class Vector2 {
        float x, y;

        /** Default constructor. */
        Vector2() {}

        /** Initializes new instance of Vector2 with default values given. */
        Vector2(const Vector2 @other) {}

        /** Initializes new instance of Vector2 with default values given. */
        Vector2(float x, float y) {}

        /** Adds the given vector to this one, and returns this. */
        Vector2 @opAddAssign(const Vector2 @other) {}

        /** Subtracts the given vector from this one, and returns this. */
        Vector2 @opSubAssign(const Vector2 @other) {}

        /** Multiplies this vector by the given one, and returns this one. */
        Vector2 @opMulAssign(const Vector2 @other) {}

        /** Divides this vector by the given one, and returns this one. */
        Vector2 @opDivAssign(const Vector2 @other) {}

        /** Adds the given vector to this one, and returns a new vector. */
        Vector2 opAdd(const Vector2 @other) const {}

        /** Subtracts the given vector from this one, and returns a new vector. */
        Vector2 opSub(const Vector2 @other) const {}

        /** Multiplies this vector by the given one, and returns a new vector. */
        Vector2 opMul(const Vector2 @other) const {}

        /** Divides this vector by the given one, and returns a new vector. */
        Vector2 opDiv(const Vector2 @other) const {}

        /** Returns the negated vector of this. */
        Vector2 opNeg() const {}

        /** Returns the normalized vector of this. */
        Vector2 Normalized {
            get const {}
        }

        /** Computes a manhattan length of this vector. */
        float ManhattanLength {
            get const {}
        }

        /** Computes a chebyshev length of this vector. */
        float ChebyshevLength {
            get const {}
        }

        /** Computes an euclidean length of this vector. */
        float Length {
            get const {}
        }

        /** Computes the second power of an euclidean length of this vector. */
        float LengthPowered {
            get const {}
        }
    }

    /** Computes a dot-product of two vectors. */
    float Dot(const Vector2 @, const Vector2 @);

    /** Represents a 3-component real vector. */
    class Vector3 {
        float x, y, z;

        /** Default constructor. */
        Vector3() {}

        /** Initializes new instance of Vector3 with default values given. */
        Vector3(const Vector3 @other) {}

        /** Initializes new instance of Vector3 with default values given. */
        Vector3(float x, float y, float z) {}

        /** Initializes new instance of Vector3 with default values given. */
        Vector3(const IntVector3 @other) {}

        /** Adds the given vector to this one, and returns this. */
        Vector3 @opAddAssign(const Vector3 @other) {}

        /** Subtracts the given vector from this one, and returns this. */
        Vector3 @opSubAssign(const Vector3 @other) {}

        /** Multiplies this vector by the given one, and returns this one. */
        Vector3 @opMulAssign(const Vector3 @other) {}

        /** Divides this vector by the given one, and returns this one. */
        Vector3 @opDivAssign(const Vector3 @other) {}

        /** Adds the given vector to this one, and returns a new vector. */
        Vector3 opAdd(const Vector3 @other) const {}

        /** Subtracts the given vector from this one, and returns a new vector. */
        Vector3 opSub(const Vector3 @other) const {}

        /** Multiplies this vector by the given one, and returns a new vector. */
        Vector3 opMul(const Vector3 @other) const {}

        /** Divides this vector by the given one, and returns a new vector. */
        Vector3 opDiv(const Vector3 @other) const {}

        /** Returns the negated vector of this. */
        Vector3 opNeg() const {}

        /** Returns the normalized vector of this. */
        Vector3 Normalized {
            get const {}
        }

        /** Computes a manhattan length of this vector. */
        float ManhattanLength {
            get const {}
        }

        /** Computes a chebyshev length of this vector. */
        float ChebyshevLength {
            get const {}
        }

        /** Computes an euclidean length of this vector. */
        float Length {
            get const {}
        }

        /** Computes the second power of an euclidean length of this vector. */
        float LengthPowered {
            get const {}
        }
    }

    /** Computes a dot-product of two vectors. */
    float Dot(const Vector3 @, const Vector3 @);

    /** Computes a cross-product of two vectors. */
    Vector3 Cross(const Vector3 @, const Vector3 @);

    /**
     * Applies the floor function to the given vector, and
     *  returns the computed one.
     */
    Vector3 Floor(const Vector3 @);

    /**
     * Applies the ceiling function to the given vector, and
     *  returns the computed one.
     */
    Vector3 Ceil(const Vector3 @);

    /** Represents a 4-component real vector. */
    class Vector4 {
        float x, y, z, w;

        /** Default constructor. */
        Vector4() {}

        /** Initializes new instance of Vector4 with default values given. */
        Vector4(const Vector4 @other) {}

        /** Initializes new instance of Vector4 with default values given. */
        Vector4(float x, float y, float z, float w) {}

        /** Initializes new instance of Vector4 with default values given. */
        Vector4(const IntVector4 @other) {}

        /** Adds the given vector to this one, and returns this. */
        Vector4 @opAddAssign(const Vector4 @other) {}

        /** Subtracts the given vector from this one, and returns this. */
        Vector4 @opSubAssign(const Vector4 @other) {}

        /** Multiplies this vector by the given one, and returns this one. */
        Vector4 @opMulAssign(const Vector4 @other) {}

        /** Divides this vector by the given one, and returns this one. */
        Vector4 @opDivAssign(const Vector4 @other) {}

        /** Adds the given vector to this one, and returns a new vector. */
        Vector4 opAdd(const Vector4 @other) const {}

        /** Subtracts the given vector from this one, and returns a new vector. */
        Vector4 opSub(const Vector4 @other) const {}

        /** Multiplies this vector by the given one, and returns a new vector. */
        Vector4 opMul(const Vector4 @other) const {}

        /** Divides this vector by the given one, and returns a new vector. */
        Vector4 opDiv(const Vector4 @other) const {}

        /** Returns the negated vector of this. */
        Vector4 opNeg() const {}

        /** Returns the normalized vector of this. */
        Vector4 Normalized {
            get const {}
        }

        /** Computes a manhattan length of this vector. */
        float ManhattanLength {
            get const {}
        }

        /** Computes a chebyshev length of this vector. */
        float ChebyshevLength {
            get const {}
        }

        /** Computes an euclidean length of this vector. */
        float Length {
            get const {}
        }

        /** Computes the second power of an euclidean length of this vector. */
        float LengthPowered {
            get const {}
        }
    }

    /** Computes a dot-product of two vectors. */
    float Dot(const Vector4 @, const Vector4 @);

    /**
     * Applies the floor function to the given vector, and
     *  returns the computed one.
     */
    Vector4 Floor(const Vector4 @);

    /**
     * Applies the ceiling function to the given vector, and
     *  returns the computed one.
     */
    Vector4 Ceil(const Vector4 @);

    /** Represents column-major 4x4 matrix. */
    class Matrix4 {
        private float m00, m10, m20, m30;
        private float m01, m11, m21, m31;
        private float m02, m12, m22, m32;
        private float m03, m13, m23, m33;

        /** Default constructor. */
        Matrix4() {}

        /** Copy constructor. */
        Matrix4(const Matrix4 @other) {}

        /** Initializes Matrix4 with the given values. */
        Matrix4(float m00, float m10, float m20, float m30, float m01, float m11, float m21,
                float m31, float m02, float m12, float m22, float m32, float m03, float m13,
                float m23, float m33) {}

        /** Multiplies this matrix by the given one, and returns this one. */
        Matrix4 @opMulAssign(const Matrix4 @other) {}

        /** Multiplies this matrix by the given one, and returns a matrix. */
        Matrix4 opMul(const Matrix4 @other) {}

        /** Multiplies this matrix by the given vector, and returns a vector. */
        Vector4 opMul(const Vector4 @other) {}

        /**
         * Multiplies this matrix by the vector [x, y, z, 1], and returns
         * a 3-component vector, discarding the fourth component.
         */
        Vector3 opMul(const Vector3 @other) {}

        /** Returns the transformed position of the vector [0, 0, 0]. */
        Vector3 GetOrigin() const {}

        /**
         * Returns the transformed orientation of the specified axis.
         * @param axis 0 for X-axis, 1 for Y-axis, and 2 for Z-axis.
         */
        Vector3 GetAxis(int axis) const {}

        /** Returns the transposed matrix. */
        Matrix4 Transposed {
            get const {}
        }

        /**
         * Returns the inverted matrix.
         * This operation works in O(N^3).
         */
        Matrix4 Inverted {
            get const {}
        }

        /** Returns the inverted matrix of an orthogonal one. */
        Matrix4 InvertedFast {
            get const {}
        }
    }

    /**
     * Creates an matrix that can be used to translate points by the
     *  specified displacement vector.
     */
    Matrix4 CreateTranslateMatrix(Vector3 displacement) {}

    /**
     * Creates an matrix that can be used to translate points by the
     *  specified displacement vector.
     */
    Matrix4 CreateTranslateMatrix(float x, float y, float z) {}

    /**
     * Creates an matrix that can be used to rotate points by the
     *  specified axis and rotation angle.
     */
    Matrix4 CreateRotateMatrix(Vector3 axis, float radians) {}

    /**
     * Creates an matrix that can be used to scale points by the
     *  specified vector.
     */
    Matrix4 CreateScaleMatrix(Vector3 factor) {}

    /**
     * Creates an matrix that can be used to scale points by the
     *  specified vector.
     */
    Matrix4 CreateScaleMatrix(float x, float y, float z) {}

    /**
     * Creates an matrix that can be used to scale points by the
     *  specified factor.
     */
    Matrix4 CreateScaleMatrix(float factor) {}

    /** Creates an matrix using the given axis vector. */
    Matirx4 CreateMatrixFromAxes(Vector3 axisX, Vector3 axisY, Vector3 axisZ, Vector3 origin);

    /** Represents a two-dimensional AABB (axis-aligned bounding box). */
    class AABB2 {
        Vector2 min, max;
        float minX {
            get const {}
            set {}
        }
        float maxX {
            get const {}
            set {}
        }
        float minY {
            get const {}
            set {}
        }
        float maxY {
            get const {}
            set {}
        }

        /** Default constructor. */
        AABB2() {}

        /** Copy constructor. */
        AABB2(const AABB2 @) {}

        /**
         * Initializes AABB2 with the top-left coordinate and the
         * dimensions of the bounding box.
         */
        AABB2(float left, float top, float width, float height) {}

        /**
         * Initializes AABB2 with the top-left coordinate and the
         * bottom-right one.
         */
        AABB2(Vector2 min, Vector2 max) {}

        /** Checks if the given vector is in the bounding box. */
        bool Contains(const Vector2 @point) {}

        /** Checks if the given box intersects with this one. */
        bool Intersects(const AABB2 @box) {}

        /** Extends this bounding box to include the given point. */
        void Add(const Vector2 @point) {}

        /** Extends this bounding box to include the given one. */
        void Add(const AABB2 @box) {}
    }

}
