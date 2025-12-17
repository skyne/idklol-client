# TurboLink

## Setup

- Has to live under `Plugins/TurboLink` (TurboLink.uplugin)
- Add `"TurboLinkGrpc"` to the project's `Build.cs`

### Example Build.cs Configuration

```cpp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "TurboLinkGrpc"
});
```

## Code Generation

- Under tools, run: `generate_code.cmd <proto_file> <output_path>`
  - `output_path` has to exist
  - `proto_file` preferred to be copied to the same folder for generation
- Result of the generation has to be copied to the `Plugins/TurboLink/Source/TurboLinkGrpc` folder
  - **Important**: Never copy the whole `Private` and `Public` folders, just their content
  - Also do not remove anything from the `Private` and `Public` folders under `Plugins/TurboLink/Source/TurboLinkGrpc`

## Code Guidelines

### Import Rules

- **Never use relative imports:**

```cpp
// ✅ Correct
#include "TurboLinkGrpcUtilities.h"

// ❌ Incorrect
#include "../../Plugins/.../TurboLinkGrpcUtilities.h"
```

### Only Import What You Need

```cpp
#include "TurboLinkGrpcUtilities.h"
#include "TurboLinkGrpcManager.h"
#include "AuthService-generated.h"
#include "AuthService.h"
// + REQUIRED
// (Only if needed - UE will auto-include normally)
```

## Project Files

- Regenerate project files:
  - `GenerateProjectFiles.bat -vscode`
  - Or on Windows with Epic Games launcher: Find the `<project>.uproject` file > Right click > Show more options > Generate Visual Studio project files

## Troubleshooting

### Clean Up After EXCEPTION_ACCESS_VIOLATION

When encountering `EXCEPTION_ACCESS_VIOLATION` with gRPC usage:

1. Delete `Binaries`, `Intermediate`, `Saved`
2. This usually happens when the generated service is being created in the actor constructor, which is illegal
3. Move the usage somewhere else, for example:
