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
#include <algorithm>
#include <cctype>

// Internal includes
#include "helpers.h"

namespace mmt {
namespace au2mhasconverterlib {
std::vector<std::string> splitString(const std::string& inString, char delimiter) {
  size_t startPos = 0;
  size_t endPos = std::string::npos;
  std::vector<std::string> results;
  while ((endPos = inString.find(delimiter, startPos)) != std::string::npos) {
    auto token = inString.substr(startPos, endPos - startPos);
    startPos = endPos + sizeof(delimiter);
    results.push_back(token);
  }
  results.push_back(inString.substr(startPos));
  return results;
}

std::string replaceInString(const std::string& inString, const std::string& searchFor,
                            const std::string& replaceWith) {
  std::string outString = inString;
  for (size_t pos = 0; (pos = outString.find(searchFor, pos)) != std::string::npos;
       pos += replaceWith.size()) {
    outString.replace(pos, searchFor.size(), replaceWith);
  }
  return outString;
}

std::string replaceInString(const std::string& inString, char searchFor, char replaceWith) {
  std::string outString = inString;
  for (size_t pos = 0; (pos = outString.find(searchFor, pos)) != std::string::npos; ++pos) {
    outString[pos] = replaceWith;
  }
  return outString;
}

bool endsWith(const std::string& inString, const std::string& endString) {
  if (endString.size() > inString.size()) {
    return false;
  }
  return std::equal(inString.end() - static_cast<std::ptrdiff_t>(endString.size()), inString.end(),
                    endString.begin());
}

bool endsWithIgnoreCase(const std::string& inString, const std::string& endString) {
  if (endString.size() > inString.size()) {
    return false;
  }
  return std::equal(inString.end() - static_cast<std::ptrdiff_t>(endString.size()), inString.end(),
                    endString.begin(),
                    [](char a, char b) { return std::toupper(a) == std::toupper(b); });
}

size_t countOccurences(const std::string& inString, char token) {
  return static_cast<std::size_t>(std::count(inString.begin(), inString.end(), token));
}
}  // namespace au2mhasconverterlib
}  // namespace mmt
