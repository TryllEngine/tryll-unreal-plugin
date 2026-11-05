// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

/// Null-safe wrappers for the Tracy calls that are *not* null-safe.
///
/// The build sets TRACY_MANUAL_LIFETIME (see the Tracy block in the top-level
/// CMakeLists.txt), so the profiler global is constructed only when something
/// calls tracy::StartupProfiler().  The server does, in Main.cpp; the test
/// binary deliberately does not, to avoid paying ~300 ms of CalibrateTimer()
/// per CTest process.
///
/// Under that mode Tracy's entry points divide in two:
///
///   safe    ZoneNamed(v, TracyIsStarted) — `TracyIsStarted` is the zone's
///           `active` flag, and ScopedZone/ZoneText early-return when inactive.
///   UNSAFE  TracyPlot / TracyMessageL — these expand straight to
///           Profiler::PlotData / Profiler::Message, which reach
///           GetProfilerData() and trip `assert(s_profilerData)`.
///
/// Nothing warns you about the second group: it compiles, and it only aborts
/// at runtime in a Tracy-enabled build that never started the profiler.  That
/// trap already caught three files (td-063), so the guard lives here rather
/// than being hand-written at each call site.
///
/// Use TRYLL_TRACY_PLOT / TRYLL_TRACY_MSG instead of the bare macros.  Zones
/// need no wrapper — keep passing `TracyIsStarted` as their `active` argument.

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>

#define TRYLL_TRACY_PLOT(name, val) do { if (TracyIsStarted) TracyPlot(name, val); } while (0)
#define TRYLL_TRACY_MSG(txt)        do { if (TracyIsStarted) TracyMessageL(txt); } while (0)

#else

// Matches the bare macros' behaviour in a non-Tracy build: the arguments are
// not evaluated, and not compiled.
#define TRYLL_TRACY_PLOT(name, val) do { } while (0)
#define TRYLL_TRACY_MSG(txt)        do { } while (0)

#endif
