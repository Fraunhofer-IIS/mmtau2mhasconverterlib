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
#include <cstring>
#include <string>

// External includes
#include "ilo/memory.h"

// Internal includes
#include "converter_mha.h"
#include "file_converter_pimpl.h"
#include "logging.h"

namespace mmt {
namespace au2mhasconverterlib {
std::unique_ptr<CConverter> openMhaConverter(uint32_t packetLabel) {
  CConverter::SConverterConfiguration converterConfig;
  converterConfig.initialPacketLabel = packetLabel;
  return ilo::make_unique<CConverter>(converterConfig);
}

//! Append sync packet to the given buffer.
static void appendSyncPacket(ilo::ByteBuffer& sample) {
  static const ilo::ByteBuffer mhasSyncPacket = {0xC0, 0x01, 0xA5};
  sample.insert(sample.end(), mhasSyncPacket.begin(), mhasSyncPacket.end());
}

mmt::isobmff::CSample convertMhaSampleToMhm(const CFileConverter::SConfig& config,
                                            CConverter& mhaConverter,
                                            const mmt::isobmff::CSample& inSample,
                                            const ilo::ByteBuffer& mpeghConfigFromMp4,
                                            bool firstSample) {
  mmt::isobmff::CSample outSample;

  // note: required every iteration due to alternating packet labels
  SMhasConfigOutput fileConfig = mhaConverter.convertConfig(mpeghConfigFromMp4);

  SMhasFrameOutput frame = mhaConverter.convertFrame(inSample.rawData);
  outSample.isSyncSample = frame.config != nullptr;

  if (firstSample && !outSample.isSyncSample) {
    ILO_ASSERT(frame.isIndepFrame,
               "First sample is not an Indep frame, this is an unrecoverable error - please check "
               "the provided input file.");

    frame.config = ilo::make_unique<ilo::ByteBuffer>(fileConfig.config);
    frame.asi = std::move(fileConfig.asi);

    if (!frame.isIpf) {
      config.logCallback(
          "First sample is not an IPF, playback will may not be possible until the first IPF has "
          "been received.");
    }
  }

  ilo::ByteBuffer mhmSample;

  if (config.insertSyncBeforeEveryFrame) {
    config.logCallback("Inserting Sync before every Frame");
    appendSyncPacket(mhmSample);
  } else {
    if (config.insertSyncBeforeFirstFrame && firstSample) {
      config.logCallback("Inserting Sync before first Frame");
      appendSyncPacket(mhmSample);
    } else {
      if (config.insertSyncBeforeEveryIpf && frame.isIpf) {
        config.logCallback("Inserting Sync before IPF");
        appendSyncPacket(mhmSample);
      }
    }
  }

  if (frame.config) {
    mhmSample.insert(mhmSample.end(), frame.config->begin(), frame.config->end());
  }

  if (frame.asi) {
    mhmSample.insert(mhmSample.end(), frame.asi->begin(), frame.asi->end());
  }
  mhmSample.insert(mhmSample.end(), frame.frame.begin(), frame.frame.end());
  outSample.ctsOffset = inSample.ctsOffset;
  outSample.duration = inSample.duration;
  outSample.rawData = mhmSample;

  if (!frame.isIpf && frame.isIndepFrame) {
    // Signal as ISO/IEC 14496-12 AudioPreRollEntry in accordance with ISO/IEC 23008-3
    // subclause 20.2
    outSample.sampleGroupInfo =
        mmt::isobmff::SSampleGroupInfo(mmt::isobmff::SampleGroupType::prol, 1, 0);
  }

  return outSample;
}
}  // namespace au2mhasconverterlib
}  // namespace mmt
