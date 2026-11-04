# Licensing

Copyright (C) 2026 findux and HazelPath contributors.

The original HazelPath source code, scripts, and project assets are licensed
under the GNU General Public License, version 3 or (at your option) any later
version (`GPL-3.0-or-later`). The full license is in [LICENSE](../LICENSE).
HazelPath is provided without warranty, as described in that license.

## Third-party components

### GPR / ofxGpr

HazelPath uses GPR by Dillon Huff through our openFrameworks addon, `ofxGpr`.
The original GPR code is Copyright (c) 2017 Dillon Huff and is MIT licensed.
The openFrameworks adaptation and original addon contributions by findux and
HazelPath contributors are also MIT licensed, Copyright (c) 2026.
This addon is a separate MIT-licensed component; HazelPath's application code
remains GPL-3.0-or-later.

The MIT permission and copyright notice must accompany copies or substantial
portions of the component, including binary distributions. A copy is provided
in [licenses/ofxGpr-MIT.txt](../licenses/ofxGpr-MIT.txt) and included by the
installer. Upstream GPR: https://github.com/dillonhuff/gpr

### Other dependencies

Third-party components retain their own copyright notices and licenses; the
HazelPath license does not relicense them. The build uses openFrameworks and
the addons listed in `addons.make`, including their transitive dependencies.
Consult their respective source distributions for license texts and notices.
Pinned addon sources and their existing notices are included in `vendor/addons`.
The automated release build collects dependency license documents into the
installer's `licenses` directory and includes the addon sources in its source
archive. See [automated releases](releases.md).

The Microsoft Visual C++ runtime DLLs in `installer/runtime/` are Microsoft
components, distributed under Microsoft's applicable redistribution terms;
they are not covered by HazelPath's GPL license.

When distributing binaries, include the applicable third-party license notices
and provide the corresponding source required by the GPL. Existing installers
in `dist/` predate this licensing update and have not been rebuilt to include
the license text.

Source repository: https://github.com/findux/HazelPath
