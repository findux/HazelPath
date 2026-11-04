# ofxGpr

OpenFrameworks addon for GPR, a C++11 G-code parser. Parses G-code into structured blocks of comments and word-address commands.

## Supported G-code Dialects

- RepRap (3D printers, Cura output)
- LinuxCNC
- Vectric (VCarve Pro)
- HAAS
- Mazak

## Installation

Clone or copy this folder into your OpenFrameworks `addons/` directory:

```
openframeworks/addons/ofxGpr/
```

## Usage

```cpp
#include "ofxGpr.h"

// Parse a G-code string
string gcodeText = ofBufferFromFile("myfile.gcode").getText();
gpr::gcode_program program = gpr::parse_gcode(gcodeText);

// Iterate over blocks (lines)
for (auto& block : program) {
    for (auto& chunk : block) {
        if (chunk.tp() == gpr::CHUNK_TYPE_WORD_ADDRESS) {
            char word = chunk.get_word();       // e.g. 'G', 'X', 'Y', 'Z'
            gpr::addr address = chunk.get_address();
        }
    }
}
```

## Example

`example-basic` loads a G-code file and visualizes the toolpaths in 3D using ofEasyCam.

## License

SPDX-License-Identifier: MIT

GPR: Copyright (c) 2017 Dillon Huff.
OpenFrameworks adaptation and original addon contributions: Copyright (c) 2026 findux and HazelPath contributors.

Both are distributed under the MIT license. See LICENSE for the full terms.
The upstream copyright and permission notice must be retained.
