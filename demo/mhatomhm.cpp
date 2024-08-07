/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2022 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
#include <exception>
#include <iostream>
#include <string>

// External includes
#include "ilo/memory.h"

// Internal includes
#include "mmtau2mhasconverterlib/file_converter.h"
#include "logging.h"

using namespace mmt;
using namespace mmt::au2mhasconverterlib;

static void printUsage() {
  std::cout << "Usage: mhatomhm [options] -o <path to mhm1 MP4 ouput file> <path to mha1 MP4 file>"
            << std::endl;

  std::cout << "Options:" << std::endl;
  std::cout << "  -l <path>          (Optional) Path to write the log to" << std::endl;
  std::cout << "  -e copy|omit|reset (Optional) Edit list mode, \"copy\" (default), \"omit\" "
               "(don't copy) or \"reset\" to copy and set media time to zero"
            << std::endl;
  std::cout << "  -p <num>           (Optional) Value (1 to 16) to overwrite the initial packet "
               "label with"
            << std::endl;
}

int main(int argc, char* argv[]) {
  std::string inputFile;
  std::string outputFile;
  std::string logFile;
  std::string editList;
  uint32_t packetLabel = 1;

  if (argc < 4 /* program, input file, output flag, output file */) {
    printUsage();
    return EXIT_FAILURE;
  }

  for (int i = 1; i < argc - 1; i += 2) {
    if (std::string{"-o"} == argv[i]) {
      outputFile = argv[i + 1];
    } else if (std::string{"-l"} == argv[i]) {
      logFile = argv[i + 1];
    } else if (std::string{"-e"} == argv[i]) {
      editList = argv[i + 1];
    } else if (std::string{"-p"} == argv[i]) {
      try {
        packetLabel = std::stoul(argv[i + 1]);
      } catch (const std::exception&) {
        std::cout << "The packet label needs to be a numerical value, got: " << argv[i + 1]
                  << std::endl;
        printUsage();
        return EXIT_FAILURE;
      }
    } else if (std::string{"-h"} == argv[i] || std::string{"--help"} == argv[i]) {
      printUsage();
      return EXIT_SUCCESS;
    } else {
      std::cout << "Invalid command line parameter: " << argv[i] << std::endl;
      printUsage();
      return EXIT_FAILURE;
    }
  }
  inputFile = argv[argc - 1];

  if (inputFile.empty() || outputFile.empty()) {
    std::cout << "Input and output files need to be set" << std::endl;
    printUsage();
    return EXIT_FAILURE;
  }

  if (inputFile == outputFile) {
    std::cout << "inputFile can't be equal to outputFile" << std::endl;
    printUsage();
    return EXIT_FAILURE;
  }
  if (logFile == inputFile || logFile == outputFile) {
    std::cout << "logfile can't be equal to input or outputfile" << std::endl;
    printUsage();
    return EXIT_FAILURE;
  }
  if (packetLabel < 1 || packetLabel > 16) {
    std::cout << "Invalid packet Label provided." << std::endl;
    printUsage();
    return EXIT_FAILURE;
  }
  if (!editList.empty() && editList != "copy" && editList != "omit" && editList != "reset") {
    std::cout
        << "Invalid edit list action provided, allowed actions are 'copy', 'omit' and 'reset'."
        << std::endl;
    printUsage();
    return EXIT_FAILURE;
  }

  if (logFile.empty()) {
    LOG_DISABLE_LOGGING();  // disable to not clutter system logs or console
  } else {
    LOG_REDIRECT_TO_FILE(logFile + ".log");
  }

  try {
    CFileConverter::SConfig converterConfig;

    converterConfig.inputFile = inputFile;
    converterConfig.outputFile = outputFile;
    converterConfig.packetLabel = packetLabel;
    converterConfig.logCallback = [](const std::string& msg) { ILO_LOG_INFO("%s", msg.c_str()); };
    uint16_t lastProgress = 0;
    converterConfig.progressCallback = [&](uint16_t progress) {
      if (progress % 5 == 0 && lastProgress != progress) {
        std::cout << progress << "%..." << std::endl;
        lastProgress = progress;
      }
    };

    converterConfig.copyTrackUserData = true;
    converterConfig.copyMhap = true;
    converterConfig.copyEditList = true;
    converterConfig.insertSyncBeforeEveryFrame = true;

    if (editList == "copy") {
      converterConfig.copyEditList = true;
      converterConfig.resetEditlistMediaTime = false;
    } else if (editList == "omit") {
      converterConfig.copyEditList = false;
      converterConfig.resetEditlistMediaTime = false;
    } else if (editList == "reset") {
      converterConfig.copyEditList = true;
      converterConfig.resetEditlistMediaTime = true;
    }

    CFileConverter runner(converterConfig);

    std::cout << "Converting " << inputFile << " to " << outputFile << std::endl;
    if (logFile.empty()) {
      std::cout << "(log file is disabled, can be enabled with -l option)" << std::endl;
    } else {
      std::cout << "(see detailed log file at " << logFile
                << ".log and MP4 structure before and after conversion at " << logFile
                << "_mp4.log)" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "0%..." << std::endl;
    runner.process();
  } catch (std::runtime_error& error) {
    std::cerr << error.what() << std::endl;
    return 5;
  }

  catch (...) {
    std::cerr << "Error while processing" << std::endl;
    return 6;
  }
  std::cout << "Done!" << std::endl;
  return 0;
}
