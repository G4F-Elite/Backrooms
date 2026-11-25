#pragma once
#include "math.h"

inline Mat4 composeModelMatrix(const Vec3& pos, float yaw, float pitch, const Vec3& scale){
    Vec3 forward(mSin(yaw) * mCos(pitch), mSin(pitch), mCos(yaw) * mCos(pitch));
    forward.normalize();

    Vec3 worldUp(0.0f, 1.0f, 0.0f);
    Vec3 right = worldUp.cross(forward);
    if(right.lenSq() < 0.0001f){
        right = Vec3(mCos(yaw), 0.0f, -mSin(yaw));
    }else{
        right.normalize();
    }
    Vec3 up = forward.cross(right).norm();

    Mat4 r;
    r.m[0] = right.x * scale.x;
    r.m[1] = right.y * scale.x;
    r.m[2] = right.z * scale.x;
    r.m[3] = 0.0f;

    r.m[4] = up.x * scale.y;
    r.m[5] = up.y * scale.y;
    r.m[6] = up.z * scale.y;
    r.m[7] = 0.0f;

    r.m[8] = forward.x * scale.z;
    r.m[9] = forward.y * scale.z;
    r.m[10] = forward.z * scale.z;
    r.m[11] = 0.0f;

    r.m[12] = pos.x;
    r.m[13] = pos.y;
    r.m[14] = pos.z;
    r.m[15] = 1.0f;
    return r;
}
