/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2019 - 2024 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
#include <cinttypes>
#include <cmath>
#include <string>

// External includes
#include "ilo/bitbuffer.h"
#include "ilo/bitparser.h"
#include "ilo/memory.h"
#include "mmtmhasparserlib/mhasasipacket.h"
#include "mmtmhasparserlib/mhasconfigpacket.h"
#include "mmtmhasparserlib/mhasframepacket.h"
#include "mmtmhasparserlib/mhaspacket.h"
#include "mmtmhasparserlib/mhasutilities.h"

// Internal includes
#include "mmtau2mhasconverterlib/converter.h"
#include "mmtau2mhasconverterlib/log_redirect.h"
#include "converter_helpers.h"
#include "logging.h"

using namespace mmt::au2mhasconverterlib;

static constexpr uint64_t MAX_PACKET_LABEL_MAIN_STREAM = 16;
static constexpr uint32_t ID_EXT_ELE_AUDIOPREROLL = 3;

static constexpr uint32_t ID_CONFIG_EXT_AUDIOSCENE_INFO = 3;
static constexpr uint32_t ID_CONFIG_EXT_COMPATIBLE_PROFILELVL_SET = 7;

enum class SB_VIOLATIONS : uint32_t {
  INVALID_PHASE_STRENGTH = 0,
  SIGNAL_TYPE_HOA = 1,
  SIGNAL_TYPE_SAOC = 2,
  INVALID_QCE_INDEX = 3,
  INVALID_LPD_STEREO_INDEX = 4,
  INVALID_TW_MDCT_VALUE = 5,
  INVALID_FULLBAND_LPD_VALUE = 6,
  INVALID_CORE_MODE_VALUE = 7,
  INVALID_COMMON_MAX_SFB_VALUE = 8,
  INVALID_TNS_ON_LR_VALUE = 9,
  INVALID_FAC_DATA_PRESENT_VALUE = 10
};

/*! According to ISO/IEC 23008-3, value table for mpegh3daProfileLevelIndication */
enum ProfileLevels : uint8_t {
  MAIN_LEVEL_1 = 0x01,
  MAIN_LEVEL_2 = 0x02,
  MAIN_LEVEL_3 = 0x03,
  MAIN_LEVEL_4 = 0x04,
  MAIN_LEVEL_5 = 0x05,
  HIGH_LEVEL_1 = 0x06,
  HIGH_LEVEL_2 = 0x07,
  HIGH_LEVEL_3 = 0x08,
  HIGH_LEVEL_4 = 0x09,
  HIGH_LEVEL_5 = 0x0A,
  LOW_COMPLEXITY_LEVEL_1 = 0x0B,
  LOW_COMPLEXITY_LEVEL_3 = 0x0D,
  LOW_COMPLEXITY_LEVEL_4 = 0x0E,
  LOW_COMPLEXITY_LEVEL_5 = 0x0F,
  BASELINE_LEVEL_1 = 0x10,
  BASELINE_LEVEL_2 = 0x11,
  BASELINE_LEVEL_3 = 0x12,
  BASELINE_LEVEL_4 = 0x13,
  BASELINE_LEVEL_5 = 0x14,
};

static std::string errorMessage(const std::vector<SB_VIOLATIONS>& violations) {
  std::string errors;
  for (size_t i = 0; i < violations.size(); ++i) {
    errors += std::to_string(static_cast<uint32_t>(violations[i]));
    if (violations.size() - 1 != i) {
      errors += ", ";
    }
  }

  return std::string("Error parsing config, bitstream is not baseline compatible: ") + errors;
}

struct SConfigurationInfo {
  std::vector<SB_VIOLATIONS> sbViolations;
  bool fulfillsLevel3BaseLevelRestrictions = true;
  SProfileLevel profileLevel;
  SProfileLevel compatibleProfileLevel;
};

enum class SignalGroupType : uint8_t { Channels = 0, Object = 1, SAOC = 2, HOA = 3 };

enum class UsacElementType : uint8_t { SCE = 0, CPE = 1, LFE = 2, EXT = 3 };

enum class HorizontalSpeakerDirection : uint8_t {
  FRONT_CENTER = 0,   // 0°
  BACK_CENTER = 180,  // 180°
};

