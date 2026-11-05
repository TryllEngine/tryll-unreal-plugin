// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllModelName.h"
#include "TryllGraphDescription.generated.h"

class UTryllNodeParamsBase;

/**
 * Inference engine the session should use for model loading.
 * Mirrors the FlatBuffers Engine::InferenceEngine enum in messages.fbs — ordinal values must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllInferenceEngine : uint8
{
	/** In-process mock engine used by tests. Performs no real inference. */
	Mock        = 0,
	/** llama.cpp GGUF runtime. The default production engine. */
	LlamaCpp    = 1,
	/** ONNX GenAI — reserved; not yet implemented. */
	OnnxGenAI   = 2,
	/** Windows ML — reserved; not yet implemented. */
	WindowsML   = 3,
	/** Intel OpenVINO — reserved; not yet implemented. */
	OpenVino    = 4,
	/** NVIDIA TensorRT-LLM — reserved; not yet implemented. */
	TensorRtLlm = 5,
	/** Sherpa-ONNX — STT/TTS engine. */
	SherpaOnnx  = 6,
	/** Bare ONNX Runtime — encoder-shaped graphs and the Chatterbox TTS family. */
	OnnxRuntime = 7,
};

/**
 * Workflow node types supported by the server.
 * Mirrors the NodeParams union order in nodes.fbs (0-based; wire discriminator is +1).
 */
UENUM(BlueprintType)
enum class ETryllNodeType : uint8
{
	/** LLM-driven text generation. */
	Generate              = 0,
	/** Pattern-match on an input slot and route accordingly. */
	RegexGuardrail = 1,
	/** Emit a pre-written reply without invoking a model. */
	CannedResponse        = 2,
	/** Detect a tool-call emission from the model (optionally notifying the client). */
	ToolCall              = 3,
	/** RAG retrieval from a session-scoped embedded string storage. */
	Retrieve              = 4,
	/** Attach a named instruction string to the current interaction. */
	Instruction           = 5,
	/** Top-1 embedding similarity against a labelled KB; attaches IntentionComponent for downstream routing. */
	ClassifyIntent        = 6,
	/** Look up detected intent in a Map-kind StringStorage; attaches a kind=Instruction SlotComponent. */
	IntentToInstruction   = 7,
	/** Single-token logprob intent classifier (SLM-based, no generation). */
	ClassifyIntentLLM     = 8,
	/** LLM inference + TTS synthesis; streams PCM audio frames. */
	GenerateAndSpeak      = 9,
	/** TTS-only synthesis for upstream/canned/inline text; no LLM inference. */
	Speak                 = 10,
	/** No-op checkpoint — the executor pauses the turn after this node exits. */
	Pause                 = 11,
	/** Model-free: renders a Mustache template into its own slot. No inference, no wire emission. */
	Transform             = 12,
	/** Conditional routing on a slot or variable; then/else exits. No model, no wire emission. */
	Branch                = 13,
	/** Experimental upsert into a named mutable knowledge base. */
	WriteToMemory         = 14,
	/** Folds ageing turns into a rolling summary that Generate-class nodes render in their system message. */
	Summarize             = 15,
	ClassifyAttributesLLM = 16,
};

// ---------------------------------------------------------------------------
// Node-parameter enums (mirrors common_enums.fbs).
// ---------------------------------------------------------------------------

/**
 * Where the rendered Mustache body is inserted relative to the node's resolved Input.
 * Mirrors the Placement enum in common_enums.fbs — ordinals must stay in sync.
 * There is no more InPlaceOfUser — use a Transform node + Input to fully replace what
 * the model sees.
 */
UENUM(BlueprintType)
enum class ETryllPlacement : uint8
{
	/** Inserted as role=user before the resolved input. */
	BeforeUserAsUser   = 0,
	/** Inserted as role=system before the resolved input. */
	BeforeUserAsSystem = 1,
	/** Inserted as role=user after the resolved input. */
	AfterUserAsUser    = 2,
	/** Inserted as role=system after the resolved input. */
	AfterUserAsSystem  = 3,
};

/**
 * Controls answer-text delivery to the client. Mirrors SendAnswer in common_enums.fbs.
 */
UENUM(BlueprintType)
enum class ETryllSendAnswer : uint8
{
	/** Captured into the node's slot only — never sent as AnswerText. */
	None     = 0,
	/** One AnswerText frame, sent once the full text is ready. */
	Whole    = 1,
	/** Per-delta AnswerText frames as text is generated. */
	Streamed = 2,
};

/**
 * Controls whether/how a node's slot replays as history on later turns.
 * Mirrors HistoryRole in common_enums.fbs.
 */
UENUM(BlueprintType)
enum class ETryllHistoryRole : uint8
{
	/** Never replayed — a same-turn-only scratch value. */
	None      = 0,
	/** Replayed as a separate assistant message on later turns. */
	Assistant = 1,
};

/**
 * Selection strategy for the CannedResponse node.
 * Mirrors the CannedResponseSelectionStrategy enum in common_enums.fbs.
 */
