#pragma once

#include <ScannerHeader.h>

#include <string>

void readScannerInterfileHeader(
  const std::string& headerFileName,
  ScannerHeader& header);