static HorizontalSpeakerDirection copyMpegH3daSpeakerDescription(ilo::CBitParser& parser,
                                                                 ilo::CBitBuffer& writer,
                                                                 bool angularPrecision) {
  HorizontalSpeakerDirection position = HorizontalSpeakerDirection::FRONT_CENTER;

  uint8_t isCicpSpeaker = parser.read<uint8_t>(1);
  writer.write(isCicpSpeaker, 1);
  if (isCicpSpeaker) {
    auto cicpSpeaker = parser.read<uint8_t>(7);
    writer.write(cicpSpeaker, 7);
    switch (cicpSpeaker) {
      case 2:  // Front center
      case 3:  // LFE
        position = HorizontalSpeakerDirection::FRONT_CENTER;
        break;
      case 10:  // Back center
        position = HorizontalSpeakerDirection::BACK_CENTER;
        break;
      case 19:  // Top front center
        position = HorizontalSpeakerDirection::FRONT_CENTER;
        break;
      case 22:  // Top back center
        position = HorizontalSpeakerDirection::BACK_CENTER;
        break;
      case 25:  // Top center
      case 29:  // Bottom front center
        position = HorizontalSpeakerDirection::FRONT_CENTER;
        break;
      default:
        break;
    }
  } else {
    auto elevationClass = parser.read<uint8_t>(2);
    writer.write(elevationClass, 2);
    if (elevationClass == 3) {
      uint8_t elevationAngle = 0;
      if (angularPrecision) {
        elevationAngle = parser.read<uint8_t>(7);
        writer.write(elevationAngle, 7);
      } else {
        elevationAngle = parser.read<uint8_t>(5);
        writer.write(elevationAngle, 5);
      }

      if (elevationAngle != 0) {
        auto elevationDirection = parser.read<uint8_t>(1);
        writer.write(elevationDirection, 1);
      }
    }

    if (angularPrecision) {
      auto azimuthAngle = parser.read<uint8_t>(8);
      writer.write(azimuthAngle, 8);
      position = static_cast<HorizontalSpeakerDirection>(azimuthAngle);
    } else {
      auto azimuthAngle = parser.read<uint8_t>(6);
      writer.write(azimuthAngle, 6);

      // 5° per ULP
      position = static_cast<HorizontalSpeakerDirection>(azimuthAngle * 5);
    }

    if (position != HorizontalSpeakerDirection::FRONT_CENTER &&
        position != HorizontalSpeakerDirection::BACK_CENTER) {
      auto azimuthDirection = parser.read<uint8_t>(1);
      writer.write(azimuthDirection, 1);
    }

    auto isLfe = parser.read<uint8_t>(1);
    writer.write(isLfe, 1);
  }

  return position;
}

static void copyMpegH3daFlexibleSpeakerConfig(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                              uint32_t numSpeakers) {
  uint8_t angularPrecision = parser.read<uint8_t>(1);
  writer.write(angularPrecision, 1);

  for (uint32_t i = 0; i < numSpeakers; ++i) {
    auto horizontalDirection =
        copyMpegH3daSpeakerDescription(parser, writer, angularPrecision == 1U);
    if (horizontalDirection != HorizontalSpeakerDirection::FRONT_CENTER &&
        horizontalDirection != HorizontalSpeakerDirection::BACK_CENTER) {
      uint8_t addSymmetricPair = parser.read<uint8_t>(1);
      writer.write(addSymmetricPair, 1);
      if (addSymmetricPair) {
        i++;
      }
    }
  }
}

static void copySpeakerConfig3d(ilo::CBitParser& parser, ilo::CBitBuffer& writer) {
  auto speakerLayoutType = parser.read<uint8_t>(2);
  writer.write(speakerLayoutType, 2);
  if (speakerLayoutType == 0)  // single ChannelConfiguration index
  {
    auto cicpSpeakerLayout = parser.read<uint8_t>(6);
    writer.write(cicpSpeakerLayout, 6);
  } else {
    auto numSpeakersMinus1 =
        static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 5, 8, 16));
    mmt::mhasparserlib::writeEscapedValue(writer, numSpeakersMinus1, 5, 8, 16);
    auto numSpeakers = numSpeakersMinus1 + 1;
    if (speakerLayoutType == 1)  // list of LoudspeakerGeometry indices
    {
      for (uint32_t i = 0; i < numSpeakers; ++i) {
        auto cicpSpeaker = parser.read<uint8_t>(7);
        writer.write(cicpSpeaker, 7);
      }
    } else if (speakerLayoutType == 2)  // list of explicit geometric position information
    {
      copyMpegH3daFlexibleSpeakerConfig(parser, writer, numSpeakers);
    } else {
      ILO_FAIL("Unknown speakerLayoutType detected.");
    }
  }
}

