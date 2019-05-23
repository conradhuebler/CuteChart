/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 */

#pragma once

#include <QtCore/QList>

#include <cmath>

struct BoxWhisker {
    QList<qreal> mild_outliers, extreme_outliers;
    qreal lower_whisker = 0;
    qreal upper_whisker = 0;
    qreal lower_quantile = 0;
    qreal upper_quantile = 0;
    qreal median = 0;
    qreal mean = 0;
    qreal stddev = 0;
    int count = 0;

    inline qreal UpperNotch() const { return median + (1.58 * (upper_quantile - lower_quantile) / sqrt(count)); }
    inline qreal LowerNotch() const { return median - (1.58 * (upper_quantile - lower_quantile) / sqrt(count)); }
};
