/*
 * Chart and axis formatting utilities.
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
#include <cmath>

namespace ChartTools {

/**
 * @brief Scales a value to be between 1-10 and returns the scaling factor
 * @param value Value to be scaled
 * @param pow Scaling factor (output parameter)
 * @return Scaled value
 */
inline qreal ScaleToNormalizedRange(qreal value, qreal& pow)
{
    pow = 1.0;
    if (value == 0.0) {
        return 0.0;
    }

    if (qAbs(value) < 1.0) {
        while (qAbs(value) < 1.0) {
            pow /= 10.0;
            value *= 10.0;
        }
    } else if (qAbs(value) >= 10.0) {
        while (qAbs(value) >= 10.0) {
            pow *= 10.0;
            value /= 10.0;
        }
    }
    return value;
}

/**
 * @brief Scales a value to be between 1-10 (without returning scaling factor)
 * @param value Value to be scaled
 * @return Scaled value
 */
inline qreal ScaleToNormalizedRange(qreal value)
{
    qreal pow = 1.0;
    return ScaleToNormalizedRange(value, pow);
}

/**
 * @brief Rounds a value down to a nice value for axis labeling
 * @param value Value to floor
 * @return Nicely floored value
 */
inline qreal NiceFloor(qreal value)
{
    if (value == 0.0) {
        return 0.0;
    }

    int sign = (value < 0) ? -1 : 1;
    value = qAbs(value);
    qreal magnitude = std::floor(log10(value));
    qreal scale = qPow(10, -magnitude);

    return sign * std::floor(value * scale) / scale;
}

/**
 * @brief Rounds a value up to a nice value for axis labeling
 * @param value Value to ceil
 * @return Nicely ceiled value
 */
inline qreal NiceCeil(qreal value)
{
    if (value == 0.0) {
        return 0.0;
    }

    int sign = (value < 0) ? -1 : 1;
    value = qAbs(value);
    qreal magnitude = std::floor(log10(value));
    qreal scale = qPow(10, -magnitude);

    return sign * std::ceil(value * scale) / scale;
}

/**
 * @brief Creates a nice minimum value for chart scaling
 * @param value Original minimum value
 * @return Nicely adjusted minimum value
 */
inline qreal NiceScalingMin(qreal value)
{
    if (value == 0.0) {
        return 0.0;
    }

    int sign = (value < 0) ? -1 : 1;
    value = qAbs(value);

    qreal magnitude = std::floor(log10(value));
    qreal normalized = value / qPow(10, magnitude);

    qreal niceValue;
    if (normalized >= 5.0) {
        niceValue = 5.0;
    } else if (normalized >= 2.0) {
        niceValue = 2.0;
    } else if (normalized >= 1.0) {
        niceValue = 1.0;
    } else {
        niceValue = 0.0;
    }

    return sign * niceValue * qPow(10, magnitude);
}

/**
 * @brief Creates a nice maximum value for chart scaling
 * @param value Original maximum value
 * @return Nicely adjusted maximum value
 */
inline qreal NiceScalingMax(qreal value)
{
    if (value == 0.0) {
        return 0.0;
    }

    int sign = (value < 0) ? -1 : 1;
    value = qAbs(value);

    qreal magnitude = std::floor(log10(value));
    qreal normalized = value / qPow(10, magnitude);

    qreal niceValue;
    if (normalized > 0.9 && normalized <= 2.0) {
        niceValue = 2.0;
    } else if (normalized <= 5.0) {
        niceValue = 5.0;
    } else {
        niceValue = 10.0;
    }

    return sign * niceValue * qPow(10, magnitude);
}

/**
 * @brief Calculates an ideal interval for axis ticks
 * @param min Minimum value (input/output parameter)
 * @param max Maximum value (input/output parameter)
 * @param start Starting point for ticks (output parameter)
 * @param step Step size for ticks (output parameter)
 */
inline void IdealInterval(qreal& min, qreal& max, qreal& start, qreal& step)
{
    // Adjust min/max if they're close to zero
    if (min * max > 0) {
        if (min / max < 0.125) {
            min = 0;
        }
    } else if (min * max < 0) {
        if (min / max > -0.125) {
            max = 0;
        }
    }

    qreal difference = max - min;
    if (difference == 0.0) {
        difference = 1.0; // Avoid division by zero
    }

    qreal magnitude = std::floor(log10(difference));
    qreal scale = qPow(10, -magnitude);
    qreal scaledDifference = std::ceil(difference * scale);

    // Determine appropriate step size
    if (scaledDifference < 1) {
        step = std::ceil(1 / scale);
    } else if (scaledDifference < 6) {
        step = std::ceil(2 / scale);
    } else {
        step = std::ceil(5 / scale);
    }

    start = min;

    // Adjust max to align with step
    qreal adjustedMax = start;
    while (adjustedMax < max) {
        adjustedMax += step;
    }
    max = adjustedMax;
}

/**
 * @brief Custom ceiling function that returns nice rounded values
 * @param value Value to ceil
 * @return Nicely ceiled value
 */
inline qreal CustomCeil(qreal value)
{
    if (value == 0.0) {
        return 0.0;
    }

    if (1.0 < qAbs(value) && qAbs(value) < 10.0) {
        return std::ceil(value);
    }

    qreal scaleFactor = 1.0;
    qreal normalized = ScaleToNormalizedRange(value, scaleFactor);

    int integer = int(normalized) + (normalized < 0 ? 0 : 1);
    return qreal(integer) * scaleFactor;
}

/**
 * @brief Custom floor function that returns nice rounded values
 * @param value Value to floor
 * @return Nicely floored value
 */
inline qreal CustomFloor(qreal value)
{
    if (value == 0.0) {
        return 0.0;
    }

    if (1.0 < qAbs(value) && qAbs(value) < 10.0) {
        return std::floor(value);
    }

    qreal scaleFactor = 1.0;
    qreal normalized = ScaleToNormalizedRange(value, scaleFactor);

    int integer = int(normalized) - (normalized < 0 ? 1 : 0);
    return qreal(integer) * scaleFactor;
}

/**
 * @brief Merges two JSON objects, preserving structure
 * @param target Target JSON object
 * @param inserted Object to insert
 * @return Merged JSON object
 */
inline QJsonObject MergeJsonObject(const QJsonObject& target, const QJsonObject& inserted)
{
    QJsonObject result = target;
    for (const QString& key : inserted.keys()) {
        if (!target.contains(key)) {
            continue;
        }

        if (inserted[key].isObject() && !inserted[key].toObject().isEmpty()) {
            result[key] = MergeJsonObject(result[key].toObject(), inserted[key].toObject());
        } else {
            result[key] = inserted[key];
        }
    }
    return result;
}

} // namespace ChartTools
