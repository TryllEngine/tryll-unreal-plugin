# Tryll for Unreal Engine

Tryll runs language, speech-to-text and text-to-speech models on the player's own machine, and
connects them to your game through workflow-driven agents. This repository holds the Tryll Unreal
plugins, version 0.10.1.

- **Platform:** Windows x64
- **Docs:** https://docs.tryllengine.com/v0.10/

## The plugins

| Folder | What it does | Needs |
|---|---|---|
| `TryllClient/` | The Tryll client: agents, workflows, speech, the editor windows, and the Tryll server | — |
| `TryllMetaHumanLipSync/` | Optional. Drives MetaHuman faces from Tryll speech | `TryllClient` |
| `TryllMCP/` | Optional. Lets an AI coding agent create and check Tryll workflows through the editor's MCP server | `TryllClient`, UE 5.8 |

Prebuilt zips per Unreal Engine version:

| Plugin | Unreal Engine |
|---|---|
| `TryllClient` | 5.7, 5.8 |
| `TryllMCP` | 5.8 |
| `TryllMetaHumanLipSync` | 5.7, 5.8 |

## Install from a release zip

This is the easy way. It works in Blueprint-only projects.

1. Open [Releases](https://github.com/TryllEngine/tryll-unreal-plugin/releases) and download the zips for your Unreal Engine version:
   `TryllClient-0.10.1-UE<version>-Win64.zip`, plus any optional plugin.
2. Unzip each one into your project's `Plugins/` folder, so you get `Plugins/TryllClient/`,
   `Plugins/TryllMetaHumanLipSync/` and so on.
3. Open the project. The **Tryll Welcome** window opens. Accept the licence there; the editor starts
   the Tryll server only after you accept.

The `TryllClient` zip includes the Tryll server, so there is nothing else to download. To package a
game, install Visual Studio 2022 with the "Game development with C++" workload: packaging compiles
the plugins for your game.

## Install from source

Use this when you want to change the plugin code. Your project needs C++ (a code project), because
Unreal compiles the plugins.

1. Copy the plugin folders from this repository into your project's `Plugins/` folder.
2. From the same release, download `tryll-server-0.10.1-win64-full.zip` (CUDA and Vulkan) or
   `tryll-server-0.10.1-win64-lite.zip` (CPU and Vulkan), and unzip it into
   `Plugins/TryllClient/Binaries/ThirdParty/TryllServer/Default/`.
3. Open the project and let Unreal build the plugins.

Getting started: https://docs.tryllengine.com/v0.10/getting-started/first-inference-unreal.html

## Help and community

**Window > Tryll > Tryll Help & Community** in the editor has links to the documentation and the community.
Found a bug? See https://docs.tryllengine.com/v0.10/how-to/report-a-bug.html, or open an issue at https://github.com/TryllEngine/tryll-unreal-plugin/issues.

## How this repository is made

This repository is generated from Tryll's main repository by the release pipeline. Every release is
one commit and one tag. Changes made here directly are overwritten, so please report issues instead
of opening pull requests.

## Licence

See [LICENSE.md](LICENSE.md), also at https://docs.tryllengine.com/v0.10/reference/legal/licence.html. Each plugin folder carries the
same file.
