# Tryll Engine (Unreal) — context for AI coding agents

**Tryll plugin version:** 0.10.1

You are looking at the Tryll Unreal Engine plugin. This file is a **map, not a manual** —
it gives you the mental model and the gotchas, then points at the full docs. Fetch a linked
page whenever you need detail; do not guess an API from its name.

- Full docs index for agents: **<https://docs.tryllengine.com/v0.10/llms.txt>**
- Every page below is also available as clean Markdown by swapping `.html` → `.md`.
- **If this file and the docs disagree, the docs win.** This file ships frozen inside the
  plugin; the site tracks the current release.

---

## What Tryll is

Tryll runs **small language models on the player's own machine**. A C++ TCP server
(`tryll_server.exe`, bundled in this plugin under
`Binaries/ThirdParty/TryllServer/Default/`) hosts the models; your C++ or Blueprint code
drives it over a binary FlatBuffers protocol. No cloud API, no API keys, no per-token
cost, works offline.

The practical consequences: models are **files that must exist on disk** before use, VRAM
and RAM are **your budget to manage**, and the models are **small** (1–8 B) — they need
tighter prompting and guardrails than a frontier cloud model.

## The mental model

A Tryll **agent is not a prompt.** It is a **workflow graph** of typed nodes, authored
once and re-walked on every user turn.

- A **graph** = a set of named **nodes** + a **start node**.
- Each node has **typed params** carrying both its config *and* its exit wiring. Exits are
  plain string fields on the params: `default_exit`, `then_exit`, `triggered_exit`, …
  An empty string means **END**. There is no separate routes/edges array.
- Nodes pass values through **slots** — a per-turn blackboard. A producing node writes a
  slot; `input` selects which slot a downstream node reads. `user_message` is the reserved
  implicit input slot. Mustache templates address slots as `{{slot.<name>}}`.
- `send` decides whether a slot's text goes to the client (`None` / `Whole` / `Streamed`);
  `history_role` decides whether it replays as dialogue history on later turns.

Read these two pages before writing any graph:

- <https://docs.tryllengine.com/v0.10/concepts/workflows-graphs-nodes.md>
- <https://docs.tryllengine.com/v0.10/concepts/slots-and-inter-node-value-passing.md>

## Things that differ from cloud LLM SDKs

These are the mistakes an agent makes by pattern-matching on OpenAI/Anthropic SDKs.

1. **Models are never auto-downloaded.** Acquire a model first — the editor **Model
   Manager** panel, or `UTryllSubsystem::RequestDownloadModel(Name)` at runtime. Creating
   an agent whose graph references an absent model fails fast with
   `GraphCompilationFailed`. This applies to TTS voices too.
2. **A session is mandatory and one-shot.** `Connect()` establishes the connection;
   a session must then be created (`CreateSessionFromSettings()`) before anything else, or
   requests are rejected with `SessionNotReady`. A second attempt fails with
   `SessionAlreadyExists`. With **Auto Create Session** (`bAutoCreateSession`, on by
   default) the subsystem creates the session for you when `Connect()` succeeds, so do
   not also call `CreateSessionFromSettings()` unless `bAutoCreateSession` is false.
   Unity differs: its **Auto Start Session** is off by default.
3. **There is no `stream: bool`.** Use `send: SendAnswer` and `history_role` (see above).
4. **Never hand-translate node type numbers.** The client `ETryllNodeType` enum and the
   wire `NodeParams` union discriminator are **two independent numbering schemes** and they
   diverge for newer node types. Always use the named enum value or the builder method.
5. **Some params are structural** — fixed for the agent's lifetime. Changing one at runtime
   is rejected with `ParamNotMutable`. Rebuild the agent instead.
6. **Storage is usually a relative file path**, not a wire call. Node params like
   `string_storage` / `embedded_string_storage` accept a path resolved against the
   process storage root (`UTryllRuntimeSettings::StorageDataFolder`, default
   `TryllStorage` outside `Content/`); the server loads it at agent creation.
   Prefer this over creating storages over the wire. The storage root is set when
   the server launches (`--storage-root`) — `CreateSession` does not take a folder
   (protocol v24).
7. **Turns are cancellable and pausable, not fire-and-forget.** A turn can pause between
   nodes (tool calls, `Pause` node) and **something must resume it**. A pause holds the
   turn open: the agent stays busy and every later `Send Message` fails with `AgentBusy`
   (3004) until `Resume`, `Cancel`, or the pause's timeout ends it.
   So if a workflow contains a `Pause` node or a pausing `Tool Call` disposition, the game
   must either bind `OnPaused` (on `UTryllAgentComponent`, or `UTryllSubsystem` for all
   agents) and call `Resume`, or `Register Tool` a handler for every tool in the batch
   (which auto-resumes). A pause can time out. On a `Pause` node, `Pause Timeout Ms`
   (`pause_timeout_ms`) `0`, the default, waits forever. With a timeout, `Timeout Exit`
   (`timeout_exit`) continues the graph — use it so the character says *something*
   instead of going quiet; an empty timeout exit ends the turn with `PauseTimedOut`. On a
   `Tool Call` node, `pause_timeout_ms` `0` inherits the server's
   `workflow.tool_call_pause_timeout_ms`, which is **0 (no deadline)** by default and
   in the released server. A source checkout's `data/server-config.json` sets `2000`
   so contributors notice a missing `Resume`. There is no timeout exit on `Tool Call`: an unresumed tool-call
   pause ends the turn with `PauseTimedOut`. `Get Paused Node` reports the parked node,
   and the plugin logs a warning when a pause arrives that nothing can resume.

