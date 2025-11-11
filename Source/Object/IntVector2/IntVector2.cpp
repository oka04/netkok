#define _USING_V110_SDK71_ 1

#include "IntVector2.h"

using namespace KeyString;
using namespace InputKey;
using namespace WindowSetting;

IntVector2::IntVector2()
	: x(0), y(0)
{
}

IntVector2::IntVector2(int x, int y)
	: x(x)
	, y(y)
{
}

int& IntVector2::operator[](int index) 
{
	if (index == 0) return x;
	return y;
}

const int& IntVector2::operator[](int index) const 
{
	if (index == 0) return x;
	return y;
}

IntVector2& IntVector2::operator += (const D3DXVECTOR2& vec) 
{
	x += (int)vec.x;
	y += (int)vec.y;
	return *this;
}

IntVector2& IntVector2::operator += (const IntVector2& vec)
{
	x += vec.x;
	y += vec.y;
	return *this;
}

IntVector2 IntVector2::operator + (const D3DXVECTOR2& vec) const
{
	return IntVector2(x + (int)vec.x, y + (int)vec.y);
}

IntVector2 IntVector2::operator + (const IntVector2& vec) const
{
	return IntVector2(x + vec.x, y + vec.y);
}

IntVector2& IntVector2::operator -= (const D3DXVECTOR2& vec)
{
	x -= (int)vec.x;
	y -= (int)vec.y;
	return *this;
}

IntVector2& IntVector2::operator -= (const IntVector2& vec)
{
	x -= vec.x;
	y -= vec.y;
	return *this;
}

IntVector2 IntVector2::operator - (const D3DXVECTOR2& vec) const 
{
	return IntVector2(x - (int)vec.x, y - (int)vec.y);
}

IntVector2 IntVector2::operator - (const IntVector2& vec) const {
	return IntVector2(x - vec.x, y - vec.y);
}

IntVector2 IntVector2::operator/(int scalar)const
{
	return IntVector2(x / scalar, y / scalar);
}

D3DXVECTOR2 IntVector2::ToD3DVEC2() const
{
	return D3DXVECTOR2((float)x, (float)y);
}