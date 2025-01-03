#pragma once

#include <types.h>

// Fundamental volume parameters
struct VolHeader
{
  // Number of voxels along each dimension
  types::VolSize volSize;

  // Spatial extent of each voxel along each dimension
  types::VoxelExtent voxelExtent;

  // Volume offset
  // In X and Y: Coordinates of center of first voxel
  // In Z: Offset from scanner center to volume center
  types::SpatialCoords3D volOffset;

  // Number of frames
  int nFrames; // Defaults to 1

  // Operators
  inline bool operator==(const VolHeader& rhs) const;
  inline bool operator!=(const VolHeader& rhs) const;
  inline VolHeader& operator=(const VolHeader& rhs);

  // Fill structure with default values
  // Note: Must executed before filling with specific values
  inline void setDefaults();

  // Check validity of values and throw if there is a problem
  // Note: Must executed after filling with specific values
  void check() const;
};

// Derived volume parameters
struct VolGeometry
{
  // Spatial extent of the entire volume along each dimension
  types::VolExtent volExtent;

  // Number of voxels in each frame and for all frames
  int nVoxelsPerFrame;
  int nVoxelsTotal;

  // Operators
  inline VolGeometry& operator=(const VolGeometry& rhs);

  // Fill structure using information contained in header
  // Note: Must be executed before using the structure
  void fill(const VolHeader& header);
};

#include <VolHeader.inl>
