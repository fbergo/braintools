#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include <utility>
#include <format>

namespace libbi {

    //! Point or Vector in 3D space.
    //! Point/Vector coordinates are stored as 32-bit floating point.
    class R3 {
     public:  
      float x,y,z;
    
      //! creates a point at the origin
      R3() { x=y=z=0.0f; }
      //! Constructor, creates a point at the given coordinates
      R3(float _x,float _y,float _z) { x=_x; y=_y; z=_z; }

      R3(int _x,int _y,int _z) { x=(float)_x; y=(float)_y; z=(float)_z; }

      //! adds b to the point
      R3 & operator+=(const R3 &b) { x+=b.x; y+=b.y; z+=b.z; return(*this); }
    
      //! subtracts b from the point
      R3 & operator-=(const R3 &b) { x-=b.x; y-=b.y; z-=b.z; return(*this); }
    
      //! multiplies the point by a scalar
      R3 & operator*=(float b) { x*=b; y*=b; z*=b; return(*this); }
    
      //! divides the point by a scalar
      R3 & operator/=(float b) { x/=b; y/=b; z/=b; return(*this); }
    
      //! vector addition
      R3 operator+(const R3 &b) const { return(R3(x+b.x,y+b.y,z+b.z)); }
  
      //! vector subtraction
      R3 operator-(const R3 &b) const { return(R3(x-b.x,y-b.y,z-b.z)); }
    
      //! vector multiplication by a scalar
      R3 operator*(float b) const { return(R3(x*b,y*b,z*b)); }
    
      //! vector division by a scalar
      R3 operator/(float b) const { return(R3(x/b,y/b,z/b)); } 

      bool operator==(const R3 &b) const { return(x==b.x && y==b.y && z==b.z); }
     
      //! vector inner product
      float inner(const R3 &b) const { return(x*b.x+y*b.y+z*b.z); }
    
      //! angle between two vectors
      float angle(const R3 &b) const { 
        return(std::acos(std::clamp(inner(b) / (length() * b.length()), -1.0f, 1.0f))); 
      }
    
      //! Returns the vector cross product of this x b. Does not modify this point.
      R3 cross(const R3 &b) const {
        return(R3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x));
      }
    
      //! sets the coordinates of the point
      void set(float _x, float _y, float _z) { x=_x; y=_y; z=_z; }
    
      //! returns the length of the vector
      float length() const { return(std::sqrt(x*x+y*y+z*z)); }
    
      //! normalizes this vector to unit-length
      void  normalize() { float l=length(); if (l != 0.0f) (*this)/=l; }
    
      std::string to_string() const { return(std::format("R3=({:.2f}, {:.2f}, {:.2f})",x,y,z)); }
      
      std::string to_string_d() const { return(std::format("R3=({}, {}, {})",x,y,z)); }
      
      //! applies usqrt to all components
      void usqrt() { x = usqrt(x); y = usqrt(y); z = usqrt(z); }
    
     private:
      float usqrt(float a) const {
        if (a<0.0f) return(-std::sqrt(-a)); else return(std::sqrt(a));
      }
    };
      
    //! 3x3 Linear Transformation.
    //! 3x3 Linear Transformation, which does not allow translations.
    class T3 {
      public:
        T3();
        
        //! Transform composition, multiplies this transform by b, and overwrites
        //! this transform. Returns a reference to this transform.
        T3 & operator*=(const T3 & b);
        
        //! composition
        T3 operator*(const T3 &b) const;
        
        //! Applies this transform to point a and returns the transformed point
        R3 apply(const R3 & a) const;
        
        //! Sets all coefficients to zero (null transform)
        void zero();
        
        //! Sets an identity transform
        void identity();
        
        //! Sets a rotation around X-axis transform, by angle degrees.
        //! Rotation center is the origin.
        void xrot(float angle);
        
        //! Sets a rotation around Y-axis transform, by angle degrees.
        //! Rotation center is the origin.
        void yrot(float angle);
        
        //! Sets a rotation around Z-axis transform, by angle degrees.
        //! Rotation center is the origin.
        void zrot(float angle);
        
        //! Sets a scaling transform, by factor on all dimensions
        void scale(float factor);
        
        //! Sets a scaling transform, with different scaling factors
        //! for each dimension.
        void scale(float fx, float fy, float fz);
        
        //! string representation
        std::string to_string() const;
        
      private:
        float e[9];
    };

    //! 4x4 linear transform with float precision
    class T4 {
        public:
            T4();
            void zero();
            void identity();
            T4 & operator*=(const T4 & b);
            T4   operator*(const T4 &b) const; 
            bool equals(const T4 &b, float epsilon=0.0001f) const;

            R3   apply(const R3 & a) const;

            void xrot(float angle, float cx, float cy, float cz);
            void yrot(float angle, float cx, float cy, float cz);
            void zrot(float angle, float cx, float cy, float cz);
            void xrot(float angle);
            void yrot(float angle);
            void zrot(float angle);

            void axisrot(const R3 &axis, float angle, float cx, float cy, float cz);
            void axisrot(const R3 &axis, float angle);

            void xshear(float yf, float zf);
            void yshear(float xf, float zf);
            void zshear(float xf, float yf);

            void scale(float factor);
            void scale(float fx, float fy, float fz);

            void translate(float dx, float dy, float dz);

            void set(std::initializer_list<float> coefs);

            void invert();

            std::string to_string() const;

        private:
            float e[16];

            bool minv4();

    };
  
}