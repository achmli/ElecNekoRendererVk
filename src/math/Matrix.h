#pragma once
#include "Vector.h"

/**
* Mat2
*/
class Mat2
{
public:
	Mat2() {}
	Mat2(const Mat2& rhs);
	Mat2(const float* mat);
	Mat2(const Vec2& row0, const Vec2& row1);
	Mat2& operator=(const Mat2& rhs);

	const Mat2& operator*=(const float rhs);
	const Mat2& operator+=(const Mat2& rhs);

	float Determinant()const { return rows[0].x * rows[1].y - rows[0].y * rows[1].x; }
public:
	Vec2 rows[2];
};

inline Mat2::Mat2(const Mat2& rhs)
{
	rows[0] = rhs.rows[0];
	rows[1] = rhs.rows[1];
}

inline Mat2::Mat2(const float* mat)
{
	rows[0] = mat + 0;
	rows[1] = mat + 2;
}

inline Mat2::Mat2(const Vec2& row0, const Vec2& row1)
{
	rows[0] = row0;
	rows[1] = row1;
}

inline Mat2& Mat2::operator=(const Mat2& rhs)
{
	rows[0] = rhs.rows[0];
	rows[1] = rhs.rows[1];
	return *this;
}

inline const Mat2& Mat2::operator*=(const float rhs)
{
	rows[0] *= rhs;
	rows[1] *= rhs;
	return *this;
}

inline const Mat2& Mat2::operator+=(const Mat2& rhs)
{
	rows[0] += rhs.rows[0];
	rows[1] += rhs.rows[1];
	return *this;
}

/**
* Mat3
*/
class Mat3
{
public:
	Mat3() {}
	Mat3(const Mat3& rhs) { rows[0] = rhs.rows[0]; rows[1] = rhs.rows[1]; rows[2] = rhs.rows[2]; }
	Mat3(const float* rhs) { rows[0] = rhs; rows[1] = rhs + 3; rows[2] = rhs + 6; }
	Mat3(const Vec3& row0, const Vec3& row1, const Vec3& row2) { rows[0] = row0; rows[1] = row1; rows[2] = row2; }
	Mat3& operator=(const Mat3& rhs) { rows[0] = rhs.rows[0]; rows[1] = rhs.rows[1]; rows[2] = rhs.rows[2]; return *this; }
	
	void Zero() { rows[0].Zero(); rows[1].Zero(); rows[2].Zero(); }
	void Identity() { rows[0] = Vec3(1, 0, 0); rows[1] = Vec3(0, 1, 0); rows[2] = Vec3(0, 0, 1); }

	float Trace() const 
	{ 
		const float xx = rows[0][0] * rows[0][0]; 
		const float yy = rows[1][1] * rows[1][1];
		const float zz = rows[2][2] * rows[2][2];
		return xx + yy + zz;
	}

	float Determiant()const;
	Mat3 Transpose()const;
	Mat3 Inverse()const;
	Mat2 Minor(const int i, const int j)const;
	float Cofactor(const int i, const int j)const;

	Vec3 operator* (const Vec3& rhs)const;
	Mat3 operator* (const float rhs)const { return Mat3(rows[0] * rhs, rows[1] * rhs, rows[2] * rhs); }
	Mat3 operator* (const Mat3& rhs)const;
	Mat3 operator+ (const Mat3& rhs)const { return Mat3(rows[0] + rhs.rows[0], rows[1] + rhs.rows[1], rows[2] + rhs.rows[2]); }
	const Mat3& operator*=(const float rhs) { rows[0] *= rhs; rows[1] *= rhs; rows[2] *= rhs; return *this; }
	const Mat3& operator+=(const Mat3& rhs) { rows[0] += rhs.rows[0]; rows[1] += rhs.rows[1]; rows[2] += rhs.rows[2]; return *this; }
public:
	Vec3 rows[3];
};

