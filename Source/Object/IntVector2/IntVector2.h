#pragma once

#include "..\\..\\GameBase.h"

//描画などfloatではくintで使いたい時用
struct IntVector2 
{
	int x;
	int y;

	IntVector2();
	IntVector2(int x, int y);

	int& operator[](int index);
	const int& operator[](int index) const;

	IntVector2& operator += (const D3DXVECTOR2&);
	IntVector2& operator += (const IntVector2&);
	IntVector2	operator + (const D3DXVECTOR2&) const;
	IntVector2	operator + (const IntVector2&) const;

	IntVector2& operator -= (const D3DXVECTOR2&);
	IntVector2& operator -= (const IntVector2&);
	IntVector2 operator-(const D3DXVECTOR2& other) const;
	IntVector2 operator-(const IntVector2& other) const;

	IntVector2 operator/(int scalar)const;

	D3DXVECTOR2 ToD3DVEC2() const;
};