UENUM(BlueprintType)
enum class ETryllCannedResponseSelectionStrategy : uint8
{
	Random     = 0,
	First      = 1,
	RoundRobin = 2,
};

/**
 * Tool-choice policy for the ToolCall node.
 * Mirrors the ToolCallMode enum in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllToolCallMode : uint8
{
	/** Model may call a tool or stay silent; no answer text when no call is parsed. */
	DetectOnly   = 0,
	/** Same as DetectOnly, but residual text is emitted as a plain answer when no call is parsed. */
	CallOrAnswer = 1,
	/** At least one tool call is mandatory; the model cannot produce plain text. */
	RequireCall  = 2,
};

/**
 * RetrieveNode search path.
 * Mirrors the RetrievalMode enum in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllRetrievalMode : uint8
{
	/** HNSW cosine only (historical default). */
	Dense   = 0,
	/** BM25 lexical only. */
	Lexical = 1,
	/** Dense + BM25 fused with Reciprocal Rank Fusion. */
	Hybrid  = 2,
};

/**
 * How Retrieve addresses a knowledge source.
 * Mirrors KnowledgeSourceKind in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllKnowledgeSourceKind : uint8
{
	/** File-backed embedded string storage. */
	FromFile = 0,
	/** Session-scoped named mutable store. */
	FromName = 1,
};

/**
 * End-to-end ToolCall notify / pause / history policy.
 * Mirrors ToolCallDisposition in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllToolCallDisposition : uint8
{
	/** No event, no pause; omit tool syntax from history. */
	RouteOnly            = 0,
	/** No event, no pause; synthetic acknowledgement history. Schema default. */
	Acknowledge          = 1,
	/** Emit tool_call event; no pause; omit from history. */
	Notify               = 2,
	/** Emit tool_call event; no pause; synthetic acknowledgement history. */
	NotifyAndAcknowledge = 3,
	/** Emit tool_call event and pause; omit from history. */
	Pause                = 4,
	/** Emit tool_call event and pause; synthetic acknowledgement history. */
	PauseAndAcknowledge  = 5,
	/** Emit tool_call event, pause, and require client Resume tool_results. */
	AwaitResult          = 6,
};

/**
 * When a ClassifyIntent / ClassifyIntentLLM node emits a NodeEvent.
 * Mirrors NotifyClient in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllNotifyClient : uint8
{
	Disabled = 0,
	OnFound  = 1,
	Always   = 2,
};

/**
 * llama.cpp KV cache dtype on ContextOverrides.
 * Mirrors KvCacheType in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllKvCacheType : uint8
{
	Inherit = 0,
	F16     = 1,
	Q8_0    = 2,
	Q4_0    = 3,
};

/**
 * How ClassifyAttributesLLM turns a scored distribution into a slot value.
 * Mirrors AttributeKind in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllAttributeKind : uint8
{
	/** Winner choice id after threshold and margin. */
	Choice = 0,
	/** One statement scored as A=True / B=False. */
	Boolean = 1,
	/** Ordered levels normalised to 0..1. */
	Score = 2,
};

/** Which Boolean result takes the triggered exit. */
UENUM(BlueprintType)
enum class ETryllBooleanTrigger : uint8
{
	Never = 0,
	// UHT rejects enumerator names True and False. Ordinals stay 1 and 2.
	True_ = 1 UMETA(DisplayName = "True"),
	False_ = 2 UMETA(DisplayName = "False"),
};

/** How a normalised Score compares with its threshold. */
UENUM(BlueprintType)
enum class ETryllScoreTrigger : uint8
{
	Never = 0,
	AtOrAbove = 1,
	Below = 2,
};

/** Exit taken when ClassifyAttributesLLM cannot score. */
UENUM(BlueprintType)
enum class ETryllAttributeFailureRoute : uint8
{
	Default = 0,
	Triggered = 1,
};

/**
 * Predicate policy for the Branch node.
 * Mirrors the BranchTest enum in common_enums.fbs — ordinals must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllBranchTest : uint8
{
	NonEmpty         = 0,
	IsTrue           = 1,
	Equals           = 2,
	EqualsIgnoreCase = 3,
	Contains         = 4,
	Regex            = 5,
	Expression       = 6,
};

/** Outcome of a workflow turn. Mirrors TurnStatus in messages.fbs. */
UENUM(BlueprintType)
enum class ETryllTurnStatus : uint8
{
	/** Turn completed successfully. */
	Success   = 0,
	/** Turn ended with a server-side error; inspect FTryllError from OnError. */
	Error     = 1,
	/** Turn was cancelled (client disconnect, agent destroyed, or explicit cancel). */
	Cancelled = 2,
	/**
	 * A pause outlived its Pause Timeout Ms budget and the pausing node had no
	 * Timeout Exit, so the server abandoned the turn. Nobody requested this — your
	 * game never called Resume — which is what makes it distinct from Cancelled.
	 * The partial interaction is kept. Bind the subsystem's OnPaused event (or
	 * register a tool handler) so pauses get resumed.
	 */
	PauseTimedOut = 3,
};

