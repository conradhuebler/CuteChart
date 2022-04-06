/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtCore/QDebug>
#include <QtCore/QJsonObject>
#include <QtMath>

namespace ChartTools {

inline qreal NiceFloor(qreal value)
{
    int sign = 1;
    if (value < 0)
        sign = -1;
    value = qAbs(value);
    qreal dim = qPow(10, -1 * std::floor(log10(value)));
    qreal tmp = value * dim;
    if (sign > 0)
        value = std::floor(tmp) / dim;
    else
        value = std::ceil(tmp) / dim;
    return sign * value;
}

inline qreal NiceCeil(qreal value)
{
    int sign = 1;
    if (value < 0)
        sign = -1;
    value = qAbs(value);
    qreal dim = qPow(10, -1 * std::floor(log10(value)));
    qreal tmp = value * dim;
    if (sign > 0)
        value = std::ceil(tmp) / dim;
    else
        value = std::floor(tmp) / dim;
    return sign * value;
}

inline qreal NiceScalingMin(qreal value)
{
    int sign = 1;
    if (value < 0)
        sign = -1;
    value = qAbs(value);

    qreal result = value;

    qreal transform = -1 * std::ceil(log10(value));
    qreal dimension = qPow(10, -1 * std::floor(log10(value)));

    if (dimension >= 5)
        result = 5;
    else if (dimension >= 2)
        result = 2;
    else if (dimension >= 0.99)
        result = 1;
    else
        result = 0;

    qDebug() << sign * value << value << transform << result << sign * result * qPow(10, -1 * floor(transform));
    return sign * result * qPow(10, transform);
}

inline qreal NiceScalingMax(qreal value)
{
    int sign = 1;
    if (value < 0)
        sign = -1;
    value = qAbs(value);

    qreal result = value;

    qreal transform = -1 * std::ceil(log10(value));
    qreal dimension = qPow(10, -1 * std::floor(log10(value)));

    if (dimension > 0.9 && dimension <= 2)
        result = 2;
    else if (dimension <= 5)
        result = 5;
    else
        result = 10;

    qDebug() << sign * value << value << transform << result << sign * result * qPow(10, -1 * floor(transform));
    return sign * result * qPow(10, transform);
}

inline void IdealInterval(qreal& min, qreal& max, qreal& start, qreal& step)
{
    if (min * max > 0) {
        if (min / max < 0.125)
            min = 0;
    } else if (min * max < 0) {
        if (min / max > -0.125)
            max = 0;
    }

    qreal difference = max - min;
    qreal dim = qPow(10, -1 * std::floor(log10(difference)));
    qreal tmp_diff = std::ceil(difference * dim);

    if (tmp_diff < 1)
        step = std::ceil(1 / dim);
    else if (tmp_diff < 6)
        step = std::ceil(2 / dim);
    else
        step = std::ceil(5 / dim);
    start = min;

    qreal tmp = start;
    while (tmp < max)
        tmp += step;
    max = tmp;

    qDebug() << min << max << difference << dim;
}

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

inline QJsonObject MergeJsonObject(const QJsonObject& target, const QJsonObject& inserted)
{
    QJsonObject result = target;
    for (const QString& key : inserted.keys()) {
        if (inserted[key].toObject().keys().size())
            result[key] = MergeJsonObject(result[key].toObject(), inserted[key].toObject());
        else
            result[key] = inserted[key];
    }
    return result;
}
}