inline float Mat3::Determiant()const
{
	const float i = rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]);
	const float j = rows[0][1] * (rows[1][2] * rows[2][0] - rows[1][0] * rows[2][2]);
	const float k = rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
	return i + j + k;
}

inline Mat3 Mat3::Transpose()const
{
	Mat3 transpose;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			transpose.rows[i][j] = rows[j][i];
		}
	}
	return transpose;
}

inline Mat3 Mat3::Inverse()const
{
	Mat3 inv;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			inv.rows[j][i] = Cofactor(i, j);
		}
	}
	float det = Determiant();
	float invDet = 1.0f / det;
	inv *= invDet;
	return inv;
}

inline Mat2 Mat3::Minor(const int i, const int j)const
{
	Mat2 minor;
	int row = 0;
	for (int x = 0; x < 3; x++)
	{
		if (i == x)
		{
			continue;
		}

		int col = 0;
		for (int y = 0; y < 3; y++)
		{
			if (y == j)
			{
				continue;
			}

			minor.rows[row][col] = rows[x][y];
			col++;
		}
		row++;
	}
	return minor;
}

inline float Mat3::Cofactor(const int i, const int j)const
{
	const Mat2 minor = Minor(i, j);
	const float C = float(pow(-1, i + j + 2)) * minor.Determinant();
	return C;
}

inline Vec3 Mat3::operator*(const Vec3& rhs)const
{
	return Vec3(rows[0].Dot(rhs), rows[1].Dot(rhs), rows[2].Dot(rhs));
}

inline Mat3 Mat3::operator*(const Mat3& rhs)const
{
	float facs[9];
	for (int i = 0; i < 3; i++)
	{
		facs[i * 3 + 0] = rows[i].x * rhs.rows[0].x + rows[i].y * rhs.rows[1].x + rows[i].z * rhs.rows[2].x;
		facs[i * 3 + 1] = rows[i].x * rhs.rows[0].y + rows[i].y * rhs.rows[1].y + rows[i].z * rhs.rows[2].y;
		facs[i * 3 + 2] = rows[i].x * rhs.rows[0].z + rows[i].y * rhs.rows[1].z + rows[i].z * rhs.rows[2].z;
	}
	return Mat3(facs);
}

/**
* Mat4
*/
class Mat4
{
public:
	Mat4() {}
	Mat4(const Mat4& rhs) { rows[0] = rhs.rows[0]; rows[1] = rhs.rows[1]; rows[2] = rhs.rows[2]; rows[3] = rhs.rows[3]; }
	Mat4(const float* mat) { rows[0] = mat; rows[1] = mat + 4; rows[2] = mat + 8; rows[3] = mat + 12; }
	Mat4(const Vec4& row0, const Vec4& row1, const Vec4& row2, const Vec4& row3) { rows[0] = row0; rows[1] = row1; rows[2] = row2; rows[3] = row3; }
	Mat4& operator=(const Mat4& rhs) { rows[0] = rhs.rows[0]; rows[1] = rhs.rows[1]; rows[2] = rhs.rows[2]; rows[3] = rhs.rows[3]; return *this; }

	void Zero() { rows[0].Zero(); rows[1].Zero(); rows[2].Zero(); rows[3].Zero(); }
	void Identity() { rows[0] = Vec4(1, 0, 0, 0); rows[1] = Vec4(0, 1, 0, 0); rows[2] = Vec4(0, 0, 1, 0); rows[3] = Vec4(0, 0, 0, 1); }

	float Trace()const { return rows[0][0] * rows[0][0] + rows[1][1] * rows[1][1] + rows[2][2] * rows[2][2] + rows[3][3] * rows[3][3]; }
	float Determinant()const;
	Mat4 Transpose()const;
	Mat4 Inverse()const;
	Mat3 Minor(const int i, const int j)const;
	float Cofactor(const int i, const int j)const;

