/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2020  Mike Tzou <Chocobo1>
 * Copyright (C) 2015  Vladimir Golovnev <glassez@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "infohash.h"

#include <QByteArray>
#include <QHash>

using namespace BitTorrent;

const int InfoHashTypeId = qRegisterMetaType<InfoHash>();

#if (LIBTORRENT_VERSION_NUM >= 20000) && (LIBTORRENT_VERSION_NUM < 20002)
namespace std
{
    template <std::ptrdiff_t N>
    struct hash<libtorrent::digest32<N>>
    {
        std::size_t operator()(const libtorrent::digest32<N>& k) const
        {
            std::size_t ret;
            static_assert(N >= sizeof(ret) * 8, "hash is not defined for small digests");
            // this is OK because digest32<N> is already a hash
            std::memcpy(&ret, k.data(), sizeof(ret));
            return ret;
        }
    };
}
#endif

InfoHashFormat InfoHash::whichFormat() const
{
    if (m_nativeHash.has_v1() && m_nativeHash.has_v2())
        return InfoHashFormat::Both;
    if (m_nativeHash.has_v2())
        return InfoHashFormat::V2;
    return InfoHashFormat::V1;
}

InfoHash::InfoHash(const lt::sha1_hash &hash)
    : m_valid {true}
    , m_nativeHash {hash}
{
    const QByteArray raw = QByteArray::fromRawData(hash.data(), v1_length());
    m_hashString[0] = QString::fromLatin1(raw.toHex());
}

InfoHash::InfoHash(const lt::sha256_hash &hash)
    : m_valid {true}
    , m_nativeHash {hash}
{
    const QByteArray raw = QByteArray::fromRawData(hash.data(), v2_length());
    m_hashString[1] = QString::fromLatin1(raw.toHex());
}

InfoHash::InfoHash(const lt::info_hash_t &hash)
    : m_valid {true}
    , m_nativeHash {hash}
{
    if (hash.has_v1())
    {
        const QByteArray raw = QByteArray::fromRawData(hash.v1.data(), v1_length());
        m_hashString[0] = QString::fromLatin1(raw.toHex());
    }
    if (hash.has_v2())
    {
        const QByteArray raw = QByteArray::fromRawData(hash.v2.data(), v2_length());
        m_hashString[1] = QString::fromLatin1(raw.toHex());
    }
}

InfoHash::InfoHash(const QString &hashString)
{
    QByteArray raw;

    if (hashString.size() == (v1_length() * 2))
    {
        raw = QByteArray::fromHex(hashString.toLatin1());
        if (raw.size() != v1_length())  // QByteArray::fromHex() will skip over invalid characters
            return;

        m_hashString[0] = hashString;
        m_nativeHash = lt::info_hash_t {lt::sha1_hash {raw.constData()}};
    }
    else if (hashString.size() == (v2_length() * 2))
    {
        raw = QByteArray::fromHex(hashString.toLatin1());
        if (raw.size() != v2_length())  // QByteArray::fromHex() will skip over invalid characters
            return;

        m_hashString[1] = hashString;
        m_nativeHash = lt::info_hash_t {lt::sha256_hash {raw.constData()}};
    }

    m_valid = true;
}

bool InfoHash::isValid() const
{
    return m_valid;
}

InfoHash::operator lt::sha1_hash() const
{
    return m_nativeHash.v1;
}

InfoHash::operator lt::sha256_hash() const
{
    return m_nativeHash.v2;
}

QString InfoHash::v1_string() const
{
    return m_hashString[0];
}

QString InfoHash::v2_string() const
{
    return m_hashString[1];
}

namespace BitTorrent
{
    bool operator==(const InfoHash &left, const InfoHash &right)
    {
        return left.m_nativeHash == right.m_nativeHash;
    }

    bool operator<(const InfoHash &left, const InfoHash &right)
    {
        return left.m_nativeHash < right.m_nativeHash;
    }
}

bool BitTorrent::operator!=(const InfoHash &left, const InfoHash &right)
{
    return !(left == right);
}

uint BitTorrent::qHash(const InfoHash &key, const uint seed)
{
    switch (key.whichFormat())
    {
    case InfoHashFormat::V1:
        return ::qHash((std::hash<lt::sha1_hash> {})(static_cast<lt::sha1_hash>(key)), seed);
    case InfoHashFormat::V2:
        return ::qHash((std::hash<lt::sha256_hash> {})(static_cast<lt::sha256_hash>(key)), seed);
    case InfoHashFormat::Both:
    default:
        return ::qHash((std::hash<lt::sha256_hash> {})(static_cast<lt::sha256_hash>(key)), seed)
            ^ ::qHash((std::hash<lt::sha1_hash> {})(static_cast<lt::sha1_hash>(key)));
    };
}
