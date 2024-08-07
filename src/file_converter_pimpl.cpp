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
#include <memory>
#include <string>

// External includes
#include "ilo/common_types.h"
#include "mmtisobmff/reader/reader.h"
#include "mmtisobmff/reader/trackreader.h"
#include "mmtisobmff/writer/trackwriter.h"
#include "mmtisobmff/writer/writer.h"

// Internal includes
#include "converter_helpers.h"
#include "converter_mha.h"
#include "converter_mhm.h"
#include "file_converter_pimpl.h"
#include "logging.h"

using namespace mmt::au2mhasconverterlib;

CFileConverterPimpl::CFileConverterPimpl(const CFileConverter::SConfig& config)
    : m_config(config) {}

void CFileConverterPimpl::process() {
  m_config.logCallback("Start processing on " + m_config.inputFile + " to " + m_config.outputFile);

  std::unique_ptr<mmt::isobmff::CIsobmffReader> reader;
  std::unique_ptr<mmt::isobmff::CMpeghTrackReader> trackReader;
  auto trackInfo = openReader(m_config.inputFile, reader, trackReader);
  mmt::isobmff::Codec codec = reader->trackInfos()[0].codec;

  std::unique_ptr<CConverter> mhaConverter;
  ilo::ByteBuffer mpeghConfigFromMp4;

  ILO_ASSERT(codec == mmt::isobmff::Codec::mpegh_mha || codec == mmt::isobmff::Codec::mpegh_mhm,
             "Codec of first track is neither mha nor  mhm");
  if (codec == mmt::isobmff::Codec::mpegh_mha) {
    mhaConverter = openMhaConverter(m_config.packetLabel);
    mpeghConfigFromMp4 = trackReader->mhaDecoderConfigRecord()->mpegh3daConfig();
  } else if (codec == mmt::isobmff::Codec::mpegh_mhm) {
    mhaConverter = openMhmConverter(m_config.packetLabel);
  }

  // Handle MPEG-H config on MP4 File-Format level
  std::unique_ptr<mmt::isobmff::config::CMhaDecoderConfigRecord> mhaDcr =
      trackReader->mhaDecoderConfigRecord();

  SMhasConfigOutput converterOut;
  std::unique_ptr<mmt::isobmff::config::CMhaDecoderConfigRecord> mhaDcrConverted = nullptr;
  if (mhaDcr) {
    ilo::ByteBuffer mhaDcrBinaryBlob = mhaDcr->mpegh3daConfig();

    // note: open a new mhaConverter for this, to keep the internal state separate w.r.t.
    // alternating MHAS PacketLabels
    auto mhaConverter2 = openMhaConverter(m_config.packetLabel);

    converterOut = mhaConverter2->convertConfig(mhaDcrBinaryBlob);
    m_config.logCallback("Profile Level " +
                         std::to_string(converterOut.compatibleProfileLevel.get()));
    ilo::ByteBuffer mhaDcrBinaryBlobConverted = converterOut.fullMpegHConfigBlob;

    // move over also the outer shell of CMhaDecoderConfigRecord. The mpegh3daConfig Binary Blob is
    // only part of it.
    mhaDcrConverted = std::move(mhaDcr);
    mhaDcrConverted->setMpegh3daConfig(mhaDcrBinaryBlobConverted);
  } else {
    m_config.logCallback(
        "WARN: No Config on MP4-Level of input file, will write no MP4-Level Config");
  }

  std::unique_ptr<mmt::isobmff::CIsobmffWriter> writer;
  std::unique_ptr<mmt::isobmff::CMpeghTrackWriter> trackWriter;
  openWriter(m_config, writer, trackWriter, reader, trackInfo, trackReader,
             std::move(mhaDcrConverted), converterOut.compatibleProfileLevel.get());

  mmt::isobmff::CSample inSample;
  trackReader->nextSample(inSample);

  size_t totalLoops = reader->trackInfos()[0].sampleCount;
  if (totalLoops == 0) {
    ILO_FAIL("Input file contains no samples.");
  }

  bool firstSample = true;
  size_t currentLoop = 0;
  while (!inSample.empty()) {
    mmt::isobmff::CSample outSample;
    if (codec == mmt::isobmff::Codec::mpegh_mha) {
      outSample =
          convertMhaSampleToMhm(m_config, *mhaConverter, inSample, mpeghConfigFromMp4, firstSample);
    } else if (codec == mmt::isobmff::Codec::mpegh_mhm) {
      outSample = cleanMhmSample(m_config, *mhaConverter, inSample, firstSample);
    }
    ILO_ASSERT(!outSample.rawData.empty(), "sample raw data is empty after patch");
    trackWriter->addSample(outSample);

    trackReader->nextSample(inSample);
    firstSample = false;

    if (m_config.interruptCallback()) {
      return;
    }

    currentLoop += 1;
    // The two passes measure and patch are half the progress each
    size_t progress = (currentLoop * 100) / totalLoops;
    if (progress <= 100) {
      m_config.progressCallback(static_cast<uint16_t>(progress));
    }
  }

  m_config.progressCallback(100);
  if (m_config.interruptCallback()) {
    m_config.logCallback("Processing Thread Cancelled");
  } else {
    m_config.logCallback("Processing Thread Finished");
  }
}
