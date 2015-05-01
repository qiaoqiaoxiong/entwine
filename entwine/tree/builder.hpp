/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <pdal/Dimension.hpp>

#include <entwine/drivers/source.hpp>
#include <entwine/tree/point-info.hpp>
#include <entwine/types/dim-info.hpp>

namespace pdal
{
    class PointView;
    class StageFactory;
}

namespace Json
{
    class Value;
}

namespace entwine
{

class Arbiter;
class BBox;
class Clipper;
class Driver;
class Pool;
class Registry;
class Reprojection;

class Builder
{
public:
    Builder(
            std::string buildPath,
            std::string tmpPath,
            const Reprojection& reprojection,
            const BBox& bbox,
            const DimList& dimList,
            std::size_t numThreads,
            std::size_t numDimensions,
            std::size_t baseDepth,
            std::size_t flatDepth,
            std::size_t diskDepth,
            std::shared_ptr<Arbiter> arbiter = 0);

    Builder(
            std::string buildPath,
            std::string tmpPath,
            const Reprojection& reprojection,
            std::size_t numThreads,
            std::shared_ptr<Arbiter> arbiter = 0);

    ~Builder();

    // Insert the points from a PointView into this index asynchronously.  To
    // await the results of all outstanding inserts, call join().
    void insert(std::string filename);
    void join();

    // Remove resources that are no longer needed.
    void clip(Clipper* clipper, std::size_t index);

    // Save the current state of the tree.
    void save();

    // Awaken the tree from a saved state.  After a load(), no queries should
    // be made until save() is subsequently called.
    void load();

    // Write the tree to an export format independent from the specifics of how
    // it was built.
    void finalize(std::string path, std::size_t base, bool compress);

    // Get bounds of the quad tree.
    const BBox& getBounds() const;

    // Return all points at depth levels between [depthBegin, depthEnd).
    // A depthEnd value of zero will return all points at levels >= depthBegin.
    std::vector<std::size_t> query(
            Clipper* clipper,
            std::size_t depthBegin,
            std::size_t depthEnd);

    // Return all points within the bounding box, searching at tree depth
    // levels from [depthBegin, depthEnd).
    // A depthEnd value of zero will return all points within the query range
    // that have a tree level >= depthBegin.
    std::vector<std::size_t> query(
            Clipper* clipper,
            const BBox& bbox,
            std::size_t depthBegin,
            std::size_t depthEnd);

    // Get the constituent bytes of a point by its index, with bytes arranged
    // in accordance with the requested schema.  If no point exists at the
    // specified index, returns an empty vector.
    std::vector<char> getPointData(
            Clipper* clipper,
            std::size_t index,
            const Schema& schema);

    const Schema& schema() const;

    std::size_t numPoints() const;
    std::string name() const;

private:
    void prep();

    void insert(
            pdal::PointView& pointView,
            Origin origin,
            Clipper* clipper);

    Json::Value getTreeMeta() const;

    Origin addOrigin(const std::string& remote);
    std::string inferPdalDriver(const std::string& path) const;

    //

    std::unique_ptr<Reprojection> m_reprojection;

    std::unique_ptr<BBox> m_bbox;
    std::unique_ptr<Schema> m_schema;
    pdal::Dimension::Id::Enum m_originId;
    std::size_t m_dimensions;
    std::size_t m_numPoints;
    std::size_t m_numTossed;

    std::vector<std::string> m_originList;

    std::unique_ptr<Pool> m_pool;

    std::shared_ptr<Arbiter> m_arbiter;
    Source m_buildSource;
    Source m_tmpSource;

    std::unique_ptr<pdal::StageFactory> m_stageFactory;
    std::unique_ptr<Registry> m_registry;

    Builder(const Builder&);
    Builder& operator=(const Builder&);
};

} // namespace entwine