## Unreal API map

| You want to… | Use |
|---|---|
| Connect / own the session | `UTryllSubsystem` (GameInstance subsystem) |
| Run an agent on an Actor | `UTryllAgentComponent` |
| Author a graph as an asset | `UTryllWorkflowAsset` (has a visual graph editor) |
| Build a graph in code | `FTryllGraphBuilder` |
| Configure host/port/auto-launch/llama.cpp GPU API/storage root | `UTryllRuntimeSettings` (**Project Settings**) |
| Resolve Localized prompt Text IDs | `UTryllRuntimeSettings::TextProviderClass` (None by default; pick `UTryllStringTableTextProvider` for String Tables) |
| Play streamed TTS audio | `UTryllSpeakerComponent` |
| Capture mic → speech-to-text | `UTryllVoiceInputComponent` |
| Read/write per-agent Variables | `UTryllAgentComponent::GetVariables()` |

Key members — see the reference pages for exact signatures:

- `UTryllSubsystem` — `Connect()`, `Disconnect()`, `IsConnected()`, `CreateSession()`,
  `CreateSessionFromSettings()`, `IsSessionReady()`, `RequestDownloadModel()`,
  `RequestLoadModel()`, `RequestUnloadModel()`,
  `RequestGetMemoryConsumption()`, `RequestEnsureMutableKb()`,
  `RequestEnumerateMutableKbs()`, `RequestSaveGameMemory()`,
  `RequestLoadGameMemory()`. Handle ops are on `FTryllMutableKb`
  (`Write` / `Delete` / `Clear` / `Query` / `Get` / `List` / `Stats` /
  `Export` / `Import`).
- `UTryllAgentComponent` — properties `WorkflowAsset`,
  `bAutoCreateOnConnect`; methods `CreateAgent()`, `SendMessage(const FString&)`,
  `Cancel()`, `Resume()`, `RegisterTool()` / `UnregisterTool()` (tool handlers that
  auto-resume pausing tool calls), `ChangeParams()`, `AppendInteractions()`, `DestroyAgent()`,
  `IsAgentReady()`; `BlueprintAssignable` delegates `OnAgentReady`, `OnAnswerText`,
  `OnAnswerFull`, `OnTurnComplete`, `OnError`, `OnParamChanged`, `OnPaused`,
  `OnResumed`.
- Node params are `UObject`s: `UTryllGenerateParams`, `UTryllRetrieveParams`,
  `UTryllBranchParams`, … all deriving from `UTryllNodeParamsBase`. Construct them via
  `UTryllNodeParamsFactory` (e.g. `MakeGenerateParams`), and clone before mutating with
  `UTryllNodeParamsFactory::CloneParams`.
- Builder methods are `Add<NodeType>Node` — e.g. `AddGenerateNode(name, …, Params)`,
  `AddGenerateAndSpeakNode(…)`. **Note:** Unreal uses `AddGenerateNode`; the Unity package
  uses `AddGenerate`. Do not copy names across engines.
- Everything above is Blueprint-exposed — see the Blueprint catalog if the user is working
  in Blueprints rather than C++.

Shape of a minimal integration:

1. Set the Tryll section of **Project Settings** (enable auto-launch for local
   dev; **Llama.cpp backend** defaults to Vulkan — set Auto to prefer CUDA on NVIDIA;
   **Storage Data Folder** defaults to `TryllStorage` outside `Content/`).
2. Add `UTryllAgentComponent` to an Actor; assign a `UTryllWorkflowAsset`; leave
   `bAutoCreateOnConnect` enabled.
3. Bind `OnAnswerText` and `OnTurnComplete`.
4. `Connect()` on the subsystem at startup.
5. `SendMessage(TEXT("…"))` to drive a turn.

The auto-launched server receives `--mode client-dev`, `--project-name`,
`--project-dir`, `--storage-root`, and `--telemetry-level` (`basic`, `turns`,
or `full` from Project Settings). Set env `TRYLL_SERVER_MODE=tryll-dev` to
force Tryll Dev mode with `--telemetry-level off` (CI / internal developers).
Packaged players launch `client-prod` and do not pass a telemetry level.
`TryllClient.Build.cs` stages `TryllStorage/` file-by-file as NonUFS (Production
models only under `TryllStorage/models/`).

