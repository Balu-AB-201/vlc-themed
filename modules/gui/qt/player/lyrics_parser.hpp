/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_QT_LYRICS_PARSER_HPP
#define VLC_QT_LYRICS_PARSER_HPP

#include <QList>
#include <QString>
#include <QStringView>

#include <algorithm>
#include <optional>\n#include <utility>

namespace LyricsParser
{
struct Word
{
    QString text;
    qint64 timeMs = 0;
};

struct Line
{
    QString text;
    qint64 timeMs = 0;
    QList<Word> words;
};

struct Result
{
    QList<Line> lines;
    qint64 offsetMs = 0;
    bool hasWordTiming = false;
};

inline std::optional<qint64> timestampMs(QStringView value)
{
    // LRC timestamps are [mm:ss] or [mm:ss.xxx]. Accept one to three
    // fractional digits and normalize them to milliseconds.
    const qsizetype colon = value.indexOf(u':');
    if (colon <= 0 || colon + 1 >= value.size())
        return std::nullopt;

    bool minutesOk = false;
    const qint64 minutes = value.first(colon).toLongLong(&minutesOk);
    if (!minutesOk || minutes < 0)
        return std::nullopt;

    QStringView secondsPart = value.sliced(colon + 1);
    const qsizetype dot = secondsPart.indexOf(u'.');
    QStringView secondsView = dot < 0 ? secondsPart : secondsPart.first(dot);
    bool secondsOk = false;
    const int seconds = secondsView.toInt(&secondsOk);
    if (!secondsOk || seconds < 0 || seconds >= 60)
        return std::nullopt;

    qint64 fraction = 0;
    if (dot >= 0)
    {
        QStringView digits = secondsPart.sliced(dot + 1);
        if (digits.isEmpty() || digits.size() > 3)
            return std::nullopt;
        for (QChar ch : digits)
            if (!ch.isDigit())
                return std::nullopt;

        bool fractionOk = false;
        fraction = digits.toLongLong(&fractionOk);
        if (!fractionOk)
            return std::nullopt;
        if (digits.size() == 1)
            fraction *= 100;
        else if (digits.size() == 2)
            fraction *= 10;
    }

    return minutes * 60000 + seconds * 1000 + fraction;
}

inline Result parseLrc(const QString &source)
{
    Result result;
    const QString normalized = source;
    const QStringList sourceLines = normalized.split(u'\n');

    // First pass: read offset metadata, regardless of its position.
    for (const QString &rawLine : sourceLines)
    {
        const QString line = rawLine.trimmed();
        if (line.startsWith(u"[offset:", Qt::CaseInsensitive)
            && line.endsWith(u']'))
        {
            bool ok = false;
            const qint64 offset = line.mid(8, line.size() - 9).toLongLong(&ok);
            if (ok)
                result.offsetMs = std::clamp<qint64>(offset, -600000, 600000);
        }
    }

    for (const QString &rawLine : sourceLines)
    {
        QStringView line(rawLine.trimmed());
        QList<qint64> lineTimes;

        // LRC permits multiple line timestamps on one line.
        while (line.startsWith(u'['))
        {
            const qsizetype end = line.indexOf(u']');
            if (end <= 1)
                break;

            const QStringView tag = line.sliced(1, end - 1);
            if (tag.startsWith(u"offset:", Qt::CaseInsensitive))
            {
                line = line.sliced(end + 1);
                continue;
            }

            const auto time = timestampMs(tag);
            if (!time)
                break;

            lineTimes.push_back(*time);
            line = line.sliced(end + 1);
        }

        if (lineTimes.isEmpty())
            continue;

        const QString text = line.toString().trimmed();
        if (text.isEmpty())
            continue;

        QList<Word> words;
        QString plainText;
        qsizetype cursor = 0;
        while (cursor < text.size())
        {
            const qsizetype open = text.indexOf(u'<', cursor);
            if (open < 0)
            {
                plainText += text.mid(cursor);
                break;
            }

            plainText += text.mid(cursor, open - cursor);
            const qsizetype close = text.indexOf(u'>', open + 1);
            if (close < 0)
            {
                plainText += text.mid(open);
                break;
            }

            const auto wordTime = timestampMs(QStringView(text).sliced(open + 1, close - open - 1));
            if (!wordTime)
            {
                plainText += text.mid(open, close - open + 1);
                cursor = close + 1;
                continue;
            }

            const qsizetype wordStart = close + 1;
            const qsizetype nextTag = text.indexOf(u'<', wordStart);
            const QString wordText = text.mid(wordStart,
                nextTag < 0 ? -1 : nextTag - wordStart);
            plainText += wordText;
            if (!wordText.isEmpty())
                words.push_back({ wordText, *wordTime });
            cursor = nextTag < 0 ? text.size() : nextTag;
        }

        for (qint64 time : lineTimes)
        {
            Line parsedLine;
            parsedLine.timeMs = std::max<qint64>(0, time + result.offsetMs);
            parsedLine.text = words.isEmpty() ? text : plainText.trimmed();
            parsedLine.words = words;
            if (!words.isEmpty())
            {
                for (Word &word : parsedLine.words)
                    word.timeMs = std::max<qint64>(0, word.timeMs + result.offsetMs);
                result.hasWordTiming = true;
            }
            result.lines.push_back(std::move(parsedLine));
        }
    }

    std::stable_sort(result.lines.begin(), result.lines.end(),
        [](const Line &a, const Line &b) { return a.timeMs < b.timeMs; });
    return result;
}
} // namespace LyricsParser

#endif // VLC_QT_LYRICS_PARSER_HPP
