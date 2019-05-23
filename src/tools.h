/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019  Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <QtMath>

namespace ChartTools {

inline qreal scale(qreal value, qreal& pow)
{
    if (qAbs(value) < 1 && value) {
        while (qAbs(value) < 1) {
            pow /= 10;
            value *= 10;
        }
    } else if (qAbs(value) > 10) {
        while (qAbs(value) > 10) {
            pow *= 10;
            value /= 10;
        }
    }
    return value;
}

inline qreal scale(qreal value)
{
    qreal pot;
    return scale(value, pot);
}

inline qreal ceil(qreal value)
{
    if (1 < qAbs(value) && qAbs(value) < 10)
        return std::ceil(value);

    double pot = 1;
    value = scale(value, pot);

    int integer = int(value) + 1;
    if (value < 0)
        integer -= 1;
    return qreal(integer) * pot;
}

inline qreal floor(qreal value)
{
    if (1 < qAbs(value) && qAbs(value) < 10)
        return std::floor(value);

    double pot = 1;
    value = scale(value, pot);

    int integer = int(value);

    if (value < 0)
        integer -= 1;
    return qreal(integer) * pot;
}

}