static void pushViolation(std::vector<SB_VIOLATIONS>& vector, SB_VIOLATIONS violation) {
  if (std::find(vector.begin(), vector.end(), violation) == vector.end()) {
    vector.push_back(violation);
  }
}

static uint32_t copyFrameworkConfig3d(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                      SConfigurationInfo& info) {
  uint32_t numAudioChannels = 0;
  uint32_t numAudioObjects = 0;
  uint32_t numSAOCTransportChannels = 0;
  uint32_t numHOATransportChannels = 0;

  auto numSignalGroupsMinus1 = parser.read<uint8_t>(5);
  writer.write(numSignalGroupsMinus1, 5);
  auto numSignalGroups = numSignalGroupsMinus1 + 1u;
  for (uint32_t i = 0; i < numSignalGroups; ++i) {
    auto signalType = static_cast<SignalGroupType>(parser.read<uint8_t>(3));
    writer.write(static_cast<uint8_t>(signalType), 3);

    auto numberOfSignals =
        static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 5, 8, 16));
    mmt::mhasparserlib::writeEscapedValue(writer, numberOfSignals, 5, 8, 16);

    switch (signalType) {
      case SignalGroupType::Object:
        numAudioObjects += numberOfSignals + 1;
        if (numberOfSignals + 1 > 24) {
          info.fulfillsLevel3BaseLevelRestrictions = false;
        }
        break;
      case SignalGroupType::Channels: {
        numAudioChannels += numberOfSignals + 1;
        info.fulfillsLevel3BaseLevelRestrictions = false;
        uint8_t differsFromReferenceLayout = parser.read<uint8_t>(1);
        writer.write(differsFromReferenceLayout, 1);
        if (differsFromReferenceLayout) {
          copySpeakerConfig3d(parser, writer);
        }
        break;
      }
      case SignalGroupType::SAOC: {
        numSAOCTransportChannels += numberOfSignals + 1;
        info.fulfillsLevel3BaseLevelRestrictions = false;
        pushViolation(info.sbViolations, SB_VIOLATIONS::SIGNAL_TYPE_SAOC);
        uint8_t saocDmxLayoutPresent = parser.read<uint8_t>(1);
        writer.write(saocDmxLayoutPresent, 1);
        if (saocDmxLayoutPresent) {
          copySpeakerConfig3d(parser, writer);
        }
        break;
      }
      case SignalGroupType::HOA:
        numHOATransportChannels += numberOfSignals + 1;
        info.fulfillsLevel3BaseLevelRestrictions = false;
        pushViolation(info.sbViolations, SB_VIOLATIONS::SIGNAL_TYPE_HOA);
        break;
      default:
        ILO_FAIL("Unknown signalType detected.");
        break;
    }
  }

  return static_cast<uint32_t>(
             std::floor(std::log2((numHOATransportChannels + numSAOCTransportChannels +
                                   numAudioChannels + numAudioObjects - 1)))) +
         1;
}

static bool copyMpegH3daCoreConfig(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                   SConfigurationInfo& info) {
  uint8_t twMdct = parser.read<uint8_t>(1);
  writer.write(twMdct, 1);

  if (twMdct) {
    pushViolation(info.sbViolations, SB_VIOLATIONS::INVALID_TW_MDCT_VALUE);
  }

  uint8_t fullbandLpd = parser.read<uint8_t>(1);
  writer.write(fullbandLpd, 1);

  if (fullbandLpd) {
    pushViolation(info.sbViolations, SB_VIOLATIONS::INVALID_FULLBAND_LPD_VALUE);
  }

  uint8_t noiseFilling = parser.read<uint8_t>(1);
  writer.write(noiseFilling, 1);

  uint8_t enhancedNoiseFilling = parser.read<uint8_t>(1);
  writer.write(enhancedNoiseFilling, 1);

  if (enhancedNoiseFilling) {
    auto additionalBits = parser.read<uint32_t>(13);
    writer.write(additionalBits, 13);
  }
  return enhancedNoiseFilling == 1U;
}

static void copyMpegH3daSingleChannelElementConfig(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                                   SConfigurationInfo& info) {
  copyMpegH3daCoreConfig(parser, writer, info);
}

