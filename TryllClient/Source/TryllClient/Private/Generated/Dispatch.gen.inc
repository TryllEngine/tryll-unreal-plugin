// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_dispatch.inc.j2
// See: docs/dev/codegen.md
//
// Usage: #include this file inside a switch(msg->body_type()) { ... } block after
// defining the macros you need.  Any undefined macros expand to a no-op.
//
//   Server — dispatching incoming client-to-server messages:
//     #define TRYLL_MSG_REQ(Name, Handler)             case ::Tryll::MessageBody_##Name: { ... }
//     #define TRYLL_MSG_FIRE_AND_FORGET(Name, Handler) case ::Tryll::MessageBody_##Name: { ... }
//
//   Client — dispatching incoming server-to-client messages:
//     #define TRYLL_MSG_RESP(Name, Handler)   case ::Tryll::MessageBody_##Name: { ... }
//     #define TRYLL_MSG_STREAM(Name, Handler) case ::Tryll::MessageBody_##Name: { ... }
//     #define TRYLL_MSG_NOTIF(Name, Handler)  case ::Tryll::MessageBody_##Name: { ... }
//
//   Unknown arm (must be last):
//     #define TRYLL_MSG_UNKNOWN()             default: { ... }
//
// All macros default to a no-op if not defined before the include.

#ifndef TRYLL_MSG_REQ
#  define TRYLL_MSG_REQ(Name, Handler)
#endif
#ifndef TRYLL_MSG_FIRE_AND_FORGET
#  define TRYLL_MSG_FIRE_AND_FORGET(Name, Handler)
#endif
#ifndef TRYLL_MSG_RESP
#  define TRYLL_MSG_RESP(Name, Handler)
#endif
#ifndef TRYLL_MSG_STREAM
#  define TRYLL_MSG_STREAM(Name, Handler)
#endif
#ifndef TRYLL_MSG_NOTIF
#  define TRYLL_MSG_NOTIF(Name, Handler)
#endif
#ifndef TRYLL_MSG_UNKNOWN
#  define TRYLL_MSG_UNKNOWN()
#endif