	void Orient(Vec3 pos, Vec3 forward, Vec3 up);
	void LookAt(Vec3 pos, Vec3 lookAt, Vec3 up);
	void PerspectiveOpenGL(float fov, float aspectRatio, float zNear, float zFar);
	void PerspectiveVulkan(float fov, float aspectRatio, float zNear, float zFar);
	void OrthoOpenGL(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
	void OrthoVulkan(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);

	const float* ToPtr()const { return rows[0].ToPtr(); }
	float* ToPtr() { return rows[0].ToPtr(); }

	Vec4 operator*(const Vec4& rhs)const { return Vec4(rows[0].Dot(rhs), rows[1].Dot(rhs), rows[2].Dot(rhs), rows[3].Dot(rhs)); }
	Mat4 operator*(const float rhs)const { return Mat4(rows[0] * rhs, rows[1] * rhs, rows[2] * rhs, rows[3] * rhs); }
	Mat4 operator*(const Mat4& rhs)const;
	const Mat4& operator*=(const float rhs) { rows[0] *= rhs; rows[1] *= rhs; rows[1] *= rhs; rows[1] *= rhs; return *this; }
public:
	Vec4 rows[4];
};

inline float Mat4::Determinant()const
{
	float det = 0;
	for (int i = 0; i < 4; i++)
	{
		Mat3 minor = Minor(0, i);

		if (i == 0 || i == 2)
		{
			det += minor.Determiant() * rows[0][i];
		}
		else
		{
			det -= minor.Determiant() * rows[0][i];
		}
	}

	return det;
}

inline Mat4 Mat4::Transpose()const
{
	Mat4 trans;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			trans.rows[i][j] = rows[j][i];
		}
	}
	return trans;
}

inline Mat4 Mat4::Inverse()const
{
	Mat4 inv;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			inv.rows[j][i] = Cofactor(i, j);
		}
	}
	float det = Determinant();
	float invdet = 1.0f / det;
	inv *= invdet;
	return inv;
}

inline Mat3 Mat4::Minor(const int i, const int j)const
{
	Mat3 minor;
	int row = 0;
	for (int x = 0; x < 4; x++)
	{
		if (x == i)continue;

		int col = 0;
		for (int y = 0; y < 4; y++)
		{
			if (y == j)continue;

			minor.rows[row][col] = rows[x][y];
			col++;
		}
		row++;
	}

	return minor;
}

inline float Mat4::Cofactor(const int i, const int j)const
{
	const Mat3 minor = Minor(i, j);
	return float(pow(-1, i + j + 2)) * minor.Determiant();
}

inline void Mat4::Orient(Vec3 pos, Vec3 forward, Vec3 up)
{
	Vec3 left = up.Cross(forward);
	// x-axis: forward
	// y-axis: left
	// z-axis: up
	rows[0] = Vec4(forward.x, left.x, up.x, pos.x);
	rows[1] = Vec4(forward.y, left.y, up.y, pos.y);
	rows[2] = Vec4(forward.z, left.z, up.z, pos.z);
	rows[3] = Vec4(0, 0, 0, 1);
}

inline void Mat4::LookAt(Vec3 pos, Vec3 lookAt, Vec3 up)
{
	Vec3 forward = pos - lookAt;
	forward.Normalize();

	Vec3 right = up.Cross(forward);
	right.Normalize();

	up = forward.Cross(right);
	up.Normalize();

	// For NDC coordinate system where:
	// +x-axis = right
	// +y-axis = up
	// +z-axis = fwd
	rows[0] = Vec4(right.x, right.y, right.z, -pos.Dot(right));
	rows[1] = Vec4(up.x, up.y, up.z, -pos.Dot(up));
	rows[2] = Vec4(forward.x, forward.y, forward.z, -pos.Dot(forward));
	rows[3] = Vec4(0, 0, 0, 1);
}

