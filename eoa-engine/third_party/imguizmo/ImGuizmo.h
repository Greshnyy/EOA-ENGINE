#pragma once

// Compatibility header for the editor integration.
// The current editor does not call ImGuizmo directly yet, while CMake keeps
// the vendor source directory in the include path. Keeping this header in
// place makes the dependency explicit and allows the real ImGuizmo API to be
// dropped in later without changing editor includes.
