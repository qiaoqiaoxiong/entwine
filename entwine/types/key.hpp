/******************************************************************************
* Copyright (c) 2018, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#pragma once

#include <cassert>
#include <cctype>
#include <cstdint>
#include <string>

#include <entwine/types/bounds.hpp>
#include <entwine/types/dir.hpp>
#include <entwine/types/metadata.hpp>
#include <entwine/types/point.hpp>
#include <entwine/types/structure.hpp>

namespace entwine
{

struct Xyz
{
    Xyz() { }
    Xyz(uint64_t x, uint64_t y, uint64_t z) : x(x), y(y), z(z) { }

    void reset() { x = 0; y = 0; z = 0; }

    std::string toString() const
    {
        return
            std::to_string(x) + '-' +
            std::to_string(y) + '-' +
            std::to_string(z);
    }

    std::string toString(std::size_t d) const
    {
        return (d < 10 ? "0" : "") + std::to_string(d) + '-' + toString();
    }

    uint64_t x = 0;
    uint64_t y = 0;
    uint64_t z = 0;
};

struct Dxyz
{
    Dxyz() : x(p.x), y(p.y), z(p.z) { }

    Dxyz(uint64_t d, uint64_t x, uint64_t y, uint64_t z)
        : p(x, y, z)
        , d(d)
        , x(p.x)
        , y(p.y)
        , z(p.z)
    { }

    Dxyz(uint64_t d, const Xyz& p)
        : Dxyz(d, p.x, p.y, p.z)
    { }

    Dxyz(std::string v)
        : Dxyz()
    {
        const auto s(pdal::Utils::split(v, [](char c)
        {
            return !std::isdigit(c);
        }));

        if (s.size() != 4)
        {
            throw std::runtime_error("Couldn't parse " + v + " as DXYZ");
        }

        d = std::stoull(s[0]);

        p = Xyz(
                std::stoull(s[1]),
                std::stoull(s[2]),
                std::stoull(s[3]));

        assert(toString() == v);
    }

    std::string toString() const { return p.toString(d); }

    Xyz p;

    uint64_t d = 0;
    uint64_t& x;
    uint64_t& y;
    uint64_t& z;
};

inline bool operator<(const Xyz& a, const Xyz& b)
{
    return
        (a.x < b.x) ||
        (a.x == b.x &&
            ((a.y < b.y) || (a.y == b.y && a.z < b.z)));
}

inline bool operator<(const Dxyz& a, const Dxyz& b)
{
    return a.d < b.d || (a.d == b.d && a.p < b.p);
}

struct Key
{
    Key(const Metadata& metadata)
        : m(metadata)
    {
        reset();
    }

    void reset()
    {
        b = m.boundsScaledCubic();
        p.reset();
    }

    void step(const Point& g)
    {
        step(getDirection(b.mid(), g));
    }

    void step(Dir dir)
    {
        p.x = (p.x << 1) | (isEast(dir)  ? 1u : 0u);
        p.y = (p.y << 1) | (isNorth(dir) ? 1u : 0u);
        p.z = (p.z << 1) | (isUp(dir)    ? 1u : 0u);

        b.go(dir);
    }

    const Bounds& bounds() const { return b; }
    const Xyz& position() const { return p; }

    const Metadata& m;

    Bounds b;
    Xyz p;
};

struct ChunkKey
{
    ChunkKey(const Metadata& m) : k(m) { }

    void reset()
    {
        d = 0;
        k.reset();
    }

    void step(const Point& g)
    {
        if (inBody()) k.step(g);
        ++d;
    }

    void step(Dir dir)
    {
        if (inBody()) k.step(dir);
        ++d;
    }

    void step()
    {
        assert(inTail());
        ++d;
    }

    ChunkKey getStep(Dir dir) const
    {
        ChunkKey c(*this);
        c.step(dir);
        return c;
    }

    ChunkKey getStep() const
    {
        ChunkKey c(*this);
        c.step();
        return c;
    }

    bool inBody() const
    {
        const auto& s(k.m.structure());
        return d >= s.body() && d < s.tail();
    }

    bool inTail() const
    {
        return d >= k.m.structure().tail();
    }

    Dxyz get() const { return Dxyz(d, k.p); }

    const Bounds& bounds() const { return k.bounds(); }
    uint64_t depth() const { return d; }

    Key k;
    uint64_t d = 0;
};

} // namespace entwine
