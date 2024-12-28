#include <ScannerInterfileReader.h>

#include <KeyParser.h>

void readScannerInterfileHeader(
  const std::string& headerFileName,
  ScannerHeader& header)
{
  // Set default values
  header.setDefaults();

  KeyParser kp;

  kp.addStartKey("!SCANNER PARAMETERS");

  kp.addKey(
    "crystal dimensions XYZ in mm",
    &header.crystalDimsXYZ);
  kp.addKey(
    "crystal repeat numbers YZ",
    &header.crystalRepeatNumbersYZ);
  kp.addKey(
    "inter-crystal distance YZ in mm",
    &header.interCrystalDistanceYZ);

  kp.addKey(
    "module dimensions XYZ in mm",
    &header.moduleDimsXYZ);
  kp.addKey(
    "module repeat numbers YZ",
    &header.moduleRepeatNumbersYZ);
  kp.addKey(
    "inter-module distance YZ in mm",
    &header.interModuleDistanceYZ);

  kp.addKey(
    "rSector dimensions XYZ in mm",
    &header.rSectorDimsXYZ);
  kp.addKey(
    "rSector repeat number",
    &header.rSectorRepeatNumber);
  kp.addKey(
    "rSector inner radius in mm",
    &header.rSectorInnerRadius);

  kp.addStopKey("!END OF SCANNER PARAMETERS");

  kp.parse(headerFileName);

  // Check header values
  header.check();
}
