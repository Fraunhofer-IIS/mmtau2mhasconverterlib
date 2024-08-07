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
#include <string>
#include <vector>

// External includes
#include "ilo/bytebuffertools.h"
#include "ilo/memory.h"
#include "mmtisobmff/helper/commonhelpertools.h"
#include "mmtisobmff/reader/input.h"
#include "mmtisobmff/writer/output.h"
#include "mmtmhasparserlib/mhasutilities.h"

// Internal includes
#include "converter_helpers.h"
#include "mmtau2mhasconverterlib/file_converter.h"
#include "logging.h"

namespace mmt {
namespace au2mhasconverterlib {
mmt::isobmff::CTrackInfo openReader(const std::string& inputFile,
                                    std::unique_ptr<mmt::isobmff::CIsobmffReader>& reader,
                                    std::unique_ptr<mmt::isobmff::CMpeghTrackReader>& trackReader) {
  try {
    reader = ilo::make_unique<mmt::isobmff::CIsobmffReader>(
        ilo::make_unique<mmt::isobmff::CIsobmffFileInput>(inputFile));
  } catch (...) {
    ILO_FAIL("Open input file failed");
  }

  ILO_ASSERT(reader->trackCount() == 1, "Only single track files are supported");

  try {
    trackReader = reader->trackByIndex<mmt::isobmff::CMpeghTrackReader>(0);
    return reader->trackInfos().at(0);
  } catch (...) {
    ILO_FAIL("Open MPEG-H track reader failed");
  }
}

void openWriter(const CFileConverter::SConfig& config,
                std::unique_ptr<mmt::isobmff::CIsobmffWriter>& writer,
                std::unique_ptr<mmt::isobmff::CMpeghTrackWriter>& trackWriter,
                const std::unique_ptr<mmt::isobmff::CIsobmffReader>& reader,
                const mmt::isobmff::CTrackInfo& trackInfo,
                const std::unique_ptr<mmt::isobmff::CMpeghTrackReader>& trackReader,
                std::unique_ptr<mmt::isobmff::config::CMhaDecoderConfigRecord>&& mhaDcr,
                uint8_t profileLevel) {
  try {
    mmt::isobmff::CIsobmffFileWriter::SOutputConfig outputConfig;
    outputConfig.outputUri = config.outputFile;

    mmt::isobmff::SMovieConfig movieConfig;
    movieConfig.currentTimeInUtc = mmt::isobmff::tools::currentUTCTime();
    movieConfig.majorBrand = ilo::toFcc("mp42");
    movieConfig.movieTimeScale = reader->movieInfo().timeScale;

    movieConfig.compatibleBrands = reader->movieInfo().compatibleBrands;

    writer = ilo::make_unique<mmt::isobmff::CIsobmffFileWriter>(outputConfig, movieConfig);

    mmt::isobmff::SMpeghMhm1TrackConfig trackConfig;
    trackConfig.language = reader->trackInfos()[0].language;
    trackConfig.mediaTimescale = reader->trackInfos()[0].timescale;
    trackConfig.sampleRate = trackReader->sampleRate();
    trackConfig.configRecord = std::move(mhaDcr);

    if (config.copyMhap) {
      ILO_LOG_INFO("Transfering profileAndLevelCompatibleSet from bitstream: %u",
                   static_cast<uint32_t>(profileLevel));
      trackConfig.profileAndLevelCompatibleSets = std::vector<uint8_t>({profileLevel});
    } else {
      ILO_LOG_WARNING("Copy profileAndLevelCompatibleSets is disabled");
    }

    trackWriter = writer->trackWriter<mmt::isobmff::CMpeghTrackWriter>(trackConfig);

    if (config.copyTrackUserData) {
      // copy user data
      for (const auto& entry : trackInfo.userData) {
        trackWriter->addUserData(entry);
      }
    }

    if (config.copyEditList) {
      auto trackinfo = reader->trackInfos()[0];
      for (const auto& editListEntry : trackinfo.editList) {
        if (config.resetEditlistMediaTime) {
          auto copiedEntry = editListEntry;
          copiedEntry.mediaTime = 0;
          trackWriter->addEditListEntry(copiedEntry);
        } else {
          trackWriter->addEditListEntry(editListEntry);
        }
      }
    }
  } catch (const std::exception& err) {
    ILO_FAIL("Open output file failed with exception: ", err.what());
  }
}
}  // namespace au2mhasconverterlib
}  // namespace mmt
