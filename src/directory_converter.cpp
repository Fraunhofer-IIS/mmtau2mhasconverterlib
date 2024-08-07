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
#include <exception>
#include <sstream>
#include <string>
#include <vector>

// Project includes
#include "mmtau2mhasconverterlib/directory_converter.h"
#include "mmtau2mhasconverterlib/file_converter.h"
#include "directories.h"
#include "file_converter_pimpl.h"

namespace mmt {
namespace au2mhasconverterlib {
CDirectoryConverter::CDirectoryConverter(const SConfig& config) : m_config(config) {}

std::string CDirectoryConverter::getVersion() const {
  return "v" + std::to_string(mmtau2mhasconverterlib_VERSION_MAJOR) + "." +
         std::to_string(mmtau2mhasconverterlib_VERSION_MINOR) + "." +
         std::to_string(mmtau2mhasconverterlib_VERSION_PATCH);
}

void CDirectoryConverter::process() {
  std::vector<CDirectories::SConversion> conversionList =
      CDirectories::getFileConversionList(m_config.inputDirectoryPath, m_config.outputDirectoryPath,
                                          true, m_config.includeSubfolders, m_config.addMhmSuffix);

  // Print paths to debug log
  {
    std::stringstream sstream{};
    sstream << "[ ] Directory conversion..." << std::endl;
    for (const CDirectories::SConversion& entry : conversionList) {
      sstream << entry.inputFile << " \n  -> " << entry.outputFile << "\n";
    }
    m_config.logCallback(sstream.str());
  }

  size_t no = 0;
  size_t succeeded = 0;
  size_t existed = 0;
  for (const CDirectories::SConversion& entry : conversionList) {
    // Log status status
    {
      std::stringstream sstream{};
      sstream << "Converting " << no++ << " of " << conversionList.size() << "...";
      sstream << ((entry.inputFile.size() > 48)
                      ? entry.inputFile.substr(entry.inputFile.size() - 48)
                      : entry.inputFile)
              << std::endl;
      m_config.logCallback(sstream.str());
    }

    if (!m_config.replaceFiles) {
      if (CDirectories::checkFileExists(entry.outputFile)) {
        std::stringstream sstream{};
        sstream << "[ ] Skipping conversion of file that already exists (replaceFiles=Off) "
                << entry.outputFile << std::endl;
        m_config.logCallback(sstream.str());
        existed++;
        continue;
      }
    }

    CFileConverter::SConfig converterConfig;
    converterConfig.inputFile = entry.inputFile;
    converterConfig.outputFile = entry.outputFile + ".tmp";
    converterConfig.logCallback = m_config.logCallback;
    converterConfig.progressCallback = m_config.progressCallback;
    converterConfig.interruptCallback = m_config.interruptCallback;

    try {
      CFileConverterPimpl converter(converterConfig);
      converter.process();
    } catch (const std::exception& ex) {
      std::stringstream sstream{};
      sstream << "[E] Conversion of file " << entry.inputFile << " failed with " << ex.what()
              << std::endl;
      m_config.logCallback(sstream.str());
      continue;
    }

    if (m_config.interruptCallback()) {
      m_config.logCallback("Conversion stopped by user");
      break;
    }

    CDirectories::moveFile(entry.outputFile + ".tmp", entry.outputFile);
    succeeded++;
  }

  {
    std::stringstream sstream{};
    sstream << "Conversion completed [ ";
    sstream << succeeded << " succeeded, ";
    sstream << existed << " existed, ";
    sstream << (conversionList.size() - succeeded - existed) << " failed of ";
    sstream << conversionList.size() << " files ]" << std::endl;
    m_config.logCallback(sstream.str());
  }
}
}  // namespace au2mhasconverterlib
}  // namespace mmt
