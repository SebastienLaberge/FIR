#include <ProjData.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <exception>

// Class to test the check method used for validating values
// inserted into ProjHeader after calling setDefaults
class ProjHeaderCheckTest: public testing::Test
{
protected:

  ProjHeaderCheckTest()
  {
    mProjHeader.setDefaults();
  }

  ProjHeader mProjHeader;
};

// Validation fails if no scanner geometry is provided
TEST_F(ProjHeaderCheckTest, NoSetValues)
{
  EXPECT_THROW(mProjHeader.check(), std::exception);
}

// The number of rings is correctly validated
TEST_F(ProjHeaderCheckTest, NRingsValidation)
{
  // Provide valid value for nCrystalsPerRing
  mProjHeader.nCrystalsPerRing = 4;

  // nRings not provided while mandatory
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with value zero
  mProjHeader.nRings = 0;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with negative value
  mProjHeader.nRings = -1;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with valid value given default span of 1
  mProjHeader.nRings = 1;
  EXPECT_NO_THROW(mProjHeader.check());
}

// The number of crystals per ring is correctly validated
TEST_F(ProjHeaderCheckTest, NCrystalsPerRingNotProvidedOrWrong)
{
  // Provide valid value for nRings
  mProjHeader.nRings = 1;

  // nCrystalsPerRing not provided while mandatory
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with value zero
  mProjHeader.nCrystalsPerRing = 0;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with negative value
  mProjHeader.nCrystalsPerRing = -1;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with value that is not a multiple of 4
  mProjHeader.nCrystalsPerRing = 3;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Provided with valid value
  mProjHeader.nCrystalsPerRing = 4;
  EXPECT_NO_THROW(mProjHeader.check());
}

// The segment span is correctly validated
TEST_F(ProjHeaderCheckTest, SegmentSpanValidation)
{
  // Provide valid values for scanner geometry
  mProjHeader.nRings = 2;
  mProjHeader.nCrystalsPerRing = 4;

  // Given two rings, the maximum value for segmentSpan is 3.

  // Value is zero
  mProjHeader.segmentSpan = 0;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is negative
  mProjHeader.segmentSpan = -1;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is even while still smaller than maximum value
  mProjHeader.segmentSpan = 2;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is greater than maximum value while correctly odd
  mProjHeader.segmentSpan = 5;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is valid
  mProjHeader.segmentSpan = 3;
  EXPECT_NO_THROW(mProjHeader.check());
}

// The number of segments is correctly validated
TEST_F(ProjHeaderCheckTest, NSegmentsValidation)
{
  // Provide valid values for scanner geometry
  mProjHeader.nRings = 2;
  mProjHeader.nCrystalsPerRing = 4;

  // Given two rings and the default span of 1, the maximum
  // value for nSegments is 3.

  // Value is zero
  mProjHeader.nSegments = 0;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is negative
  mProjHeader.nSegments = -1;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is even
  mProjHeader.nSegments = 2;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is greater than maximum value while correctly odd
  mProjHeader.nSegments = 5;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is valid
  mProjHeader.nSegments = 3;
  EXPECT_NO_THROW(mProjHeader.check());
}

// The number of tangential coordinates is correctly validated
TEST_F(ProjHeaderCheckTest, NTangCoordsValidation)
{
  // Provide valid values for scanner geometry
  mProjHeader.nRings = 2;
  mProjHeader.nCrystalsPerRing = 4;

  // Given four crystals per ring, the maximum number of
  // tangential coordinates is 3.

  // Note: The value of zero is not checked because the check
  // method replaces it with a default value

  // Value is negative
  mProjHeader.nTangCoords = -1;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is greater than maximum value
  mProjHeader.nTangCoords = 4;
  EXPECT_THROW(mProjHeader.check(), std::exception);

  // Value is valid
  mProjHeader.nTangCoords = 3;
  EXPECT_NO_THROW(mProjHeader.check());
}

// The default number of tangential coordinates is correctly
// calculated given the number of crystals per ring
TEST_F(ProjHeaderCheckTest, NTangCoordsDefaultValue)
{
  // Provide valid values for scanner geometry
  mProjHeader.nRings = 2;
  mProjHeader.nCrystalsPerRing = 4;

  // Given four crystals per ring, the default value for the
  // number of tangential coordinates is the maximum of 3
  EXPECT_NO_THROW(mProjHeader.check());
  EXPECT_EQ(mProjHeader.nTangCoords, 3);
}

// Test the fill method used for populating the ProjGeometry
// class from an instance of ProjHeader

// With a span on one
TEST(ProjGeometryFillTest, SpanOne)
{
  ProjHeader projHeader;
  projHeader.setDefaults();

  projHeader.nRings = 5;
  projHeader.nCrystalsPerRing = 32;
  projHeader.segmentSpan = 1;
  projHeader.nSegments = 3;
  projHeader.nTangCoords = 8;

  projHeader.check();

  ProjGeometry projGeometry;
  projGeometry.fill(projHeader);
  EXPECT_EQ(projGeometry.nBins, 1664);
  EXPECT_EQ(projGeometry.segOffset, 1);
  EXPECT_EQ(projGeometry.tangCoordOffset, 4);
  EXPECT_THAT(
    projGeometry.nAxialCoords,
    testing::ElementsAreArray(std::vector{4, 5, 4}));
  EXPECT_EQ(projGeometry.nViews, 16);
  EXPECT_EQ(projGeometry.halfSegmentSpan, 0);
  EXPECT_EQ(projGeometry.maxRingDiff, 1);
}

// With a span other than one
TEST(ProjGeometryFillTest, SpanNotOne)
{
  ProjHeader projHeader;
  projHeader.setDefaults();

  projHeader.nRings = 5;
  projHeader.nCrystalsPerRing = 32;
  projHeader.segmentSpan = 3;
  projHeader.nSegments = 3;
  projHeader.nTangCoords = 8;

  projHeader.check();

  ProjGeometry projGeometry;
  projGeometry.fill(projHeader);
  EXPECT_EQ(projGeometry.nBins, 2432);
  EXPECT_EQ(projGeometry.segOffset, 1);
  EXPECT_EQ(projGeometry.tangCoordOffset, 4);
  EXPECT_THAT(
    projGeometry.nAxialCoords,
    testing::ElementsAreArray(std::vector{5, 9, 5}));
  EXPECT_EQ(projGeometry.nViews, 16);
  EXPECT_EQ(projGeometry.halfSegmentSpan, 1);
  EXPECT_EQ(projGeometry.maxRingDiff, 4);
}