static void copyMpegh3daChannelPairElementConfig(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                                 uint32_t numBits, SConfigurationInfo& info) {
  bool enf = copyMpegH3daCoreConfig(parser, writer, info);
  if (enf) {
    uint8_t igfIndependentTiling = parser.read<uint8_t>(1);
    writer.write(igfIndependentTiling, 1);
  }

  auto qceIndex = parser.read<uint8_t>(2);
  writer.write(qceIndex, 2);
  if (qceIndex != 0) {
    pushViolation(info.sbViolations, SB_VIOLATIONS::INVALID_QCE_INDEX);
    ILO_FAIL(errorMessage(info.sbViolations).c_str());
  }

  uint8_t shiftIndex1 = parser.read<uint8_t>(1);
  writer.write(shiftIndex1, 1);
  if (shiftIndex1) {
    auto shiftChannel1 = parser.read<uint32_t>(numBits);
    writer.write(shiftChannel1, numBits);
  }

  uint8_t lpdStereoEnabled = parser.read<uint8_t>(1);
  writer.write(lpdStereoEnabled, 1);
}

static void copyMpegh3daExtElementConfig(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                         bool isFirstFrame, SConfigurationInfo& info) {
  uint32_t usacExtElementType =
      static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));
  mmt::mhasparserlib::writeEscapedValue(writer, usacExtElementType, 4, 8, 16);
  if (!isFirstFrame) {
    if (usacExtElementType == ID_EXT_ELE_AUDIOPREROLL) {
      ILO_LOG_WARNING("ID_EXT_ELE_AUDIOPREROLL is not the first ExtElementConfig.");
    }
  }

  auto usacExtElementConfigLength =
      static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));
  mmt::mhasparserlib::writeEscapedValue(writer, usacExtElementConfigLength, 4, 8, 16);

  uint8_t usacExtElementDefaultLengthPresent = parser.read<uint8_t>(1);
  writer.write(usacExtElementDefaultLengthPresent, 1);
  if (usacExtElementDefaultLengthPresent) {
    auto usacExtElementDefaultLength =
        static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 8, 16, 0));
    mmt::mhasparserlib::writeEscapedValue(writer, usacExtElementDefaultLength, 8, 16, 0);
  }

  auto usacExtElementPayloadFrag = parser.read<uint8_t>(1);
  writer.write(usacExtElementPayloadFrag, 1);

  for (uint32_t i = 0; i < usacExtElementConfigLength; ++i) {
    auto byte = parser.read<uint8_t>(8);
    writer.write(byte, 8);
  }
}

static void copyMpegH3daDecoderConfig(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                      uint32_t numBits, SConfigurationInfo& info) {
  uint32_t numElementsMinus1 =
      static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));
  mmt::mhasparserlib::writeEscapedValue(writer, numElementsMinus1, 4, 8, 16);

  auto numElements = numElementsMinus1 + 1;

  uint8_t elementLengthPresent = parser.read<uint8_t>(1);
  writer.write(elementLengthPresent, 1);

  for (uint32_t i = 0; i < numElements; ++i) {
    auto usacElementType = static_cast<UsacElementType>(parser.read<uint8_t>(2));
    writer.write(static_cast<uint8_t>(usacElementType), 2);

    switch (usacElementType) {
      case UsacElementType::SCE:
        copyMpegH3daSingleChannelElementConfig(parser, writer, info);
        break;
      case UsacElementType::CPE:
        copyMpegh3daChannelPairElementConfig(parser, writer, numBits, info);
        break;
      case UsacElementType::LFE:
        // There is nothing to copy for mpegh3daLfeElementConfig
        break;
      case UsacElementType::EXT:
        copyMpegh3daExtElementConfig(parser, writer, (i == 0), info);
        break;
    }
  }
}

static void copyUntilConfigExtension(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                     SConfigurationInfo& info) {
  auto mpegh3daProfileLevelIndication = parser.read<uint8_t>(8);
  info.profileLevel.set(mpegh3daProfileLevelIndication);
  writer.write(mpegh3daProfileLevelIndication, 8);

  auto usacSamplingFrequencyIndex = parser.read<uint8_t>(5);
  writer.write(usacSamplingFrequencyIndex, 5);
  if (usacSamplingFrequencyIndex == 0x1fu) {
    auto usacSamplingFrequency = parser.read<uint32_t>(24);
    writer.write(usacSamplingFrequency, 24);
  }

  auto coreSbrFrameLengthIndex = parser.read<uint8_t>(3);
  writer.write(coreSbrFrameLengthIndex, 3);

  ILO_ASSERT(coreSbrFrameLengthIndex < 2, "Invalid LC config found.");

  auto flags = parser.read<uint8_t>(2);
  writer.write(flags, 2);

  copySpeakerConfig3d(parser, writer);
  uint32_t numBits = copyFrameworkConfig3d(parser, writer, info);
  copyMpegH3daDecoderConfig(parser, writer, numBits, info);
}

