#pragma once
#include <cmath>

struct Vec3 {
    float x, y, z;
    Vec3(float a=0, float b=0, float c=0): x(a), y(b), z(c) {}
    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3 operator*(float s) const { return Vec3(x*s, y*s, z*s); }
    Vec3 norm() const { float l=sqrtf(x*x+y*y+z*z); return l>0?Vec3(x/l,y/l,z/l):Vec3(); }
    float dot(const Vec3& v) const { return x*v.x+y*v.y+z*v.z; }
    Vec3 cross(const Vec3& v) const { return Vec3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
    float len() const { return sqrtf(x*x+y*y+z*z); }
};

struct Mat4 {
    float m[16];
    Mat4() { for(int i=0;i<16;i++)m[i]=0; m[0]=m[5]=m[10]=m[15]=1; }
    
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for(int i=0;i<16;i++)r.m[i]=0;
        for(int i=0;i<4;i++)
            for(int j=0;j<4;j++)
                for(int k=0;k<4;k++)
                    r.m[i*4+j] += m[i*4+k] * o.m[k*4+j];
        return r;
    }
    
    static Mat4 translate(float x, float y, float z) {
        Mat4 r;
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }
    
    static Mat4 persp(float fov, float asp, float n, float f) {
        Mat4 r; for(int i=0;i<16;i++)r.m[i]=0;
        float t=tanf(fov/2);
        r.m[0]=1/(asp*t); r.m[5]=1/t;
        r.m[10]=-(f+n)/(f-n); r.m[11]=-1;
        r.m[14]=-(2*f*n)/(f-n);
        return r;
    }
    
    static Mat4 look(Vec3 e, Vec3 c, Vec3 u) {
        Vec3 f=(c-e).norm(), s=f.cross(u).norm(), up=s.cross(f);
        Mat4 r;
        r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;
        r.m[1]=up.x; r.m[5]=up.y; r.m[9]=up.z;
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
        r.m[12]=-s.dot(e); r.m[13]=-up.dot(e); r.m[14]=f.dot(e);
        return r;
    }
};