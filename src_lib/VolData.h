#pragma once

#include <VolHeader.h>
#include <types.h>

#include <string>
#include <vector>

class VolData
{
public:

  enum class ConstructionMode
  {
    // Allocate data without initializing it
    ALLOCATE,

    // Initialize all voxels to initValue (default: 0.0)
    INITIALIZE,

    // Read data file provided in header (error if absent)
    // For copy constructor, copy values from volume copied
    READ_DATA,

    // Do READ_DATA if data file provided, do INITIALIZE
    // otherwise. For copy constructor, check if volume is
    // allocated.
    READ_DATA_IF_PROVIDED
  };

  // Empty volume
  VolData();

  // From header file
  VolData(
    const std::string& inputVolFile,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::VoxelValue initValue = 0.0);

  // Copy of another volume
  VolData(
    const VolData& vol,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::VoxelValue initValue = 0.0);

  // From header structure
  VolData(const VolHeader& header);

  // Destructor
  ~VolData();

  // Allow only move assignment (TODO: Check if it's done right)
  VolData& operator=(const VolData&) = delete;
  VolData& operator=(VolData&&) = default;

  // Read volume (use if empty constructor was used)
  void read(
    const std::string& headerFileName,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::VoxelValue initValue = 0.0);

  // Copy another volume
  void copy(
    const VolData& vol,
    ConstructionMode mode = ConstructionMode::READ_DATA,
    types::VoxelValue initValue = 0.0);

  // Write volume in interfile format
  void write(const std::string& outputVolFile) const;

  // Allocate volume as multi-volume with parameters taken
  // from an existing template volume
  void allocateAsMultiVol(
    const VolData& templateVol,
    int nFrames);

  // Allocate a single frame based on a template multi-volume
  void allocateSingleFrameFromMultiVol(VolData& templateVol);

  // Print volume info
  void printContent() const;

  // Issue error if number of frames allocated is different
  // from input
  void checkNFrames(int nFrames) const;

  // Assign all values from another volume (all frames)
  void assign(const VolData& vol);

  // Assign values from frame of input volume to active frame
  void assignFrame(const VolData& vol, int frame);

  // Set all voxels to the same value on current frame
  void setAllVoxels(types::VoxelValue value = 0.0);

  // Set all voxels to the same value on all frames
  void setAllVoxelsAllFrames(types::VoxelValue value = 0.0);

  // Voxel-by-voxel arithmetics
  VolData& operator*=(const VolData& inputVol);
  VolData& operator/=(const VolData& inputVol);

  // Active frame
  void setActiveFrame(int frame) const;
  inline int getActiveFrame() const;

  // Line integrals (TODO: Relocate?)
  types::VoxelValue computeLineIntegral(
    types::PathElement* pathElementsArray) const;
  void projectLineIntegral(
    types::PathElement* pathElementsArray,
    types::VoxelValue line);

  // Get volume information
  inline const VolHeader& getHeader() const;
  inline int getNFrames() const;
  inline const types::VolExtent& getVolExtent() const;
  inline int getNVoxelsPerFrame() const;
  inline bool isAllocated() const;
  inline types::VoxelValue* getDataArray() const;

  // Set and get single bin value
  inline void setVoxel(
    int i,
    int j,
    int k,
    types::VoxelValue value);
  inline types::VoxelValue getVoxel(int i, int j, int k) const;

private:

  // Copy parsed and calculated parameters from another volume
  void copyParameters(const VolData& vol);

  void allocate(
    bool initialize = true,
    types::VoxelValue initValue = 0.0);
  void deallocate();

  VolHeader mHeader;
  VolGeometry mGeometry;

  // Index of active frame
  mutable int mActiveFrame;

  // Voxel data

  // Pointer to active frame
  mutable types::VoxelValue* mDataArray;

  // Pointers to each frame
  std::vector<types::VoxelValue*> mFrameVector;

  // Note: Frames are contiguous in memory. Therefore, the
  // pointer mFrameVector[0] points to an array containing all
  // voxels of all frames.
};

#include <VolData.inl>