inline void Mat4::PerspectiveOpenGL(float fov, float aspectRatio, float zNear, float zFar)
{
	const float fovRadians = ElecNekoMath::Math::Radians(fov);
	const float f = 1.0f / tanf(fovRadians * 0.5f);
	const float xScale = f;
	const float yScale = f / aspectRatio;

	rows[0] = Vec4(xScale, 0, 0, 0);
	rows[1] = Vec4(0, yScale, 0, 0);
	rows[2] = Vec4(0, 0, (zFar + zNear) / (zNear - zFar), 2.0f * zNear * zFar / (zNear - zFar));
	rows[3] = Vec4(0, 0, -1, 0);
}

inline void Mat4::PerspectiveVulkan(float fov, float aspectRatio, float zNear, float zFar)
{
	// Vulkan changed its NDC.  It switch from a left handed coordinate system to a right handed one.
	// +x points to the right, +z points into the screen, +y points down (it used to point up, in opengl).
	// It also changed the range from [-1,1] to [0,1] for the z.
	// Clip space remains [-1,1] for x and y.

	Mat4 PersVulkan;
	PersVulkan.rows[0] = Vec4(1, 0, 0, 0);
	PersVulkan.rows[1] = Vec4(0, -1, 0, 0);
	PersVulkan.rows[2] = Vec4(0, 0, 0.5f, 0.5f);
	PersVulkan.rows[3] = Vec4(0, 0, 0, 1);

	Mat4 PersOpenGL;
	PersOpenGL.PerspectiveOpenGL(fov, aspectRatio, zNear, zFar);

	*this = PersVulkan * PersOpenGL;
}

inline void Mat4::OrthoOpenGL(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
	const float width = xMax - xMin;
	const float height = yMax - yMin;
	const float depth = zMax - zMin;

	const float tx = -(xMin + xMax) / width;
	const float ty = -(yMin + yMax) / height;
	const float tz = -(zMin + zMax) / depth;

	rows[0] = Vec4(2.0f / width, 0, 0, tx);
	rows[1] = Vec4(0, 2.0f / height, 0, ty);
	rows[2] = Vec4(0, 0, -2.0f / depth, tz);
	rows[3] = Vec4(0, 0, 0, 1);
}

inline void Mat4::OrthoVulkan(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
	// Vulkan changed its NDC.  It switch from a left handed coordinate system to a right handed one.
	// +x points to the right, +z points into the screen, +y points down (it used to point up, in opengl).
	// It also changed the range from [-1,1] to [0,1] for the z.
	// Clip space remains [-1,1] for x and y.

	Mat4 OrthVulkan;
	OrthVulkan.rows[0] = Vec4(1, 0, 0, 0);
	OrthVulkan.rows[1] = Vec4(0, -1, 0, 0);
	OrthVulkan.rows[2] = Vec4(0, 0, 0.5f, 0.5f);
	OrthVulkan.rows[3] = Vec4(0, 0, 0, 1);

	Mat4 OrthOpenGL;
	OrthOpenGL.OrthoOpenGL(xMin, xMax, yMin, yMax, zMin, zMax);

	*this = OrthVulkan * OrthOpenGL;
}

inline Mat4 Mat4::operator*(const Mat4& rhs)const
{
	Mat4 tmp;
	for (int i = 0; i < 4; i++) 
	{
		tmp.rows[i].x = rows[i].x * rhs.rows[0].x + rows[i].y * rhs.rows[1].x + rows[i].z * rhs.rows[2].x + rows[i].w * rhs.rows[3].x;
		tmp.rows[i].y = rows[i].x * rhs.rows[0].y + rows[i].y * rhs.rows[1].y + rows[i].z * rhs.rows[2].y + rows[i].w * rhs.rows[3].y;
		tmp.rows[i].z = rows[i].x * rhs.rows[0].z + rows[i].y * rhs.rows[1].z + rows[i].z * rhs.rows[2].z + rows[i].w * rhs.rows[3].z;
		tmp.rows[i].w = rows[i].x * rhs.rows[0].w + rows[i].y * rhs.rows[1].w + rows[i].z * rhs.rows[2].w + rows[i].w * rhs.rows[3].w;
	}
	return tmp;
}