TRYLL_MSG_NOTIF(ConnectionReady, OnIncomingConnectionReady)
TRYLL_MSG_REQ(CreateSessionRequest, OnCreateSessionRequest)
TRYLL_MSG_RESP(CreateSessionResponse, OnIncomingCreateSessionResponse)
TRYLL_MSG_REQ(CreateAgentRequest, OnCreateAgentRequest)
TRYLL_MSG_RESP(CreateAgentResponse, OnIncomingCreateAgentResponse)
TRYLL_MSG_REQ(SendMessageRequest, OnSendMessageRequest)
TRYLL_MSG_REQ(DestroyAgentRequest, OnDestroyAgentRequest)
TRYLL_MSG_STREAM(AnswerText, OnIncomingAnswerText)
TRYLL_MSG_STREAM(TurnComplete, OnIncomingTurnComplete)
TRYLL_MSG_RESP(ErrorResponse, OnIncomingErrorResponse)
TRYLL_MSG_RESP(Ack, OnIncomingAck)
TRYLL_MSG_REQ(ListModelsRequest, OnListModelsRequest)
TRYLL_MSG_RESP(ListModelsResponse, OnIncomingListModelsResponse)
TRYLL_MSG_REQ(DownloadModelRequest, OnDownloadModelRequest)
TRYLL_MSG_STREAM(DownloadProgress, OnIncomingDownloadProgress)
TRYLL_MSG_RESP(DownloadComplete, OnIncomingDownloadComplete)
TRYLL_MSG_REQ(CreateStringStorageRequest, OnCreateStringStorageRequest)
TRYLL_MSG_RESP(CreateStringStorageResponse, OnIncomingCreateStringStorageResponse)
TRYLL_MSG_REQ(DestroyStringStorageRequest, OnDestroyStringStorageRequest)
TRYLL_MSG_NOTIF(NodeEvent, OnIncomingNodeEvent)
TRYLL_MSG_REQ(LoadModelRequest, OnLoadModelRequest)
TRYLL_MSG_RESP(LoadModelResponse, OnIncomingLoadModelResponse)
TRYLL_MSG_REQ(UnloadModelRequest, OnUnloadModelRequest)
TRYLL_MSG_REQ(CreateEmbeddedStringStorageRequest, OnCreateEmbeddedStringStorageRequest)
TRYLL_MSG_RESP(CreateEmbeddedStringStorageResponse, OnIncomingCreateEmbeddedStringStorageResponse)
TRYLL_MSG_REQ(DestroyEmbeddedStringStorageRequest, OnDestroyEmbeddedStringStorageRequest)
TRYLL_MSG_REQ(ChangeAgentParamRequest, OnChangeAgentParamRequest)
TRYLL_MSG_REQ(CreateVoiceInputRequest, OnCreateVoiceInputRequest)
TRYLL_MSG_RESP(CreateVoiceInputResponse, OnIncomingCreateVoiceInputResponse)
TRYLL_MSG_REQ(BeginUtteranceRequest, OnBeginUtteranceRequest)
TRYLL_MSG_FIRE_AND_FORGET(AudioBuffer, OnAudioBuffer)
TRYLL_MSG_REQ(EndUtteranceRequest, OnEndUtteranceRequest)
TRYLL_MSG_REQ(CancelUtteranceRequest, OnCancelUtteranceRequest)
TRYLL_MSG_NOTIF(WireTranscriptUpdate, OnIncomingWireTranscriptUpdate)
TRYLL_MSG_REQ(DestroyVoiceInputRequest, OnDestroyVoiceInputRequest)
TRYLL_MSG_STREAM(TtsAudioFormatFrame, OnIncomingTtsAudioFormatFrame)
TRYLL_MSG_STREAM(TtsAudioFrame, OnIncomingTtsAudioFrame)
TRYLL_MSG_REQ(DeleteModelRequest, OnDeleteModelRequest)
TRYLL_MSG_REQ(DebugCommandRequest, OnDebugCommandRequest)
TRYLL_MSG_RESP(DebugCommandResponse, OnIncomingDebugCommandResponse)
TRYLL_MSG_REQ(CancelRequest, OnCancelRequest)
TRYLL_MSG_REQ(ResumeAgentRequest, OnResumeAgentRequest)
TRYLL_MSG_REQ(UpdateAgentVariablesRequest, OnUpdateAgentVariablesRequest)
TRYLL_MSG_REQ(GetAgentVariablesRequest, OnGetAgentVariablesRequest)
TRYLL_MSG_RESP(GetAgentVariablesResponse, OnIncomingGetAgentVariablesResponse)
TRYLL_MSG_REQ(PrefillAgentKvCacheRequest, OnPrefillAgentKvCacheRequest)
TRYLL_MSG_RESP(PrefillAgentKvCacheResponse, OnIncomingPrefillAgentKvCacheResponse)
TRYLL_MSG_REQ(EvictAgentKvCacheRequest, OnEvictAgentKvCacheRequest)
TRYLL_MSG_RESP(EvictAgentKvCacheResponse, OnIncomingEvictAgentKvCacheResponse)
TRYLL_MSG_REQ(GetAgentKvCacheStatusRequest, OnGetAgentKvCacheStatusRequest)
TRYLL_MSG_RESP(GetAgentKvCacheStatusResponse, OnIncomingGetAgentKvCacheStatusResponse)
TRYLL_MSG_REQ(AppendInteractionsRequest, OnAppendInteractionsRequest)
TRYLL_MSG_RESP(AppendInteractionsResponse, OnIncomingAppendInteractionsResponse)
TRYLL_MSG_REQ(RemoveInteractionsFromEndRequest, OnRemoveInteractionsFromEndRequest)
TRYLL_MSG_RESP(RemoveInteractionsFromEndResponse, OnIncomingRemoveInteractionsFromEndResponse)
TRYLL_MSG_FIRE_AND_FORGET(SetInferenceThrottle, OnSetInferenceThrottle)
TRYLL_MSG_REQ(SetSessionLocaleRequest, OnSetSessionLocaleRequest)
TRYLL_MSG_REQ(ExportDialogRequest, OnExportDialogRequest)
TRYLL_MSG_RESP(ExportDialogResponse, OnIncomingExportDialogResponse)
TRYLL_MSG_REQ(ImportDialogRequest, OnImportDialogRequest)
TRYLL_MSG_RESP(ImportDialogResponse, OnIncomingImportDialogResponse)
TRYLL_MSG_REQ(GetMemoryConsumptionRequest, OnGetMemoryConsumptionRequest)
TRYLL_MSG_RESP(GetMemoryConsumptionResponse, OnIncomingGetMemoryConsumptionResponse)
TRYLL_MSG_REQ(EnsureMutableKbRequest, OnEnsureMutableKbRequest)
TRYLL_MSG_RESP(EnsureMutableKbResponse, OnIncomingEnsureMutableKbResponse)
TRYLL_MSG_REQ(EnumerateMutableKbsRequest, OnEnumerateMutableKbsRequest)
TRYLL_MSG_RESP(EnumerateMutableKbsResponse, OnIncomingEnumerateMutableKbsResponse)
TRYLL_MSG_REQ(MutableKbWriteRequest, OnMutableKbWriteRequest)
TRYLL_MSG_RESP(MutableKbWriteResponse, OnIncomingMutableKbWriteResponse)
TRYLL_MSG_REQ(MutableKbDeleteRequest, OnMutableKbDeleteRequest)
TRYLL_MSG_RESP(MutableKbDeleteResponse, OnIncomingMutableKbDeleteResponse)
TRYLL_MSG_REQ(MutableKbClearRequest, OnMutableKbClearRequest)
TRYLL_MSG_RESP(MutableKbClearResponse, OnIncomingMutableKbClearResponse)
TRYLL_MSG_REQ(MutableKbQueryRequest, OnMutableKbQueryRequest)
TRYLL_MSG_RESP(MutableKbQueryResponse, OnIncomingMutableKbQueryResponse)
TRYLL_MSG_REQ(MutableKbGetRequest, OnMutableKbGetRequest)
TRYLL_MSG_RESP(MutableKbGetResponse, OnIncomingMutableKbGetResponse)
TRYLL_MSG_REQ(MutableKbListRequest, OnMutableKbListRequest)
TRYLL_MSG_RESP(MutableKbListResponse, OnIncomingMutableKbListResponse)
TRYLL_MSG_REQ(MutableKbStatsRequest, OnMutableKbStatsRequest)
TRYLL_MSG_RESP(MutableKbStatsResponse, OnIncomingMutableKbStatsResponse)
TRYLL_MSG_REQ(ExportMutableKbRequest, OnExportMutableKbRequest)
TRYLL_MSG_RESP(ExportMutableKbResponse, OnIncomingExportMutableKbResponse)
TRYLL_MSG_REQ(ImportMutableKbRequest, OnImportMutableKbRequest)
TRYLL_MSG_RESP(ImportMutableKbResponse, OnIncomingImportMutableKbResponse)
TRYLL_MSG_REQ(SaveGameMemoryRequest, OnSaveGameMemoryRequest)
TRYLL_MSG_RESP(SaveGameMemoryResponse, OnIncomingSaveGameMemoryResponse)
TRYLL_MSG_REQ(LoadGameMemoryRequest, OnLoadGameMemoryRequest)
TRYLL_MSG_RESP(LoadGameMemoryResponse, OnIncomingLoadGameMemoryResponse)
TRYLL_MSG_REQ(SaveDialogRequest, OnSaveDialogRequest)
TRYLL_MSG_RESP(SaveDialogResponse, OnIncomingSaveDialogResponse)
TRYLL_MSG_REQ(LoadDialogRequest, OnLoadDialogRequest)
TRYLL_MSG_RESP(LoadDialogResponse, OnIncomingLoadDialogResponse)
TRYLL_MSG_STREAM(TtsVisemeFrame, OnIncomingTtsVisemeFrame)
TRYLL_MSG_REQ(SetAgentVoiceRequest, OnSetAgentVoiceRequest)
TRYLL_MSG_REQ(SetAgentDeliveryModeRequest, OnSetAgentDeliveryModeRequest)
TRYLL_MSG_UNKNOWN()

#undef TRYLL_MSG_REQ
#undef TRYLL_MSG_FIRE_AND_FORGET
#undef TRYLL_MSG_RESP
#undef TRYLL_MSG_STREAM
#undef TRYLL_MSG_NOTIF
#undef TRYLL_MSG_UNKNOWN
