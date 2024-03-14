#pragma once

#include "MathUtils.h"

/**
* Vec2
*/
class Vec2
{
public:
	Vec2();
	Vec2(const float Value);
	Vec2(float X, float Y);
	Vec2(const Vec2& rhs);
	Vec2(const float* xy);
	Vec2& operator=(const Vec2& rhs);

	bool operator==(const Vec2& rhs)const;
	bool operator!=(const Vec2& rhs)const;
	Vec2 operator+(const Vec2& rhs)const;
	const Vec2& operator+=(const Vec2& rhs);
	const Vec2& operator-=(const Vec2& rhs);
	Vec2 operator-(const Vec2& rhs)const;
	Vec2 operator*(const float rhs)const;
	const Vec2& operator*=(const float rhs);
	const Vec2& operator/=(const float rhs);
	float operator[](const int idx)const;
	float& operator[](const int idx);

	const Vec2& Normalize();
	float GetMagnitude() const;
	bool IsValid()const;
	float Dot(const Vec2& rhs)const { return x * rhs.x + y * rhs.y; }

	const float* ToPtr()const { return &x; }

public:
	float x;
	float y;
};

inline Vec2::Vec2() :
	x(0), y(0) {}

inline Vec2::Vec2(const float value) :
	x(value), y(value) {}

inline Vec2::Vec2(const Vec2& rhs) :
	x(rhs.x), y(rhs.y) {}

inline Vec2::Vec2(float X, float Y) :
	x(X), y(Y) {};

inline Vec2::Vec2(const float* xy) :
	x(xy[0]), y(xy[1]) {}

inline Vec2& Vec2::operator=(const Vec2& rhs)
{
	x = rhs.x;
	y = rhs.y;
	return *this;
}

inline bool Vec2::operator==(const Vec2& rhs)const
{
	return x == rhs.x && y == rhs.y;
}

inline bool Vec2::operator!=(const Vec2& rhs)const
{
	return x != rhs.x || y != rhs.y;
}

inline Vec2 Vec2::operator+(const Vec2& rhs)const
{
	return Vec2(x + rhs.x, y + rhs.y);
}

inline const Vec2& Vec2::operator+=(const Vec2& rhs)
{
	x += rhs.x;
	y += rhs.y;
	return *this;
}

inline const Vec2& Vec2::operator-=(const Vec2& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	return *this;
}

inline Vec2 Vec2::operator-(const Vec2& rhs)const
{
	return Vec2(x - rhs.x, y - rhs.y);
}

inline Vec2 Vec2::operator*(const float rhs)const
{
	return Vec2(x * rhs, y * rhs);
}

inline const Vec2& Vec2::operator*=(const float rhs)
{
	x *= rhs;
	y *= rhs;
	return *this;
}

inline const Vec2& Vec2::operator/=(const float rhs)
{
	x /= rhs;
	y /= rhs;
	return *this;
}

inline float Vec2::operator[](const int idx)const
{
	assert(idx >= 0 && idx < 2);
	return (&x)[idx];
}

inline float& Vec2::operator[](const int idx)
{
	assert(idx >= 0 && idx < 2);
	return (&x)[idx];
}

inline const Vec2& Vec2::Normalize()
{
	float mag = GetMagnitude();
	float invMag = 1.f / mag;
	if (0.f * invMag == 0.f * invMag)
	{
		x *= invMag;
		y *= invMag;
	}

	return *this;
}

inline float Vec2::GetMagnitude()const
{
	float mag;

	mag = x * x + y * y;
	mag = sqrtf(mag);

	return mag;
}

inline bool Vec2::IsValid()const
{
	return (x * 0.0f == x * 0.0f) && (y * 0.0f == y * 0.0f);
}

/**
* Vec3
*/
class Vec3
{
public:
	Vec3() :x(0), y(0), z(0) {}
	Vec3(float value) :x(value), y(value), z(value) {}
	Vec3(const Vec3& rhs) :x(rhs.x), y(rhs.y), z(rhs.z) {}
	Vec3(float X, float Y, float Z) :x(X), y(Y), z(Z) {}
	Vec3(const float* xyz) :x(xyz[0]), y(xyz[1]), z(xyz[1]) {}

