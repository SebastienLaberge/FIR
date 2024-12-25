#include <VolData.h>

#include <VolInterfileReader.h>
#include <console.h>
#include <macros.h>

#include <cstdlib>

VolData::VolData():
  mActiveFrame{0},
  mDataArray{nullptr}
{
}

VolData::VolData(
  const std::string& inputVolFile,
  ConstructionMode mode,
  types::VoxelValue initValue):
  VolData{}
{
  read(inputVolFile, mode, initValue);
}

VolData::VolData(
  const VolData& vol,
  ConstructionMode mode,
  types::VoxelValue initValue):
  VolData{}
{
  copy(vol, mode, initValue);
}

VolData::~VolData()
{
  deallocate();
}

void VolData::read(
  const std::string& headerFileName,
  ConstructionMode mode,
  types::VoxelValue initValue)
{
  // Deallocate in case it has already been allocated
  deallocate();

  VolInterfileReader reader(headerFileName);
  const auto hasDataFile = reader.hasDataFile();
  mHeader = reader.getHeader();
  mGeometry = reader.getGeometry();

  allocate();

  switch (mode)
  {
  case ConstructionMode::ALLOCATE:

    break;

  case ConstructionMode::INITIALIZE:

    setAllVoxelsAllFrames(initValue);
    break;

  case ConstructionMode::READ_DATA:

    reader.readData(mFrameVector);
    break;

  case ConstructionMode::READ_DATA_IF_PROVIDED:

    if (hasDataFile)
    {
      reader.readData(mFrameVector);
    }
    else
    {
      setAllVoxelsAllFrames(initValue);
    }
    break;
  }
}

void VolData::copy(
  const VolData& vol,
  ConstructionMode mode,
  types::VoxelValue initValue)
{
  // Deallocate in case it has already been allocated
  deallocate();

  copyParameters(vol);

  allocate();

  switch (mode)
  {
  case ConstructionMode::ALLOCATE:

    break;

  case ConstructionMode::INITIALIZE:

    setAllVoxelsAllFrames(initValue);
    break;

  case ConstructionMode::READ_DATA:

    if (!vol.isAllocated())
    {
      error("Mode is READ_DATA and volume to copy isn't "
            "allocated");
    }
    assign(vol);
    break;

  case ConstructionMode::READ_DATA_IF_PROVIDED:

    if (vol.isAllocated())
    {
      assign(vol);
      break;
    }
    else
    {
      setAllVoxelsAllFrames(initValue);
      break;
    }
  }
}

void VolData::write(const std::string& outputVolFile) const
{
  if (!isAllocated())
  {
    error("Volume data is not allocated");
  }

  VolInterfileReader::writeVolInterfile(
    outputVolFile,
    mHeader,
    mFrameVector);
}

void VolData::allocateAsMultiVol(
  const VolData& templateVol,
  int nFrames)
{
  if (nFrames <= 0)
  {
    error("Number of frames must be greater than zero");
  }

  deallocate();

  copyParameters(templateVol);
  mHeader.nFrames = nFrames;
  mGeometry.nVoxelsTotal =
    mGeometry.nVoxelsPerFrame * mHeader.nFrames;

  allocate();
}

void VolData::allocateSingleFrameFromMultiVol(
  VolData& templateVol)
{
  deallocate();

  copyParameters(templateVol);
  mHeader.nFrames = 1;
  mGeometry.nVoxelsTotal = mGeometry.nVoxelsPerFrame;

  allocate();
}

void VolData::printContent() const
{
  echo("= Volume header:");
  printEmptyLine();

  echo("== Volume dimensions (width in x, height in y, "
       "depth in z):");
  printVector<types::Size>(
    "volume size in voxels",
    {mHeader.volSize.nPixelsX,
     mHeader.volSize.nPixelsY,
     mHeader.volSize.nSlices});
  printVector<types::SpatialExtent>(
    "voxel extent in mm",
    {mHeader.voxelExtent.pixelWidth,
     mHeader.voxelExtent.pixelHeight,
     mHeader.voxelExtent.sliceThickness});
  printVector<types::SpatialExtent>(
    "volume extent in mm",
    {mGeometry.volExtent.sliceWidth,
     mGeometry.volExtent.sliceHeight,
     mGeometry.volExtent.volDepth});
  printEmptyLine();

  echo("== Volume position:");
  printVector<types::SpatialCoord>(
    "x,y coordinates of first voxel in mm",
    {mHeader.volOffset.x, mHeader.volOffset.y});
  printValue("volume offset in z in mm", mHeader.volOffset.z);

  printEmptyLine();

  echo("== Number of frames and voxels:");
  printValue("number of time frames", mHeader.nFrames);
  printValue(
    "number of voxels per frame",
    mGeometry.nVoxelsPerFrame);
  printValue("total number of voxels", mGeometry.nVoxelsTotal);
  printEmptyLine();
}

