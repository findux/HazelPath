# Automated Windows releases

Pushes to `main` and **Actions → Windows release → Run workflow** run tests,
build Windows x64, and upload packages as a workflow artifact. They do not
create a release. A `vMAJOR.MINOR.PATCH` tag runs the same checks and then creates
a **draft** GitHub Release with the installer, source archive, build metadata,
dependency lock, SHA-256 checksums and generated release notes.

## Make a release

1. Update `HAZELPATH_VERSION` in `src/AppVersion.h`, commit and push to `main`.
2. Wait for the Windows release validation to pass.
3. Tag that exact commit and push the tag, for example:

   ```powershell
   git tag -a v1.0.6 -m "HazelPath 1.0.6" <validated-commit>
   git push origin v1.0.6
   ```

4. Open the draft under GitHub Releases, review the notes, install and smoke-test
   the unsigned x64 installer, then publish the draft when ready.

Tags must exactly match `AppVersion.h`; invalid tags fail before downloading
dependencies. Tests and compilation failures prevent draft creation. No signing
certificate or additional repository secret is required. GitHub's automatic
token has write permission only in the draft release job.

For a failed run, use **Re-run failed jobs**. If a draft already exists, inspect
it before retrying; the workflow refuses to overwrite an existing release.
Do not move published release tags. Use a new patch version for corrections.

## Reproduce the build

Use Windows, Visual Studio 2022 C++ tools (v143 and Windows SDK), 7-Zip, and
NSIS 3.11. Download the SDK URL from `scripts/release-dependencies.json`, then:

```powershell
./scripts/prepare-release.ps1 -SdkArchive C:/build/of-sdk.zip -Destination C:/build/fresh-sdk
# Run the script from the application path printed by prepare-release:
& C:/build/fresh-sdk/of_v0.12.1_vs_64_release/apps/myApps/HazelPath/scripts/build-release.ps1 -Tag v1.0.5 -OutputDirectory C:/build/fresh-output
```

The preparation script verifies the SDK SHA-256 and copies the pinned addon
sources from `vendor/addons`. No local addon installation is used. The SDK
release is fixed to 0.12.1; addon source versions are pinned by the project Git
commit. A fresh output directory prevents accidentally packaging old binaries.
The source archive contains those addons and the build scripts; the SDK archive
is available at the locked official URL. Compiler/Windows runner updates may
change binary hashes; this pipeline does not promise bit-for-bit builds.

NSIS includes the application GPL and collected dependency notices. Older
installers tracked in `dist/` are never used by the workflow.