static void copyCompatibleProfileLevelSetMpegh3daConfigExtension(ilo::CBitParser& parser,
                                                                 ilo::CBitBuffer& writer,
                                                                 SConfigurationInfo& info) {
  mmt::mhasparserlib::writeEscapedValue(writer, ID_CONFIG_EXT_COMPATIBLE_PROFILELVL_SET, 4, 8, 16);

  auto configExtLength =
      static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));
  mmt::mhasparserlib::writeEscapedValue(writer, configExtLength, 4, 8, 16);

  // copy CompatibleSetIndications
  for (uint32_t k = 0; k < configExtLength; k++) {
    auto value = parser.read<uint8_t>(8);
    writer.write(value, 8);

    if (k == configExtLength - 1) {
      // store last CompatibleSetIndication in configuration info
      ILO_LOG_INFO(
          "extractASIFromConfigExtensionAndAddCompatibleProfileLevelSet - CompatibleProfileLevel "
          "%u",
          value);
      info.compatibleProfileLevel.set(value);
    }
  }
}

static void copyGenericMpegh3daConfigExtension(ilo::CBitParser& parser, ilo::CBitBuffer& writer,
                                               uint32_t configExtType) {
  mmt::mhasparserlib::writeEscapedValue(writer, configExtType, 4, 8, 16);

  auto configExtLength =
      static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));
  mmt::mhasparserlib::writeEscapedValue(writer, configExtLength, 4, 8, 16);

  // copy extension body
  for (uint32_t k = 0; k < configExtLength; k++) {
    auto extensionByte = parser.read<uint8_t>(8);
    writer.write(extensionByte, 8);
  }
}

static void writeCompatibleProfileLevelSetToConfig(ilo::CBitBuffer& writer,
                                                   SConfigurationInfo& info) {
  ILO_ASSERT(info.profileLevel.get() >= ProfileLevels::LOW_COMPLEXITY_LEVEL_1 &&
                 info.profileLevel.get() <= ProfileLevels::LOW_COMPLEXITY_LEVEL_5,
             "Only LC bitstreams are supported, found profile level: %d",
             static_cast<int>(info.profileLevel.get()));
  mmt::mhasparserlib::writeEscapedValue(writer, 7, 4, 8, 16);  // usacConfigExtType = 7
  mmt::mhasparserlib::writeEscapedValue(writer, 2, 4, 8, 16);  // usacConfigExtLength = 2
  writer.write(0U, 4);  // bsNumCompatibleSets (num compatible profile sets - 1)
  writer.write(0U, 4);  // reserved
  uint8_t compatibleSetIndication = 0;
  if (info.profileLevel.get() == ProfileLevels::LOW_COMPLEXITY_LEVEL_4 &&
      info.fulfillsLevel3BaseLevelRestrictions) {
    compatibleSetIndication = ProfileLevels::BASELINE_LEVEL_3;
  } else {
    // The value for "Baseline Level X" is exactly 5 larger than the value for "Low Complexity Level
    // X"
    compatibleSetIndication = info.profileLevel.get() + 5;
  }
  info.compatibleProfileLevel.set(compatibleSetIndication);

  ILO_LOG_INFO("writeCompatibleProfileLevelSetToConfig - CompatibleProfileLevel %u",
               static_cast<unsigned>(compatibleSetIndication));
  writer.write(compatibleSetIndication, 8);
}