void VolData::checkNFrames(int nFrames) const
{
  if (mHeader.nFrames != nFrames)
  {
    error("Wrong number of frames (expected ", nFrames, ")");
  }
}

void VolData::assign(const VolData& vol)
{
  if (!isAllocated())
  {
    error("Volume not allocated");
  }

  if (
    mHeader != vol.mHeader ||
    mHeader.nFrames != vol.mHeader.nFrames)
  {
    error("Volume assigned is not the right size");
  }

#pragma omp parallel for
  LOOP(i, 0, mGeometry.nVoxelsTotal - 1)
  {
    mFrameVector[0][i] = vol.mFrameVector[0][i];
  }
}

void VolData::assignFrame(const VolData& vol, int frame)
{
  if (!isAllocated())
  {
    error("Volume not allocated");
  }

  if (mHeader != vol.mHeader)
  {
    error("Volume assigned is not the right size");
  }

#pragma omp parallel for
  LOOP(i, 0, mGeometry.nVoxelsPerFrame - 1)
  {
    mDataArray[i] = vol.mFrameVector[frame][i];
  }
}

void VolData::setAllVoxels(types::VoxelValue value)
{
  if (!isAllocated())
  {
    error("Volume not allocated");
  }

#pragma omp parallel for
  LOOP(i, 0, mGeometry.nVoxelsPerFrame - 1)
  {
    mDataArray[i] = value;
  }
}

void VolData::setAllVoxelsAllFrames(types::VoxelValue value)
{
  if (!isAllocated())
  {
    error("Volume not allocated");
  }

  auto* completeDataArray = mFrameVector[0];

#pragma omp parallel for
  LOOP(i, 0, mGeometry.nVoxelsTotal - 1)
  {
    completeDataArray[i] = value;
  }
}

VolData& VolData::operator*=(const VolData& inputVol)
{
#pragma omp parallel for
  LOOP(i, 0, mGeometry.nVoxelsPerFrame - 1)
  {
    if (
      mDataArray[i] > EPSILON &&
      inputVol.mDataArray[i] > EPSILON)
    {
      mDataArray[i] *= inputVol.mDataArray[i];
    }
    else
    {
      mDataArray[i] = 0.0;
    }
  }

  return *this;
}

VolData& VolData::operator/=(const VolData& inputVol)
{
#pragma omp parallel for
  LOOP(i, 0, mGeometry.nVoxelsPerFrame - 1)
  {
    if (
      mDataArray[i] > EPSILON &&
      inputVol.mDataArray[i] > EPSILON)
    {
      mDataArray[i] /= inputVol.mDataArray[i];
    }
    else
    {
      mDataArray[i] = 0.0;
    }
  }

  return *this;
}

void VolData::setActiveFrame(int frame) const
{
  if (!(frame >= 0 && frame < mHeader.nFrames))
  {
    error("Invalid frame");
  }

  mActiveFrame = frame;

  mDataArray = mFrameVector[frame];
}

types::VoxelValue VolData::computeLineIntegral(
  types::PathElement* pathElementsArray) const
{
  types::VoxelValue line{0.0};

  for (auto pathIndex = 0;
       pathElementsArray[pathIndex].coord != -1;
       pathIndex++)
  {
    line += pathElementsArray[pathIndex].length *
      mDataArray[pathElementsArray[pathIndex].coord];
  }

  return line;
}

void VolData::projectLineIntegral(
  types::PathElement* pathElementsArray,
  types::VoxelValue line)
{
  for (auto pathIndex = 0;
       pathElementsArray[pathIndex].coord != -1;
       pathIndex++)
  {
#pragma omp atomic
    mDataArray[pathElementsArray[pathIndex].coord] +=
      pathElementsArray[pathIndex].length * line;
  }
}

void VolData::copyParameters(const VolData& vol)
{
  mHeader = vol.mHeader;
  mGeometry = vol.mGeometry;
}

void VolData::allocate(
  bool initialize,
  types::VoxelValue initValue)
{
  // After execution, volume is set to first frame

  // Deallocate in case it has already been allocated
  deallocate();

  // Allocate mDataArray
  mDataArray = (types::VoxelValue*)std::malloc(
    mGeometry.nVoxelsTotal * sizeof(types::VoxelValue));

  // Allocate frame vector
  mFrameVector.resize(mHeader.nFrames);
  LOOP(frame, 0, mHeader.nFrames - 1)
  {
    mFrameVector[frame] =
      mDataArray + frame * mGeometry.nVoxelsPerFrame;
  }

  if (initialize)
  {
    setAllVoxelsAllFrames(initValue);
  }
}

void VolData::deallocate()
{
  if (mFrameVector.size() > 0)
  {
    std::free(mFrameVector[0]);
    mFrameVector.clear();
  }

  mActiveFrame = 0;
  mDataArray = nullptr;
}