	Vec3& operator=(const Vec3& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}
	
	Vec3& operator=(const float* xyz)
	{
		x = xyz[0];
		y = xyz[1];
		z = xyz[2];

		return *this;
	}

	bool operator==(const Vec3& rhs)const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const Vec3& rhs)const { return !(*this == rhs); }
	Vec3 operator+(const Vec3& rhs)const { return Vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
	const Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	const Vec3& operator-=(const Vec3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	Vec3 operator-(const Vec3& rhs)const { return Vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
	Vec3 operator*(const float rhs)const { return Vec3(x * rhs, y * rhs, z * rhs); }
	Vec3 operator/(const float rhs)const { return Vec3(x / rhs, y / rhs, z / rhs); }
	const Vec3& operator*=(const float rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	const Vec3& operator/=(const float rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }
	float operator[](const int idx)const { assert(idx >= 0 && idx <= 2); return (&x)[idx]; }
	float& operator[](const int idx) { assert(idx >= 0 && idx <= 2); return (&x)[idx]; }

	void Zero() { x = 0.f; y = 0.f; z = 0.f; }

	Vec3 Cross(const Vec3& rhs)const;
	float Dot(const Vec3& rhs)const;

	const Vec3& Normalize();
	float GetMagnitude()const;
	float GetLengthSquare()const { return Dot(*this); }
	bool IsValid()const;
	void GetOrtho(Vec3& u, Vec3& v)const;

	const float* ToPtr()const { return &x; }

public:
	float x, y, z;
};

inline Vec3 Vec3::Cross(const Vec3& rhs)const
{
	return Vec3(y * rhs.z - rhs.y * z, z * rhs.x - rhs.z * x, x * rhs.y - rhs.x * y);
}

inline float Vec3::Dot(const Vec3& rhs)const
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

inline const Vec3& Vec3::Normalize()
{
	float mag = GetMagnitude();
	float invMag = 1.f / mag;
	if (0.0f * invMag == 0.0f * invMag)
	{
		x *= invMag;
		y *= invMag;
		z *= invMag;
	}
	return *this;
}

inline float Vec3::GetMagnitude()const
{
	return sqrtf(x * x + y * y + z * z);
}

inline bool Vec3::IsValid()const
{
	return (x * 0.0f == x * 0.0f) && (y * 0.0f == y * 0.0f) && (z * 0.0f == z * 0.0f);
}

inline void Vec3::GetOrtho(Vec3& u, Vec3& v)const
{
	Vec3 n = *this;
	n.Normalize();

	const Vec3 w = (n.z * n.z > 0.9f * 0.9f) ? Vec3(1, 0, 0) : Vec3(0, 0, 1);
	u = w.Cross(n);
	u.Normalize();
	v = n.Cross(u);
	v.Normalize();

	u = v.Cross(n);
	u.Normalize();
}

/**
* Vec4
*/
class Vec4
{
public:
	Vec4() :x(0), y(0), z(0), w(0) {}
	Vec4(const float value) :x(value), y(value), z(value), w(value) {}
	Vec4(const Vec4& rhs) :x(rhs.x), y(rhs.y), z(rhs.z), w(rhs.z) {}
	Vec4(float X, float Y, float Z, float W) :x(X), y(Y), z(Z), w(W) {}
	Vec4(const float* rhs) :x(rhs[0]), y(rhs[1]), z(rhs[2]), w(rhs[3]) {}
	Vec4& operator=(const Vec4& rhs) { x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w; return *this; }

	bool operator==(const Vec4& rhs)const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=(const Vec4& rhs)const { return !(*this == rhs); }
	Vec4 operator+(const Vec4& rhs)const { return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
	const Vec4& operator+=(const Vec4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
	const Vec4& operator-=(const Vec4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
	const Vec4& operator*=(const Vec4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }
	const Vec4& operator/=(const Vec4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; return *this; }
	Vec4 operator-(const Vec4& rhs)const { return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
	Vec4 operator*(const float rhs)const { return Vec4(x * rhs, y * rhs, z * rhs, w * rhs); }
	float operator[](const int idx)const { return  (&x)[idx]; }
	float& operator[](const int idx) { return (&x)[idx]; }

	float Dot(const Vec4& rhs)const;
	const Vec4& Normalize();
	float GetMagnitude()const;
	bool IsValid()const;
	void Zero() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }

	const float* ToPtr()const { return &x; }
	float* ToPtr() { return &x; }
public:
	float x, y, z, w;
};

inline float Vec4::Dot(const Vec4& rhs)const
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

inline const Vec4& Vec4::Normalize()
{
	float mag = GetMagnitude();
	float invMag = 1.f / mag;
	if (invMag * 0.0f == invMag * 0.0f)
	{
		x *= invMag;
		y *= invMag;
		z *= invMag;
		w *= invMag;
	}
	return *this;
}

inline float Vec4::GetMagnitude()const
{
	return sqrtf(x * x + y * y + z * z + w * w);
}

inline bool Vec4::IsValid()const
{
	return (0.0f * x == 0.0f * x) && (0.0f * y == 0.0f * y) && (0.0f * z == 0.0f * z) && (0.0f * w == 0.0f * w);
}

/**
* VecN
*/
class VecN
{
public:
	VecN() :N(0), data(nullptr) {}
	VecN(int _N);
	VecN(const VecN& rhs);
	VecN& operator=(const VecN& rhs);
	~VecN() { delete[] data; }

	float operator[](const int idx)const { return data[idx]; }
	float& operator[](const int idx) { return data[idx]; }
	const VecN& operator*=(float rhs);
	VecN operator*(float rhs)const;
	VecN operator+(const VecN& rhs)const;
	VecN operator-(const VecN& rhs)const;
	const VecN& operator+=(const VecN& rhs);
	const VecN& operator-=(const VecN& rhs);

	float Dot(const VecN& rhs)const;
	void Zero();
public:
	int N;
	float* data;
};

inline VecN::VecN(int _N)
{
	N = _N;
	data = new float[_N];
}

inline VecN::VecN(const VecN& rhs)
{
	N = rhs.N;
	data = new float[N];
	for (int i = 0; i < N; i++)
	{
		data[i] = rhs.data[i];
	}
}

inline VecN& VecN::operator=(const VecN& rhs)
{
	delete[] data;

	N = rhs.N;
	data = new float[N];
	for (int i = 0; i < N; i++)
	{
		data[i] = rhs.data[i];
	}

	return *this;
}

inline const VecN& VecN::operator*=(float rhs)
{
	for (int i = 0; i < N; i++)
	{
		data[i] *= rhs;
	}
	return *this;
}

inline VecN VecN::operator*(float rhs)const
{
	VecN temp = *this;
	temp *= rhs;
	return temp;
}

inline VecN VecN::operator+(const VecN& rhs)const
{
	VecN temp(N);
	for (int i = 0; i < N; i++)
	{
		temp.data[i] = data[i] + rhs.data[i];
	}
	return temp;
}

inline VecN VecN::operator-(const VecN& rhs)const
{
	VecN temp(N);
	for (int i = 0; i < N; i++)
	{
		temp.data[i] = data[i] - rhs.data[i];
	}
	return temp;
}

inline const VecN& VecN::operator+=(const VecN& rhs)
{
	for (int i = 0; i < N; i++)
	{
		data[i] += rhs.data[i];
	}
	return *this;
}

inline const VecN& VecN::operator-=(const VecN& rhs)
{
	for (int i = 0; i < N; i++)
	{
		data[i] -= rhs.data[i];
	}
	return *this;
}

inline float VecN::Dot(const VecN& rhs)const
{
	int sum = 0;
	for (int i = 0; i < N; i++)
	{
		sum += data[i] * rhs.data[i];
	}
	return sum;
}

inline void VecN::Zero()
{
	for (int i = 0; i < N; i++)
	{
		data[i] = 0;
	}
}