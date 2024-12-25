#include <operations.h>

#include <console.h>
#include <macros.h>

#include <cmath>
#include <cstdlib>

namespace operations
{
void convolve(
  VolData& vol,
  std::vector<float> fwhmXYZ,
  float cutRadius)
{
  const auto& header = vol.getHeader();

  if (fwhmXYZ[0] > 0.0 && fwhmXYZ[1] > 0.0 && fwhmXYZ[2] > 0.0)
  {
    // Initialize auxiliary volumes

    VolData copyVol;
    copyVol.allocateSingleFrameFromMultiVol(vol);

    VolData image1;
    image1.allocateSingleFrameFromMultiVol(vol);

    VolData image2;
    image2.allocateSingleFrameFromMultiVol(vol);

    // Initialize kernel in X
    const auto sigmaX = fwhmXYZ[0] / 2.3548f;
    const auto halfKernelSizeX =
      (static_cast<int>(
         6.0 * sigmaX / header.voxelExtent.pixelWidth) +
       1) /
      2;
    auto* kernelX = (float*)std::malloc(
      (2 * halfKernelSizeX + 1) * sizeof(float));
    LOOP(ki, -halfKernelSizeX, halfKernelSizeX)
    {
      kernelX[halfKernelSizeX + ki] = std::exp(
        -(ki * ki * header.voxelExtent.pixelWidth *
          header.voxelExtent.pixelWidth) /
        (2 * sigmaX * sigmaX));
    }
    // Initialize kernel in Y
    const auto sigmaY = fwhmXYZ[1] / 2.3548f;
    const auto halfKernelSizeY =
      (static_cast<int>(
         6.0 * sigmaY / header.voxelExtent.pixelHeight) +
       1) /
      2;
    auto* kernelY = (float*)std::malloc(
      (2 * halfKernelSizeY + 1) * sizeof(float));
    LOOP(ki, -halfKernelSizeY, halfKernelSizeY)
    kernelY[halfKernelSizeY + ki] = std::exp(
      -(ki * ki * header.voxelExtent.pixelHeight *
        header.voxelExtent.pixelHeight) /
      (2 * sigmaY * sigmaY));

    // Initialize kernel in Z
    const auto sigmaZ = fwhmXYZ[2] / 2.3548;
    const auto halfKernelSizeZ =
      (static_cast<int>(
         6.0 * sigmaZ / header.voxelExtent.sliceThickness) +
       1) /
      2;
    auto* kernelZ = (float*)std::malloc(
      (2 * halfKernelSizeZ + 1) * sizeof(float));
    LOOP(ki, -halfKernelSizeZ, halfKernelSizeZ)
    kernelZ[ki + halfKernelSizeZ] = std::exp(
      -(ki * ki * header.voxelExtent.sliceThickness *
        header.voxelExtent.sliceThickness) /
      (2 * sigmaZ * sigmaZ));

    // Loop over frames
    LOOP(frame, 0, header.nFrames - 1)
    {
      vol.setActiveFrame(frame);
      copyVol.assignFrame(vol, frame);

      // Apply kernel in X
#pragma omp parallel for
      LOOP(k, 0, header.volSize.nSlices - 1)
      LOOP(j, 0, header.volSize.nPixelsY - 1)
      LOOP(i, 0, header.volSize.nPixelsX - 1)
      {
        float sum{0.0};
        float norm{0.0};

        LOOP(ki, -halfKernelSizeX, halfKernelSizeX)
        {
          if (i + ki >= 0 && i + ki < header.volSize.nPixelsX)
          {
            const auto kv = kernelX[halfKernelSizeX + ki];
            sum += kv * vol.getVoxel(i + ki, j, k);
            norm += kv;
          }
        }

        if (norm > 0.0)
        {
          image1.setVoxel(i, j, k, sum / norm);
        }
      }

      // Apply kernel in Y
#pragma omp parallel for
      LOOP(k, 0, header.volSize.nSlices - 1)
      LOOP(j, 0, header.volSize.nPixelsY - 1)
      LOOP(i, 0, header.volSize.nPixelsX - 1)
      {
        float sum{0.0};
        float norm{0.0};

        LOOP(ki, -halfKernelSizeY, halfKernelSizeY)
        {
          if (j + ki >= 0 && j + ki < header.volSize.nPixelsY)
          {
            const auto kv = kernelY[halfKernelSizeY + ki];
            sum += kv * image1.getVoxel(i, j + ki, k);
            norm += kv;
          }
        }

        if (norm > 0.0)
        {
          image2.setVoxel(i, j, k, sum / norm);
        }
      }

      // Apply kernel in Z
#pragma omp parallel for
      LOOP(k, 0, header.volSize.nSlices - 1)
      LOOP(j, 0, header.volSize.nPixelsY - 1)
      LOOP(i, 0, header.volSize.nPixelsX - 1)
      {
        float sum{0.0};
        float norm{0.0};

        LOOP(ki, -halfKernelSizeZ, halfKernelSizeZ)
        {
          if (k + ki >= 0 && k + ki < header.volSize.nSlices)
          {
            const auto kv = kernelZ[halfKernelSizeZ + ki];
            sum += kv * image2.getVoxel(i, j, k + ki);
            norm += kv;
          }
        }

        if (norm > 0.0)
        {
          vol.setVoxel(i, j, k, sum / norm);
        }
      }

      // Set voxels outside cylindrical fov with original
      // values to suppress artifacts
      if (cutRadius > 0.0)
      {
        const auto fwhm = MAX(fwhmXYZ[0], fwhmXYZ[1]);
        const auto& volExtent = vol.getVolExtent();

#pragma omp parallel for
        LOOP(k, 0, header.volSize.nSlices - 1)
        LOOP(j, 0, header.volSize.nPixelsY - 1)
        LOOP(i, 0, header.volSize.nPixelsX - 1)
        {
          const auto px = i * header.voxelExtent.pixelWidth +
            header.voxelExtent.pixelWidth / 2.0 -
            volExtent.sliceWidth / 2.0;

          const auto py = j * header.voxelExtent.pixelHeight +
            header.voxelExtent.pixelHeight / 2.0 -
            volExtent.sliceHeight / 2.0;

          if (
            std::sqrt(px * px + py * py) >=
            cutRadius - 5.0 * fwhm)
          {
            vol.setVoxel(i, j, k, copyVol.getVoxel(i, j, k));
          }
        }
      }
    }

    std::free(kernelX);
    std::free(kernelY);
    std::free(kernelZ);
  }
}

void cutCircle(VolData& vol, float cutRadius)
{
  const auto& header = vol.getHeader();
  const auto& volExtent = vol.getVolExtent();

  if (cutRadius > 0.0)
  {
#pragma omp parallel for
    LOOP(k, 0, header.volSize.nSlices - 1)
    LOOP(j, 0, header.volSize.nPixelsY - 1)
    LOOP(i, 0, header.volSize.nPixelsX - 1)
    {
      const auto px = i * header.voxelExtent.pixelWidth +
        header.voxelExtent.pixelWidth / 2.0 -
        volExtent.sliceWidth / 2.0;

      const auto py = j * header.voxelExtent.pixelHeight +
        header.voxelExtent.pixelHeight / 2.0 -
        volExtent.sliceHeight / 2.0;

      if (std::sqrt(px * px + py * py) > cutRadius)
      {
        vol.setVoxel(i, j, k, 0.0);
      }
    }
  }
}

void applyMask(VolData& vol, VolData& maskVol)
{
  if (vol.getHeader() != maskVol.getHeader())
  {
    error("Mask volume not the same size");
  }

  auto* dataArray = vol.getDataArray();
  const auto* maskDataArray = maskVol.getDataArray();
  const auto nVoxelsPerFrame = vol.getNVoxelsPerFrame();

#pragma omp parallel for
  LOOP(i, 0, nVoxelsPerFrame - 1)
  {
    if (maskDataArray[i] <= 0)
    {
      dataArray[i] = 0.0;
    }
  }
}

void HounsfieldToMuMap(VolData& vol)
{
  // Bi-linear scaling:
  // 1) HU in [-infinity,-1000[ => mu = 0.0 mm^-1
  // 2) HU in [-1000,0[         => mu in [0.0, 0.0096[ mm^-1
  // 3) HU in [0,1000[          => mu in [0.0096, 0.015[ mm^-1
  // 4) HU greater than 1000: Use line 3

  const types::VoxelValue waterMu{0.0096};
  const types::VoxelValue thousandMu{0.015};

  const types::VoxelValue scale1{
    waterMu / static_cast<types::VoxelValue>(1000.0)};
  const types::VoxelValue scale2{
    (thousandMu - waterMu) /
    static_cast<types::VoxelValue>(1000.0)};

  auto* dataArray = vol.getDataArray();
  const auto nVoxelsPerFrame = vol.getNVoxelsPerFrame();

#pragma omp parallel for
  LOOP(i, 0, nVoxelsPerFrame - 1)
  {
    if (dataArray[i] <= -1000.0)
    {
      // Region 1
      dataArray[i] = 0.0;
    }
    else if (dataArray[i] <= 0.0)
    {
      // Region 2
      dataArray[i] *= scale1;
      dataArray[i] += waterMu;
    }
    else
    {
      // Region 3
      dataArray[i] *= scale2;
      dataArray[i] += waterMu;
    }
  }
}
}