static ilo::ByteBuffer extractASIFromConfigExtensionAndAddCompatibleProfileLevelSet(
    ilo::CBitParser& parser, ilo::CBitBuffer& writer, SConfigurationInfo& info) {
  ilo::ByteBuffer returnValue;

  bool hasExtensions = parser.read<uint8_t>(1) == 1;

  uint32_t numConfigExtensions = 0;
  if (hasExtensions) {
    numConfigExtensions =
        static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 2, 4, 8) + 1);
  }

  ilo::CBitBuffer configExtensionsTempWriter;
  uint32_t numConfigExtensionsCopied = 0;
  bool compatibleProfileLevelSetFound = false;

  for (uint32_t i = 0; i < numConfigExtensions; i++) {
    uint32_t configExtType =
        static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));

    if (configExtType == ID_CONFIG_EXT_AUDIOSCENE_INFO) {
      // Copy the ASI
      auto length = static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 8, 16));
      returnValue.resize(length);

      std::generate_n(returnValue.begin(), length, [&parser] { return parser.read<uint8_t>(8); });
    } else if (configExtType == ID_CONFIG_EXT_COMPATIBLE_PROFILELVL_SET) {
      ILO_LOG_INFO("Found ID_CONFIG_EXT_COMPATIBLE_PROFILELVL_SET, will not be overwritten.");
      compatibleProfileLevelSetFound = true;
      copyCompatibleProfileLevelSetMpegh3daConfigExtension(parser, configExtensionsTempWriter,
                                                           info);
      numConfigExtensionsCopied++;
    } else {
      copyGenericMpegh3daConfigExtension(parser, configExtensionsTempWriter, configExtType);
      numConfigExtensionsCopied++;
    }
  }

  if (!compatibleProfileLevelSetFound &&
      info.profileLevel.get() < ProfileLevels::BASELINE_LEVEL_1) {
    writeCompatibleProfileLevelSetToConfig(configExtensionsTempWriter, info);
    numConfigExtensionsCopied++;
  } else {
    ILO_LOG_WARNING("Skipping CompatibleSetIndication (extension already present)");
  }

  if (numConfigExtensionsCopied != 0) {
    writer.write(1u, 1);
    mmt::mhasparserlib::writeEscapedValue(writer, numConfigExtensionsCopied - 1, 2, 4, 8);

    auto configExtensionsBuffer = configExtensionsTempWriter.bytebuffer();

    ilo::CBitParser configExtensionsTempReader(configExtensionsBuffer,
                                               configExtensionsTempWriter.nofBits());

    while (configExtensionsTempReader.nofBits() - configExtensionsTempReader.tell() >= 8) {
      writer.write(configExtensionsTempReader.read<uint8_t>(8), 8);
    }

    auto numBitsLeft = configExtensionsTempReader.nofBits() - configExtensionsTempReader.tell();
    if (numBitsLeft != 0) {
      writer.write(configExtensionsTempReader.read<uint8_t>(numBitsLeft), numBitsLeft);
    }
  } else {
    writer.write(0u, 1);
  }

  writer.byteAlign();
  return returnValue;
}

uint8_t SProfileLevel::get() const noexcept {
  if (!m_profileLevelSet) {
    ILO_LOG_WARNING(
        "Retrieving Profile Level that was not set, perhaps no decoder config record was found in "
        "the input file, returning %d as a sensible default",
        static_cast<int>(ProfileLevels::LOW_COMPLEXITY_LEVEL_3));
    return ProfileLevels::LOW_COMPLEXITY_LEVEL_3;
  }
  return m_profileLevel;
}

CConverter::CConverter(const CConverter::SConverterConfiguration& config)
    : m_currentPacketLabel(config.initialPacketLabel), m_config(config) {
  ILO_ASSERT(m_currentPacketLabel <= MAX_PACKET_LABEL_MAIN_STREAM,
             "Provided packet label is too big.");
  ILO_ASSERT(m_currentPacketLabel != 0, "Provided packet label is zero.");
}

SMhasConfigOutput CConverter::convertConfig(const ilo::ByteBuffer& mpegh3daConfig) {
  SConfigurationInfo info;
  ilo::CBitParser configParser(mpegh3daConfig);
  ilo::CBitBuffer configWriter;
  copyUntilConfigExtension(configParser, configWriter, info);

  auto asi = extractASIFromConfigExtensionAndAddCompatibleProfileLevelSet(configParser,
                                                                          configWriter, info);
  bool hasAsi = !asi.empty();

  ilo::ByteBuffer convertedConfig = configWriter.bytebuffer();

  ilo::ByteBuffer asiBuffer;
  mmt::mhasparserlib::CMhasConfigPacket configPacket(m_currentPacketLabel, convertedConfig.begin(),
                                                     convertedConfig.end());
  if (hasAsi) {
    auto asiBegin = asi.cbegin();
    auto asiEnd = asi.cend();
    mmt::mhasparserlib::CMhasAsiPacket asiPacket(m_currentPacketLabel, asiBegin, asiEnd);
    asiBuffer.resize(asiPacket.calculatePacketSize());
    asiPacket.writePacket(asiBuffer);
  }

  ILO_ASSERT(info.sbViolations.empty(), errorMessage(info.sbViolations).c_str());

  ilo::ByteBuffer configBuffer(configPacket.calculatePacketSize());
  configPacket.writePacket(configBuffer);

  if (m_currentConfig &&
      (*m_currentConfig != configBuffer || (m_currentAsi && !hasAsi) || (!m_currentAsi && hasAsi) ||
       (m_currentAsi && hasAsi && *m_currentAsi != asiBuffer))) {
    ++m_currentPacketLabel;
    m_currentPacketLabel %= (MAX_PACKET_LABEL_MAIN_STREAM + 1);
    if (m_currentPacketLabel == 0) {
      m_currentPacketLabel = 1;
    }

    configPacket = mmt::mhasparserlib::CMhasConfigPacket(
        m_currentPacketLabel, convertedConfig.begin(), convertedConfig.end());
    configBuffer.resize(configPacket.calculatePacketSize());
    configPacket.writePacket(configBuffer);

    if (hasAsi) {
      auto asiBegin = asi.cbegin();
      auto asiEnd = asi.cend();
      mmt::mhasparserlib::CMhasAsiPacket asiPacket(m_currentPacketLabel, asiBegin, asiEnd);
      asiBuffer.resize(asiPacket.calculatePacketSize());
      asiPacket.writePacket(asiBuffer);
      m_currentAsi = ilo::make_unique<ilo::ByteBuffer>(asiBuffer);
    } else {
      m_currentAsi = nullptr;
    }

    m_currentConfig = ilo::make_unique<ilo::ByteBuffer>(configBuffer);
  } else {
    m_currentConfig = ilo::make_unique<ilo::ByteBuffer>(configBuffer);
    if (hasAsi) {
      m_currentAsi = ilo::make_unique<ilo::ByteBuffer>(asiBuffer);
    }
  }

  SMhasConfigOutput out;
  out.fullMpegHConfigBlob = convertedConfig;
  out.config = std::move(configBuffer);
  out.compatibleProfileLevel.set(info.compatibleProfileLevel.get());
  if (hasAsi) {
    out.asi = ilo::make_unique<ilo::ByteBuffer>(asiBuffer);
  }
  return out;
}