/**
* MatMN
*/
class MatMN
{
public:
	MatMN():M(0),N(0){}
	MatMN(int m, int n) :M(m), N(n) { rows = new VecN(M); for (int i = 0; i < M; i++)rows[i] = VecN(N); }
	MatMN(const MatMN& rhs) { *this = rhs; }
	~MatMN() { delete[] rows; }

	const MatMN& operator=(const MatMN& rhs) { M = rhs.M; N = rhs.N; for (int i = 0; i < M; i++)rows[i] = VecN(N); }
	const MatMN& operator*=(float rhs) { for (int i = 0; i < M; i++)rows[i] *= rhs; return *this; }
	VecN operator*(const VecN& rhs)const;
	MatMN operator*(const MatMN& rhs)const;
	MatMN operator*(const float rhs)const { MatMN mat = *this; mat *= rhs; return mat; }

	void Zero() { for (int i = 0; i < M; i++)rows[i].Zero(); }
	MatMN Transpose()const;

public:
	int M, N;
	VecN* rows;
};

inline VecN MatMN::operator*(const VecN& rhs)const
{
	if (rhs.N != N)
	{
		return rhs;
	}

	VecN vec(N);
	for (int i = 0; i < M; i++)
	{
		vec[i] = rhs.Dot(rows[i]);
	}
	return vec;
}

inline MatMN MatMN::operator*(const MatMN& rhs)const
{
	if (rhs.M != N || rhs.N != M)
	{
		return rhs;
	}

	MatMN transRhs = rhs.Transpose();

	MatMN mat(M, rhs.N);
	for (int i = 0; i < M; i++)
	{
		for (int j = 0; j < N; j++)
		{
			mat.rows[i][j] = rows[i].Dot(transRhs.rows[j]);
		}
	}
	return mat;
}

inline MatMN MatMN::Transpose()const
{
	MatMN mat(N, M);
	for (int i = 0; i < M; i++)
	{
		for (int j = 0; j < N; j++)
		{
			mat.rows[j][i] = rows[i][j];
		}
	}
	return mat;
}

/**
* MatN
*/
class MatN
{
public:
	MatN():numDimensions(0){}
	MatN(int n) :numDimensions(n) { rows = new VecN[n]; for (int i = 0; i < n; i++)rows[i] = VecN(n); }
	MatN(const MatN& rhs) { *this = rhs; }
	MatN(const MatMN& rhs) { *this = rhs; }
	~MatN() { delete[] rows; }
	
	const MatN& operator=(const MatN& rhs) { *this = rhs; return *this; }
	const MatN& operator=(const MatMN& rhs) { *this = rhs; return *this; }

	void Identity();
	void Zero() { for (int i = 0; i < numDimensions; i++)rows[i].Zero(); }
	void Transpose();

	void operator*=(float rhs) { for (int i = 0; i < numDimensions; i++)rows[i] *= rhs; }
	VecN operator*(const VecN& rhs);
	MatN operator*(const MatN& rhs);
public:
	int numDimensions;
	VecN* rows;
};

inline void MatN::Identity()
{
	Zero();
	for (int i = 0; i < numDimensions; i++)
	{
		rows[i][i] = 1;
	}
}

inline void MatN::Transpose()
{
	MatN mat = *this;
	for (int i = 0; i < numDimensions; i++)
	{
		for (int j = 0; j < numDimensions; j++)
		{
			rows[i][j] = mat.rows[j][i];
		}
	}
}

inline VecN MatN::operator*(const VecN& rhs)
{
	VecN vec(numDimensions);

	for (int i = 0; i < numDimensions; i++)
	{
		vec[i] = rows[i].Dot(rhs);
	}

	return vec;
}

inline MatN MatN::operator*(const MatN& rhs)
{
	MatN mat(numDimensions);
	mat.Zero();

	for (int i = 0; i < numDimensions; i++)
	{
		for (int j = 0; j < numDimensions; j++)
		{
			mat.rows[i][j] += rows[i][j] * rhs.rows[j][i];
		}
	}

	return mat;
}