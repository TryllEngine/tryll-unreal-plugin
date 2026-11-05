// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: VERSION  +  tools/codegen/templates/unreal_docs_base.h.j2
// See: docs/dev/codegen.md
#pragma once

/**
 * Docs site root pinned to the repo VERSION's /vX.Y/ folder. The site serves no unversioned
 * paths, and the in-tree plugin reports 0.0.0 until a release stamps it, so the pin is baked
 * in here for TryllDocs.h and the node help links. A wide literal, so it concatenates with
 * TEXT("path"): `TRYLL_DOCS_BASE_URL TEXT("how-to/page.html")`.
 */
#define TRYLL_DOCS_BASE_URL TEXT("https://docs.tryllengine.com/v0.10/")
