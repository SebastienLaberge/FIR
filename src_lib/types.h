#pragma once

namespace types
{
// === VolData ===

using Size = int;
using Index = Size;

using SpatialExtent = double;
using SpatialCoord = SpatialExtent;

// TODO: Find the right type considering I/O and precision
using VoxelValue = float;
using BinValue = float;

struct VolSize
{
  Size nPixelsX;
  Size nPixelsY;
  Size nSlices;

  inline Size nVoxelsTotal() const;
  inline bool operator==(const VolSize& rhs) const;
};

struct VoxelExtent
{
  SpatialExtent pixelWidth;
  SpatialExtent pixelHeight;
  SpatialExtent sliceThickness;

  inline bool operator==(const VoxelExtent& rhs) const;
};

struct VolExtent
{
  SpatialExtent sliceWidth;
  SpatialExtent sliceHeight;
  SpatialExtent volDepth;

  // TODO: Could this be plugged somewhere?
  // inline bool operator==(const VolExtent& rhs) const;
};

inline VolExtent operator*(
  const VolSize& volSize,
  const VoxelExtent& voxelExtent);

struct SpatialCoords3D
{
  SpatialCoord x;
  SpatialCoord y;
  SpatialCoord z;

  inline bool operator==(const SpatialCoords3D& rhs) const;
};

// === Scanner ===

struct SpatialCoords2D
{
  SpatialCoord x;
  SpatialCoord y;

  // TODO: Could this be useful somewhere?
  // inline bool operator==(const SpatialCoords2D& rhs) const;
};

// === Siddon ===

struct PathElement
{
  Index coord;
  SpatialExtent length;
};
}

#include <types.inl>