In the editor, the server starts only after the person accepts the Tryll Runtime Licence in
**Window → Tryll → Tryll Welcome**. Until then, a start fails with "Accept the Tryll Runtime
Licence in the Tryll Welcome window first." Commandlets, `-unattended` runs and packaged games are
not gated.

Full walkthrough: <https://docs.tryllengine.com/v0.10/getting-started/first-inference-unreal.md>

## Editor windows

| Window | Menu | What it is |
|---|---|---|
| Tryll Welcome | **Window → Tryll → Tryll Welcome** | Accept the licence, server status, community links |
| Tryll Help & Community | **Window → Tryll → Tryll Help & Community** | Client and server versions, update check, docs, support and community links, logs folder |
| Tryll Chat | **Window → Tryll → Tryll Chat** | Talk to a scene agent without PIE |
| Tryll Model Manager | **Window → Tryll → Tryll Model Manager** | List, download, load, register models |
| Tryll Dialog Lab | **Window → Tryll → Tryll Dialog Lab** | Replay a script across variants |
| Tryll Agent Log | **Window → Tryll → Tryll Agent Log** | Per-connection send/receive timeline |
| Tryll Memory | **Window → Tryll → Tryll Memory** | Server-wide RAM/VRAM snapshot (Refresh only) |
| Tryll Knowledge | **Window → Tryll → Tryll Knowledge** | Session mutable KBs (manual Refresh, never polls): stores, Filter/Query, write, delete, clear, TMKB export/import |

## Where to look for what

| Question | Page |
|---|---|
| What does this term mean? | [glossary](https://docs.tryllengine.com/v0.10/reference/glossary.md) |
| Which node do I need? | [nodes index](https://docs.tryllengine.com/v0.10/reference/nodes/index.md) |
| What params does node X take? | `reference/nodes/<node>.md` |
| Every field on agent creation | [agent-parameters](https://docs.tryllengine.com/v0.10/reference/agent-parameters.md) |
| What does this error code mean? | [error-codes](https://docs.tryllengine.com/v0.10/reference/error-codes.md) |
| Simplest working graph | [build a chat agent](https://docs.tryllengine.com/v0.10/how-to/build-chat-agent-with-graph.md) |
| Answer from my own data (RAG) | [create a RAG assistant](https://docs.tryllengine.com/v0.10/how-to/create-simple-rag-assistant.md) |
| Give an NPC a voice | [add voice output](https://docs.tryllengine.com/v0.10/how-to/add-voice-to-an-agent.md) |
| Let the model call game code | [tool calls](https://docs.tryllengine.com/v0.10/how-to/define-and-handle-tool-calls.md) |
| Test tools in Chat / Dialog Lab | [test tool calls in the editor](https://docs.tryllengine.com/v0.10/how-to/test-tool-calls-in-editor.md) |
| Which model for tool calls | [compare tool-call models](https://docs.tryllengine.com/v0.10/how-to/compare-tool-call-models.md) |
| Shape a tool schema / prompt | [design a tool-call prompt](https://docs.tryllengine.com/v0.10/how-to/design-a-tool-call-prompt.md) |
| Feed game state into prompts | [drive prompts from game state](https://docs.tryllengine.com/v0.10/how-to/drive-prompts-and-retrieval-from-game-state.md) |
| Localize a prompt as a Text ID | [localize prompts](https://docs.tryllengine.com/v0.10/how-to/localize-prompts.md) |
| Force JSON / structured output | [constrain with a grammar](https://docs.tryllengine.com/v0.10/how-to/constrain-output-with-a-grammar.md) |
| RAM / VRAM cost | [estimate memory footprint](https://docs.tryllengine.com/v0.10/how-to/estimate-memory-footprint.md) |
| Live server RAM / VRAM | [inspect server memory](https://docs.tryllengine.com/v0.10/how-to/inspect-server-memory.md) |
| Ship storage files in a build | [ship storage folders](https://docs.tryllengine.com/v0.10/how-to/ship-storage-folders-for-builds.md) |
| Unreal C++ API details | [Unreal C++ API](https://docs.tryllengine.com/v0.10/reference/client-api/unreal/cpp/index.md) |
| Blueprint nodes | [Blueprint catalog](https://docs.tryllengine.com/v0.10/reference/client-api/unreal/blueprint-catalog.md) |

## Do not

- **Do not edit anything under `Source/TryllClient/Public/Generated/`.** It is generated
  from the wire schema and will be overwritten. Node params, the params factory, and
  builder methods live there.
- **Do not edit `Binaries/ThirdParty/TryllServer/`.** That is the bundled server binary
  and its data.
- Do not edit `Source/ThirdParty/` (vendored FlatBuffers and shared headers).
- Do not invent node types, param names, or exit names. Check the node reference page.
- Do not assume a model name is valid. List the catalog or check the Model Manager.
