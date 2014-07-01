#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <D3D11.h>
#include <D3DX10math.h>

// generic vector struct
template <typename T, int N>
struct Vector {
	T data[N];
	Vector() { memset(data, 0, N); }
	explicit Vector(const T &constant) { memset(data, T, N); }
	explicit Vector(const Vector &other) { memcpy(data, other.data, N); }
	T& operator[](const int index)
	{
		return data[index];
	}
	const T& operator[](const int index) const
	{
		return data[index];
	}
};

// typedefs for commonly used sizes
typedef Vector<float, 2> fl2;
typedef Vector<float, 3> fl3;
typedef Vector<int, 2> int2;

// template specialization for commonly used ones
template <>
struct Vector<float, 2> {
	union {
		float data[2];
		struct { float x, y; };
		struct { float s, t; };
	};
	Vector() : x(0), y(0) {}
	Vector(const float nx, const float ny) : x(nx), y(ny) {}
	float& operator[](const int index)
	{
		return data[index];
	}
	const float& operator[](const int index) const
	{
		return data[index];
	}
};
template <>
struct Vector<float, 3> {
	union {
		float data[3];
		struct { float x, y, z; };
		struct { float s, t, r; };
		struct { float r, g, b; };
	};
	Vector() : x(0), y(0), z(0) {}
	Vector(const float nx, const float ny, const float nz) : x(nx), y(ny), z(nz) {}
	Vector(const D3DXVECTOR3 &src)
	{
		memcpy(data, &src.x, sizeof(D3DXVECTOR3));
	}
	float& operator[](const int index)
	{
		return data[index];
	}
	const float& operator[](const int index) const
	{
		return data[index];
	}
};

template <>
struct Vector<int, 2> {
	union {
		int data[2];
		struct { int x, y; };
	};
	Vector() : x(0), y(0) {}
	Vector(const int nx, const int ny) : x(nx), y(ny) {}
	int& operator[](const int index)
	{
		return data[index];
	}
	const int& operator[](const int index) const
	{
		return data[index];
	}
};

template <typename T, int N>
bool isZero(const Vector<T, N> &v)
{
	for (int i = 0; i < N; i++) {
		if (v[i] != 0) {
			return false;
		}
	}
	return true;
}

template <typename T, int N>
bool operator==(const Vector<T, N> &first, const Vector<T, N> &second)
{
	for (int i = 0; i < N; i++) {
		if (first[i] != second[i]) {
			return false;
		}
	}
	return true;
}

template <typename T, int N>
Vector<T, N> operator-(const Vector<T, N> &v)
{
	Vector<T, N> nv;
	for (int i = 0; i < N; i++) {
		nv[i] = -v[i];
	}
	return v;
}

template <typename T, int N>
Vector<T, N> operator+(const Vector<T, N> &first, const Vector<T, N> &second)
{
	Vector<T, N> v;
	for (int i = 0; i < N; i++) {
		v[i] = first[i] + second[i];
	}
	return v;
}

template <typename T, int N>
Vector<T, N> operator-(const Vector<T, N> &first, const Vector<T, N> &second)
{
	Vector<T, N> v;
	for (int i = 0; i < N; i++) {
		v[i] = first[i] - second[i];
	}
	return v;
}

template <typename T, int N>
Vector<T, N> operator*(const Vector<T, N> &first, const Vector<T, N> &second)
{
	Vector<T, N> v;
	for (int i = 0; i < N; i++) {
		v[i] = data[i] * other[i];
	}
	return v;
}

template <typename T, int N>
Vector<T, N> operator*(const Vector<T, N> &v, const T &scalar)
{
	Vector<T, N> nv;
	for (int i = 0; i < N; i++) {
		nv[i] = v[i] * scalar;
	}
	return v;
}

template <typename T, int N>
Vector<T, N>& operator+=(Vector<T, N> &first, const Vector<T, N> &second)
{
	for (int i = 0; i < N; i++) {
		first[i] += second[i];
	}
	return first;
}

template <typename T, int N>
Vector<T, N>& operator*=(Vector<T, N> &first, const Vector<T, N> &second)
{
	for (int i = 0; i < N; i++) {
		first[i] *= second[i];
	}
	return first;
}

template <typename T, int N>
Vector<T, N>& operator*=(Vector<T, N> &v, const T &scalar)
{
	for (int i = 0; i < N; i++) {
		v[i] *= scalar;
	}
	return v;
}

template <typename T, int N>
static T dot(const Vector<T, N> &first, const Vector<T, N> &second)
{
	T product;
	for (int i = 0; i < N; i++) {
		product += first[i] * second[i];
	}
	return product;
}

template <typename T, int N>
static bool operator<(const Vector<T, N> &first, const Vector<T, N> &second)
{
	for (int i = 0; i < N; i++) {
		if (first[i] < second[i]) {
			return true;
		}
		if (first[i] > second[i]) {
			return false;
		}
	}
	return false;
}

template <typename T, int N>
T lengthSq(const Vector<T, N> &v)
{
	float temp = 0;
	for (int i = 0; i < N; i++) {
		temp += v[i] * v[i];
	}
	return temp;
}

template <int N>
float length(const Vector<float, N> &v)
{
	return sqrt(lengthSq(v));
}

template <int N>
double length(const Vector<double, N> &v)
{
	return sqrt(lengthSq(v));
}

template <int N>
void normalize(Vector<float, N> &v)
{
	const float magnitude = length(v);
	if (magnitude == 0) {
		return;
	}
	for (int i = 0; i < N; i++) {
		v[i] /= magnitude;
	}
}

template <int N>
Vector<float, N> operator/(const Vector<float, N> &first, const Vector<float, N> &second)
{
	Vector<float, N> v;
	for (int i = 0; i < N; i++) {
		v[i] = first[i] / second[i];
	}
	return v;
}

template <int N>
Vector<float, N> operator/(const Vector<float, N> &v, const float &scalar)
{
	Vector nv;
	for (int i = 0; i < N; i++) {
		nv[i] = v[i] * scalar;
	}
	return nv;
}

template <int N>
Vector<float, N> operator/=(Vector<float, N> &v, const float &scalar)
{
	for (int i = 0; i < N; i++) {
		v[i] /= scalar;
	}
	return v;
}

static Vector<float, 3> cross(const Vector<float, 3> &first, const Vector<float, 3> &second)
{
	return Vector<float, 3>(first.y*second.z - first.z*second.y, first.z*second.x - first.x*second.z, first.x*second.y - first.y*second.x);
}

// some common vertex structs
struct PTNvert {
	fl3 pos_;
	fl3 tex_;
	fl3 norm_;
};

struct PTvert {
	fl3 pos_;
	fl3 tex_;
};

struct PNvert {
	fl3 pos_;
	fl3 norm_;
};

#endif // UTILS_H