static bool isIpf(const ilo::ByteBuffer& mpegh3daFrame) {
  ILO_ASSERT(!mpegh3daFrame.empty(), "Frame does not contain any payload");
  return (mpegh3daFrame[0] & 0xE0u) == 0xC0u;
}

static bool isIFrame(const ilo::ByteBuffer& mpegh3daFrame) {
  ILO_ASSERT(!mpegh3daFrame.empty(), "Frame does not contain any payload");
  return (mpegh3daFrame[0] & 0x80u) == 0x80u;
}

static SMhasFrameOutput convertFrameInternal(const ilo::ByteBuffer& mpegh3daFrame, bool isIPF,
                                             uint32_t currentPacketLabel) {
  auto begin = mpegh3daFrame.cbegin();
  mmt::mhasparserlib::CMhasFramePacket packet(currentPacketLabel, begin, mpegh3daFrame.end(),
                                              isIPF);

  SMhasFrameOutput output;
  output.frame.resize(packet.calculatePacketSize());
  packet.writePacket(output.frame);

  return output;
}

SMhasFrameOutput CConverter::convertFrame(const ilo::ByteBuffer& mpegh3daFrame) {
  bool inputIpf = isIpf(mpegh3daFrame);
  auto out = inputIpf ? convertIPF(mpegh3daFrame)
                      : convertFrameInternal(mpegh3daFrame, false, m_currentPacketLabel);
  out.isIpf = inputIpf;
  out.isIndepFrame = inputIpf || isIFrame(mpegh3daFrame);
  ++m_currentFrameNumber;
  return out;
}

static uint32_t readExtElementPayloadLength(ilo::CBitParser& parser) {
  auto value = parser.read<uint32_t>(8);
  if (value == 255) {
    auto tmpValue = parser.read<uint16_t>(16);
    value += tmpValue - 2;
  }
  return value;
}

static void writeExtElementPayloadLength(ilo::CBitBuffer& buffer, uint32_t value) {
  if (value > 254) {
    buffer.write(255u, 8);
    value -= 253;
    buffer.write(value, 16);
    return;
  }

  buffer.write(value, 8);
}

uint32_t CConverter::currentPacketLabel() const {
  return m_currentPacketLabel;
}

