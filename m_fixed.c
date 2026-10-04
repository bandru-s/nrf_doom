/*-----------------------------------------------------------------------------
 *
 *
 *  Copyright (C) 2026 Frenkel Smeijers
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Calculate 0xffffffffu / v
 *
 *-----------------------------------------------------------------------------*/

#include "m_fixed.h"


#define USE_LOOKUP_TABLE

#if !defined __GNUC__
static int __builtin_clzl(uint32_t v)
{
	const int bits = sizeof(v) * CHAR_BIT;
	
	for (int i = 0; i < bits; i++)
		if ((v >> (bits - 1 - i)) & 1)
			return i;

	return bits;
}
#endif

fixed_t CONSTFUNC FixedReciprocal(fixed_t v)
{
    if (v == 0)
        return 0;
    return (fixed_t)(0xFFFFFFFFu / (uint32_t)v);
}

fixed_t CONSTFUNC FixedMul(fixed_t a, fixed_t b)
{
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FRACBITS);
}

fixed_t CONSTFUNC FixedMulAngle(fixed_t a, fixed_t b)
{
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FRACBITS);
}

fixed_t CONSTFUNC FixedMul3216(fixed_t a, uint16_t blw)
{
    return (fixed_t)(((int64_t)a * (int64_t)blw) >> 16);
}

fixed_t CONSTFUNC FixedApproxDiv(fixed_t a, fixed_t b)
{
    return FixedMul(a, FixedReciprocal(b));
}