/** How a mid-turn cancel treats the in-flight interaction. Mirrors CancelMode in messages.fbs. */
UENUM(BlueprintType)
enum class ETryllCancelMode : uint8
{
	/** Stop generating; keep the partial interaction in history (chat "Stop"). */
	StopAndKeep    = 0,
	/** Stop and discard the last interaction, as if the turn never happened. */
	StopAndDiscard = 1,
};

/** Description of one workflow node sent to the server. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllNodeDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Graph")
	FString Name;

	/**
	 * Typed node parameters.  Use UTryllNodeParamsFactory::MakeXxxParams() to create;
	 * use Instanced so the UObject is serialised inline in Blueprint/editor assets.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Tryll|Graph")
	TObjectPtr<UTryllNodeParamsBase> Params;

#if WITH_EDITORONLY_DATA
	/**
	 * Canvas position in the visual graph editor. Authoring-only: stripped from
	 * cooked/runtime builds and never sent on the wire. The graph editor persists
	 * the node's location here; nodes with the sentinel value (FVector2D::ZeroVector
	 * is treated as "unset" only when bHasEditorPos is false) are auto-laid-out.
	 */
	UPROPERTY()
	FVector2D EditorPos = FVector2D::ZeroVector;

	/** True once EditorPos has been assigned by the graph editor (distinguishes a real (0,0) from "never placed"). */
	UPROPERTY()
	bool bHasEditorPos = false;
#endif
};

// ── Graph description ─────────────────────────────────────────────────────────

/**
 * Complete graph description: nodes and entry point. Sent inside CreateAgentRequest.
 *
 * Protocol v2: wiring lives on each node's typed UTryllNodeParamsBase in fields
 * marked with `meta=(GetOptions="GetExitTargetOptions")` (one per declared exit).
 * Empty value = END (turn finishes after the node).  There is no separate Routes
 * list.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllGraphDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Graph")
	TArray<FTryllNodeDescription> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Graph",
		meta = (GetOptions = "GetStartNodeOptions"))
	FString StartNode;

	/**
	 * Fallback model name for nodes that do not specify their own "model_name" param.
	 * Empty means every Generate node must supply its own.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Graph", meta = (TryllModelType = "language"))
	FTryllModelName DefaultModelName;

	/** Table name passed to the text provider. Copied from UTryllWorkflowAsset::TextTable. */
	UPROPERTY()
	FName TextTable;

	/** Asset name included in Text ID errors. Copied from the workflow asset. */
	UPROPERTY()
	FString LocalizedAssetName;

	/**
	 * True when any node speaks through the agent's voice (UTryllNodeParamsBase::IsTtsConsumer:
	 * Speak, Generate And Speak). Such a graph needs a voice profile at CreateAgent (3025).
	 */
	bool UsesTts() const;
};

// ── Graph-option helpers ──────────────────────────────────────────────────────

/**
 * Shared helpers for Details-panel dropdowns that enumerate node names.
 * Used by GetExitTargetOptions (on UTryllNodeParamsBase) and GetStartNodeOptions
 * (on UTryllWorkflowAsset).
 */
struct TRYLLCLIENT_API FTryllGraphOptions
{
	/** Returns all non-empty node names from Graph.Nodes, in authoring order. No END sentinel. */
	static TArray<FString> GetNodeNameOptions(const FTryllGraphDescription& Graph);
};

// ── Graph builder ─────────────────────────────────────────────────────────────

/**
 * Fluent C++ builder for FTryllGraphDescription.
 * Typed AddXxx methods are declared in TryllGraphBuilder.Nodes.h (generated).
 * Not a USTRUCT — for programmatic C++ use only.
 *
 * Example:
 *   UTryllGenerateParams* P = UTryllNodeParamsFactory::MakeGenerateParams(this);
 *   P->ModelName    = TEXT("qwen2.5-0.5b-instruct");
 *   P->SystemPrompt = TEXT("You are helpful.");
 *
 *   // Protocol v2: wiring lives on each node's params (e.g. P->DefaultExit).
 *   FTryllGraphDescription Graph = FTryllGraphBuilder()
 *       .AddGenerate(TEXT("gen"), P)  // P->DefaultExit defaults to "" = END
 *       .SetStartNode(TEXT("gen"))
 *       .Build();
 */
struct TRYLLCLIENT_API FTryllGraphBuilder
{
	// Typed AddXxx methods declared in TryllGraphBuilder.Nodes.h (generated from nodes.fbs).
	// Include that header to get them.

	FTryllGraphBuilder& AddNode(FString Name, UTryllNodeParamsBase* Params);

	FTryllGraphBuilder& SetStartNode(FString Name);
	FTryllGraphBuilder& SetDefaultModelName(FString Name);

	[[nodiscard]] FTryllGraphDescription Build() const { return Description; }

private:
	FTryllGraphDescription Description;
};
