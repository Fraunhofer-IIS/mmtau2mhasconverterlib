# Main Page {#mainpage}

## Description

The MHA to MHM converter library contains routines to convert MPEG-H 3DA audio data from MHA to MHM encapsulation formats.
The library provides interfaces operating on byte buffers ([CConverter](@ref mmt::au2mhasconverterlib::CConverter)), input files ([CFileConverter](@ref mmt::au2mhasconverterlib::CFileConverter)) as well as directories ([CDirectoryConverter](@ref mmt::au2mhasconverterlib::CDirectoryConverter)).

## Buffer Converter

The [CConverter](@ref mmt::au2mhasconverterlib::CConverter) consumes raw byte buffers containing MHA-encapsulated MPEG-H 3D Audio packets and converts them to MHM packets.

### Minimal Code Example

The following lines provide the minimal steps to convert byte-buffers of MPEG-H 3D Audio.

```{.c}
#include "mmtau2mhasconverterlib/converter.h"

using namespace mmt;
using namespace mmt::au2mhasconverterlib;

...

std::vector<uint8_t> mpegh3daConfigPacket = ...;
std::vector<uint8_t> mpegh3daFramePacket = ...;

CConverter::SConverterConfiguration config{};
CConverter converter{config};

SMhasConfigOutput outputConfigPacket = converter.convertConfig(mpegh3daConfigPacket);
SMhasFrameOutput outputFramePacket = converter.convertFrame(mpegh3daFramePacket);

// ... do something with the converter packets
```

## File Converter

The [CFileConverter](@ref mmt::au2mhasconverterlib::CFileConverter) is a convenience wrapper directly reading a full MHA-encapsulated MPEG-H 3D Audio file from the local file system and converting it to a MHM-encapsulated MPEG-H 3D Audio file.

### Minimal Code Example

The following lines provide the minimal steps to convert a single file of MPEG-H 3D Audio.

```{.c}
#include "mmtau2mhasconverterlib/file_converter.h"

using namespace mmt;
using namespace mmt::au2mhasconverterlib;

...

CFileConverter::SConfig config{};
config.inputFile = "path/to/mha/input/file";
config.outputFile = "path/to/mhm/output/file";
CFileConverter converter{config};

converter.process();

```

## Directory Converter

The [CDirectoryConverter](@ref mmt::au2mhasconverterlib::CDirectoryConverter) is a convenience wrapper to quickly convert a set of MHA-encapsulated MPEG-H 3D Audio files to MHM.

### Minimal Code Example

The following lines provide the minimal steps to convert all MHA files in a single directory.

```{.c}
#include "mmtau2mhasconverterlib/directory_converter.h"

using namespace mmt;
using namespace mmt::au2mhasconverterlib;

...

CDirectoryConverter::SConfig config{};
config.inputDirectoryPath = "path/to/mha/input/folder";
config.outputDirectoryPath = "path/to/mhm/input/folder";

CDirectoryConverter converter{config};

converter.process();

```
