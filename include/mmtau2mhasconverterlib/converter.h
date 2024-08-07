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

/**
 * @file converter.h
 *
 * @brief Main interface for the MHA to MHM converter.
 */
#pragma once

// System includes
#include <memory>
#include <vector>

// Internal includes
#include "mmtau2mhasconverterlib/version.h"

namespace mmt {
namespace au2mhasconverterlib {

//! Type alias to a buffer of raw bytes.
using ByteBuffer = std::vector<uint8_t>;

//! Container around an optional MPEG-H profile level indication value.
struct SProfileLevel {
  //! Sets the new stored profile level value
  void set(uint8_t profileLevel) noexcept {
    m_profileLevel = profileLevel;
    m_profileLevelSet = true;
  }

  //! Returns the stored profile level or a default value.
  uint8_t get() const noexcept;

  //! Returns whether a profile level value is explicitly set.
  bool isSet() const noexcept { return m_profileLevelSet; }

 private:
  bool m_profileLevelSet = false;
  uint8_t m_profileLevel = 0;
};

//! Container for converted MHAS configuration packet.
struct SMhasConfigOutput {
  //! The buffer containing the converted MHAS config packet.
  ByteBuffer config;
  //! The buffer containing the (optional) MHAS audio scene information packet.
  std::unique_ptr<ByteBuffer> asi;
  //! The binary blob containing the full MPEG-H 3D Audio config (MHAS config packet).
  ByteBuffer fullMpegHConfigBlob;
  //! The profile level of the MHAS configuration packet.
  SProfileLevel compatibleProfileLevel;
};

//! Container for converted MHAS frame packet.
struct SMhasFrameOutput {
  //! The buffer containing the (optional) effective MHAS config packet for this MHAS packet.
  std::unique_ptr<ByteBuffer> config;
  //! The buffer containing the (optional) effective MHAS audio scene information packet.
  std::unique_ptr<ByteBuffer> asi;
  //! The buffer containing the converted MHAS frame packet.
  ByteBuffer frame;
  //! Whether the output MHAS frame is an Immediate Playout Frame (IPF).
  bool isIpf = false;
  //! Whether the output MHAS frame is an independent frame (I-frame).
  bool isIndepFrame = false;
};

//! The main converter interface.
class CConverter {
 public:
  //! The configuration structure for the creation of a new converter instance
  struct SConverterConfiguration {
    //! The packet label for the initial MHAS packet, must be in range [1, 16].
    uint32_t initialPacketLabel = 1;
  };

  //! Creates a new converter object with the given configuration.
  CConverter(const SConverterConfiguration& configuration);

  //! Convert a single MPEG-H 3DA config packet.
  SMhasConfigOutput convertConfig(const ByteBuffer& mpegh3daConfig);

  //! Convert a single MPEG-H 3DA frame packet.
  SMhasFrameOutput convertFrame(const ByteBuffer& mpegh3daFrame);

  //! Returns the label of the last processed packet.
  uint32_t currentPacketLabel() const;

 private:
  SMhasFrameOutput convertIPF(const ByteBuffer& mpegh3daFrame);

  std::unique_ptr<ByteBuffer> m_currentConfig;
  std::unique_ptr<ByteBuffer> m_currentAsi;
  uint32_t m_currentPacketLabel = 0;
  uint64_t m_currentFrameNumber = 1;
  SConverterConfiguration m_config;
};
}  // namespace au2mhasconverterlib
}  // namespace mmt
