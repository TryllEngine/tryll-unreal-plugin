// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// Hand-written once — do not delete.
// Generated per-node subclasses live alongside in this directory.
#pragma once

enum class ETryllNodeType : uint8;
namespace Tryll::NodeParams { enum NodeParams : uint8_t; }
struct FTryllGraphDescription;

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include <flatbuffers/flatbuffers.h>
#include "TryllNodeParamsBase.generated.h"

/** Abstract base for typed node-parameter UObjects (generated subclasses in this directory). */
UCLASS(Abstract, EditInlineNew, BlueprintType, meta=(BlueprintSpawnableComponent="false"))
class TRYLLCLIENT_API UTryllNodeParamsBase : public UObject
{
	GENERATED_BODY()
public:
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;

	UFUNCTION(BlueprintPure, Category="Tryll|Params")
	virtual ETryllNodeType GetNodeType() const PURE_VIRTUAL(GetNodeType, return static_cast<ETryllNodeType>(0););

	/** True when this node speaks through the agent's voice (Speak, GenerateAndSpeak).
	 *  A graph with such a node needs a voice profile. From the schema's (tts_consumer). */
	UFUNCTION(BlueprintPure, Category="Tryll|Params")
	virtual bool IsTtsConsumer() const { return false; }

	/** Reference page of this node type on docs.tryllengine.com, opened by the editor's
	 *  help button. Empty for test-only node types, which have no page. */
	UFUNCTION(BlueprintPure, Category="Tryll|Params")
	virtual FString GetDocsUrl() const { return FString(); }

	virtual flatbuffers::Offset<void> Pack(
		flatbuffers::FlatBufferBuilder& Fbb,
		::Tryll::NodeParams::NodeParams& OutType) const
		PURE_VIRTUAL(Pack, return flatbuffers::Offset<void>{};);

	/**
	 * Dropdown options for fields tagged with FlatBuffers `(exit)`.
	 * Walks the Outer chain to find the enclosing FTryllGraphDescription
	 * (typically owned by a UTryllWorkflowAsset),
	 * then returns: [""] + node names from that graph.
	 *
	 * The leading empty string represents END (turn finishes after this node);
	 * Unreal's Details panel renders it as a blank entry in the dropdown.
	 *
	 * Bound via UPROPERTY meta=(GetOptions="GetExitTargetOptions") on every
	 * codegen-emitted exit field.  When the params UObject is not part of an
	 * editable graph (orphaned / runtime-constructed), returns just [""].
	 */
	UFUNCTION()
	virtual TArray<FString> GetExitTargetOptions() const;

	/**
	 * Walks the Outer chain from @p Obj to locate the enclosing FTryllGraphDescription
	 * (a UTryllWorkflowAsset's Graph). Returns nullptr when @p Obj is not part of an
	 * editable graph (orphaned / runtime-constructed). Shared by GetExitTargetOptions
	 * and the editor-only slot-input dropdown / rename-propagation tooling
	 * (TryllClientEditor).
	 */
	static const FTryllGraphDescription* FindEnclosingGraph(const UObject* Obj);

	/**
	 * The node name that owns this Params instance within FindEnclosingGraph(this) —
	 * i.e. the FTryllNodeDescription whose Params pointer equals `this` — or the empty
	 * string when not found.
	 */
	FString GetOwnerNodeName() const;

protected:
	/** Called before saved properties load, for packages saved at SavedVersion (an FTryllAssetVersion). */
	virtual void ApplyOldDefaults(int32 SavedVersion);

	/** Called after load, for packages saved at SavedVersion. Returns true if anything changed. */
	virtual bool UpgradeFrom(int32 SavedVersion);
};