SMhasFrameOutput CConverter::convertIPF(const ilo::ByteBuffer& mpegh3daFrame) {
  ilo::CBitParser parser(mpegh3daFrame);
  ilo::CBitBuffer writer;

  uint8_t value = parser.read<uint8_t>(3);
  ILO_ASSERT(value == 6, "Frame does not contain any AudioPreRoll.");
  writer.write(value, 3);

  uint32_t extensionPayloadLength = readExtElementPayloadLength(parser);
  uint32_t positionBegin = parser.tell();

  ilo::CBitBuffer temp;
  SMhasConfigOutput mhasConfig;
  uint32_t configLength =
      static_cast<uint32_t>(mmt::mhasparserlib::readEscapedValue(parser, 4, 4, 8));
  if (configLength != 0) {
    ilo::ByteBuffer config(configLength);

    for (uint32_t i = 0; i < configLength; ++i) {
      auto mpegh3daConfigByte = parser.read<uint8_t>(8);
      config[i] = mpegh3daConfigByte;
    }
    mhasConfig = convertConfig(config);
  } else {
    ILO_ASSERT(m_currentConfig, "No AudioPreRoll config found and no config available.");
    if (m_currentAsi) {
      mhasConfig.asi = ilo::make_unique<ilo::ByteBuffer>(*m_currentAsi);
    }
    mhasConfig.config = *m_currentConfig;
  }

  // write config length of zero
  mmt::mhasparserlib::writeEscapedValue(temp, 0, 4, 4, 8);

  uint8_t applyCrossfade = parser.read<uint8_t>(1);
  temp.write(applyCrossfade, 1);
  auto reserved = parser.read<uint8_t>(1);
  temp.write(reserved, 1);

  auto numPrerollFrames = mmt::mhasparserlib::readEscapedValue(parser, 2, 4, 0);
  mmt::mhasparserlib::writeEscapedValue(temp, numPrerollFrames, 2, 4, 0);

  ILO_LOG_INFO("Sample %" PRIu64 " is an IPF. ", m_currentFrameNumber);
  ILO_LOG_INFO("numPreRollFrames %" PRIu64 ", ", numPrerollFrames);
  ILO_LOG_INFO("applyCrossfade %u", applyCrossfade);

  if (!applyCrossfade || numPrerollFrames == 0) {
    ILO_LOG_WARNING("This can lead to audible artifacts during bitrate adaptation.");
  }

  if (numPrerollFrames > 1) {
    ILO_LOG_WARNING("numPreRollFrames is: %u. Maximal one pre-roll frame is allowed.",
                    numPrerollFrames);
  }

  for (uint64_t i = 0; i < numPrerollFrames; ++i) {
    auto auLength = mmt::mhasparserlib::readEscapedValue(parser, 16, 16, 0);
    mmt::mhasparserlib::writeEscapedValue(temp, auLength, 16, 16, 0);
    for (uint64_t k = 0; k < auLength; ++k) {
      auto frameByte = parser.read<uint8_t>(8);
      if (i == 0 && k == 0) {
        if ((frameByte & 0x80u) != 0x80u) {
          ILO_LOG_WARNING(
              "Pre-roll frame is not independently decodable. If bitrate adaption is used, this "
              "can lead to audible artifacts.");
        }
      }
      temp.write(frameByte, 8);
    }
  }

  ILO_ASSERT(parser.tell() <= positionBegin + extensionPayloadLength * 8,
             "Invalid extension segment payload length detected.");
  parser.seek(static_cast<int32_t>(positionBegin + extensionPayloadLength * 8u),
              ilo::EPosType::begin);

  temp.byteAlign();
  ilo::ByteBuffer tempBuffer = temp.bytebuffer();
  ilo::CBitParser tempParser(tempBuffer);

  writeExtElementPayloadLength(writer, temp.nofBytes());

  while (!tempParser.eof()) {
    writer.write(tempParser.read<uint8_t>(8), 8);
  }

  while (parser.nofBits() - parser.tell() >= 8) {
    writer.write(parser.read<uint8_t>(8), 8);
  }

  if (!parser.eof()) {
    auto bitsToRead = parser.nofBits() - parser.tell();
    writer.write(parser.read<uint8_t>(bitsToRead), bitsToRead);
  }
  writer.byteAlign();

  auto byteBuffer = writer.bytebuffer();
  ilo::ByteBuffer frame(byteBuffer.begin(), byteBuffer.end());
  auto returnValue = convertFrameInternal(frame, true, m_currentPacketLabel);
  returnValue.config = ilo::make_unique<ilo::ByteBuffer>(mhasConfig.config);
  returnValue.asi.swap(mhasConfig.asi);

  return returnValue;
}

void logging::redirectToConsole() {
  LOG_REDIRECT_TO_CONSOLE();
}

void logging::redirectToSyslog() {
  LOG_REDIRECT_TO_SYSTEM_LOG();
}

void logging::redirectToFile(const std::string& filePath, bool appendFile) {
  if (appendFile) {
    LOG_REDIRECT_TO_FILE_APPEND(filePath);
  } else {
    LOG_REDIRECT_TO_FILE(filePath);
  }
}

void logging::disable() {
  LOG_DISABLE_LOGGING();
}
