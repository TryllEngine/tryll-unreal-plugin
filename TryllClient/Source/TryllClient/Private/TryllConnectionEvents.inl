// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// One row per connection broadcast. Expands into the events struct, and into
// the bind and unbind loops of both subsystems. Forward method bodies stay in
// the subsystems: three of them are not a plain re-broadcast.

#ifndef TRYLL_CONNECTION_EVENT
#error Define TRYLL_CONNECTION_EVENT(Member, DelegateType, ForwardMethod) before including this file.
#endif

TRYLL_CONNECTION_EVENT(OnConnectionChanged, FTryllOnConnectionChangedNative, ForwardConnectionChanged)
TRYLL_CONNECTION_EVENT(OnError, FTryllOnErrorNative, ForwardError)
TRYLL_CONNECTION_EVENT(OnAgentDestroyed, FTryllOnAgentDestroyedNative, ForwardAgentDestroyed)
TRYLL_CONNECTION_EVENT(OnCreateSessionComplete, FTryllOnCreateSessionCompleteNative, ForwardCreateSession)
TRYLL_CONNECTION_EVENT(OnSetSessionLocaleComplete, FTryllOnSetSessionLocaleCompleteNative, ForwardSetSessionLocale)
TRYLL_CONNECTION_EVENT(OnListModelsComplete, FTryllOnListModelsCompleteNative, ForwardListModels)
TRYLL_CONNECTION_EVENT(OnDownloadProgress, FTryllOnDownloadProgressNative, ForwardDownloadProgress)
TRYLL_CONNECTION_EVENT(OnDownloadComplete, FTryllOnDownloadCompleteNative, ForwardDownloadComplete)
TRYLL_CONNECTION_EVENT(OnLoadModelComplete, FTryllOnLoadModelCompleteNative, ForwardLoadModel)
TRYLL_CONNECTION_EVENT(OnUnloadModelComplete, FTryllOnUnloadModelCompleteNative, ForwardUnloadModel)
TRYLL_CONNECTION_EVENT(OnCreateStringStorageComplete, FTryllOnCreateStringStorageCompleteNative, ForwardCreateStringStorage)
TRYLL_CONNECTION_EVENT(OnDestroyStringStorageComplete, FTryllOnDestroyStringStorageCompleteNative, ForwardDestroyStringStorage)
TRYLL_CONNECTION_EVENT(OnCreateEmbeddedStringStorageComplete, FTryllOnCreateEmbeddedStringStorageCompleteNative, ForwardCreateEmbedded)
TRYLL_CONNECTION_EVENT(OnDestroyEmbeddedStringStorageComplete, FTryllOnDestroyEmbeddedStringStorageCompleteNative, ForwardDestroyEmbedded)

#undef TRYLL_CONNECTION_EVENT
