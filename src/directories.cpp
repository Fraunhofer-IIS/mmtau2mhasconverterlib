/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2021 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

// System includes
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

// Internal includes
#include "directories.h"
#include "helpers.h"
#include "logging.h"

using namespace mmt::au2mhasconverterlib;
static const std::string MP4_EXTENSION = ".mp4";

#if defined(__APPLE__) || defined(__linux) || defined(__unix) || defined(__posix)

#include <climits>
#include <cstring>
#include <dirent.h>
#include <functional>
#include <sys/stat.h>

static void recursiveDirectorySearchHelper(const std::string& inPath,
                                           std::vector<std::string>& results) {
  std::string path = inPath;
  if (path.empty()) {
    return;
  }
  if (path.back() != CDirectories::getPathSeparator()) {
    path += CDirectories::getPathSeparator();
  }

  if (DIR* dir = opendir(path.c_str())) {
    while (struct dirent* item = readdir(dir)) {
      std::string name = item->d_name;
      if (name.empty() || name == "." || name == "..") {
        continue;
      } else if (item->d_type == DT_REG) {
        results.push_back(path + name);
      } else if (item->d_type == DT_DIR) {
        recursiveDirectorySearchHelper(path + name + CDirectories::getPathSeparator(), results);
      }
    }
    closedir(dir);
  }
}

CDirectories::ECreateDirectoryReturn CDirectories::createDirectory(const std::string& path) {
  if (path == "/") {
    return ECreateDirectoryReturn::EXISTS;
  }
  if (mkdir(path.c_str(), 0755) != 0) {
    if (errno == EEXIST) {
      return ECreateDirectoryReturn::EXISTS;
    }
    return ECreateDirectoryReturn::FAILED;
  }
  return ECreateDirectoryReturn::CREATED;
}

char CDirectories::getPathSeparator() {
  return '/';
}

#elif defined(_WIN64) || defined(_WIN32)

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <locale>
#include <codecvt>
#include <cwchar>

static std::wstring stringToWString(const std::string& in) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.from_bytes(in);
}

static std::string wstringToString(const std::wstring& in) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.to_bytes(in);
}

static void recursiveDirectorySearchHelper(const std::string& inPath,
                                           std::vector<std::string>& results) {
  std::wstring path = stringToWString(inPath);

  if (path.empty()) {
    return;
  }
  if (path.back() != '\\') {
    path += L'\\';
  }

  intptr_t dir = 0;
  struct _wfinddata_t item;
  std::wstring p;

  if ((dir = _wfindfirst(p.assign(path).append(L"*").c_str(), &item)) != -1) {
    do {
      if (std::wcscmp(item.name, L".") == 0 || std::wcscmp(item.name, L"..") == 0) {
        continue;
      } else if (!(item.attrib & _A_SUBDIR)) {
        std::wstring resultW = p.assign(path).append(item.name);
        results.push_back(wstringToString(resultW));
      } else {
        recursiveDirectorySearchHelper(wstringToString(p.assign(path).append(item.name)), results);
      }
    } while (_wfindnext(dir, &item) == 0);
  }
  _findclose(dir);
}

CDirectories::ECreateDirectoryReturn CDirectories::createDirectory(const std::string& path) {
  if (path.size() == 3 && path[1] == ':') {
    // Skip [DriverLetter:]
    return ECreateDirectoryReturn::EXISTS;
  }
  if (!(CreateDirectory(path.c_str(), nullptr))) {
    if (ERROR_ALREADY_EXISTS == GetLastError()) {
      return ECreateDirectoryReturn::EXISTS;
    } else {
      return ECreateDirectoryReturn::FAILED;
    }
  }
  return ECreateDirectoryReturn::CREATED;
}

char CDirectories::getPathSeparator() {
  return '\\';
}

#else

void CDirectories::recursiveDirectorySearchHelper(const std::string& path,
                                                  std::vector<std::string>& /* results */) {
  throw std::runtime_error("recursive directory search not implemented for this platform");
}

CDirectories::ECreateDirectoryReturn CDirectories::createDirectory(const std::string& path) {
  throw std::runtime_error("create directory not implemented for this platform");
}

char CDirectories::getPathSeparator() {
  throw std::runtime_error("path separator not implemented for this platform");
}

#endif

std::vector<std::string> CDirectories::recursiveDirectorySearch(const std::string& path) {
  std::vector<std::string> results{};
  recursiveDirectorySearchHelper(path, results);
  return results;
}

bool CDirectories::checkFileExists(const std::string& fileName) {
  std::ifstream file(fileName);
  return file.good();
}

void CDirectories::moveFile(const std::string& sourceFile, const std::string& destinationFile) {
  {
    std::ifstream in(sourceFile, std::ios::in | std::ios::binary);
    std::ofstream out(destinationFile, std::ios::out | std::ios::binary);
    out << in.rdbuf();
  }
  std::remove(sourceFile.c_str());
}

static std::size_t calculatePathDepth(const std::string& path, char separator) {
  size_t depth = countOccurences(path, separator);
#ifdef _WIN32
  if (separator != '/') {
    // Windows sometimes (at least the explorer.exe) also accepts the '/' as file separator, so do
    // here too
    depth += countOccurences(path, '/');
  }
#endif
  return depth;
}

std::vector<CDirectories::SConversion> CDirectories::getFileConversionList(
    std::string inputDirectoryPath, std::string outputDirectoryPath, bool createFolders,
    bool includeSubfolders, bool addMhmSuffix) {
  std::vector<CDirectories::SConversion> conversionList;

  ILO_ASSERT(!inputDirectoryPath.empty(), "empty input directory path");
  ILO_ASSERT(!outputDirectoryPath.empty(), "empty output directory path");

  auto addTrailingSlashToPath = [](std::string path) {
    if (path.back() != getPathSeparator()) {
      path += getPathSeparator();
    }
    return path;
  };
  inputDirectoryPath = addTrailingSlashToPath(inputDirectoryPath);
  outputDirectoryPath = addTrailingSlashToPath(outputDirectoryPath);

  std::vector<std::string> inputFilePaths = recursiveDirectorySearch(inputDirectoryPath);
  size_t inputPathDepth = calculatePathDepth(inputDirectoryPath, getPathSeparator());

  for (const std::string& inputFilePath : inputFilePaths) {
    if (inputFilePath.size() < (inputDirectoryPath.size() + MP4_EXTENSION.size())) {
      continue;
    }

    if (!endsWithIgnoreCase(inputFilePath, MP4_EXTENSION)) {
      continue;
    }

    if (!includeSubfolders) {
      size_t filePathDepth = calculatePathDepth(inputFilePath, getPathSeparator());
      if (inputPathDepth != filePathDepth) {
        continue;
      }
    }

    std::string outputFilePath =
        replaceInString(inputFilePath, inputDirectoryPath, outputDirectoryPath);

    if (addMhmSuffix) {
      outputFilePath.insert(outputFilePath.size() - MP4_EXTENSION.size(), "_mhm");
    }

    if (createFolders) {
      std::vector<std::string> outputFilePathSections =
          splitString(outputFilePath, getPathSeparator());
      std::string currentPath;
      // note: keep in mind - unix paths may start with a path separator
      for (size_t i = 0; i < outputFilePathSections.size() - 1; i++) {
        currentPath.append(outputFilePathSections[i]).push_back(getPathSeparator());
        ILO_ASSERT(createDirectory(currentPath) != ECreateDirectoryReturn::FAILED,
                   "Error creating Directory %s", currentPath.c_str());
      }
    }

    conversionList.emplace_back(inputFilePath, outputFilePath);
  }
  return conversionList;
}
