# Argus Token Router

A high-performance, minimalist C++20 / Qt6 GUI application and local OpenAI-compatible API proxy daemon. Designed for multi-key pool failover, auto rate-limit calibration, session token tracking, and transparent markdown-based long-term memory execution.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Qt6](https://img.shields.io/badge/Qt-6.5%2B-green.svg)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-orange.svg)
![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)

---

## Overview

**Argus Token Router** acts as a intelligent local API gateway running on `http://127.0.0.1:8080`. It intercepts OpenAI-compatible API calls, routes them across a pool of API keys (Google Gemini, Groq, OpenRouter, Anthropic, OpenAI), balances traffic, handles failovers, and transparently executes memory lookup tools against local `.md` files without exposing JSON tool calls to client applications.

```
+-------------------------------------------------------------------------+
|                              CLIENT APIS                                |
|          (larp, Cursor, Cline, Python OpenAI SDK, LangChain, curl)      |
+------------------------------------+------------------------------------+
                                     |  HTTP POST /v1/chat/completions
                                     v
+-------------------------------------------------------------------------+
|                         ARGUS TOKEN ROUTER                              |
|                       (http://127.0.0.1:8080)                           |
|                                                                         |
|  +-----------------------+  +-------------------+  +-----------------+  |
|  | Multi-Key Failover    |  | Rate-Limit Fuse   |  | Session Token   |  |
|  | & Round-Robin Pool    |  | & Header Parsing  |  | Usage Metrics   |  |
|  +-----------+-----------+  +---------+---------+  +--------+--------+  |
|              |                        |                     |           |
|              +------------------------+---------------------+           |
|                                       |                                 |
|                                       v                                 |
|  +-------------------------------------------------------------------+  |
|  |                 TRANSPARENT MEMORY ENGINE                         |  |
|  |   Injects `get_memory` & `list_memories` schemas into LLM payload  |  |
|  |   Executes LLM tool calls against local `.memory/*.md` files      |  |
|  |   Returns clean final text to client without raw JSON tool blocks |  |
|  +-------------------------------------------------------------------+  |
+------------------------------------+------------------------------------+
                                     |
                +--------------------+--------------------+
                |                    |                    |
                v                    v                    v
      +-------------------+  +---------------+  +-------------------+
      | Google Gemini API |  | OpenRouter    |  | Groq / Anthropic  |
      +-------------------+  +---------------+  +-------------------+
```

---

## Key Features

- **OpenAI-Compatible Local Endpoint**: Native support for `/v1/chat/completions` and `/v1/models`. Plug-and-play replacement for local LLM tools.
- **Multi-Key Pool & Automatic Failover**: Manage multiple API keys per provider. Seamlessly fails over to the next key upon HTTP `429 Too Many Requests`, `500 Server Error`, or invalid key status.
- **Transparent Long-Term Memory Engine**:
  - Automatically provisions `.memory/` modules (`user_character.md`, `code_requirements.md`, `project_context.md`).
  - Intercepts model `tool_calls` internally, executes file reads, and completes the roundtrip loop (`toolDepth < 3`).
  - The caller client receives a clean, context-aware text response without visible JSON tool call wrappers.
- **Built-in Memory (.md) Editor Tab**: Integrated IDE-style Markdown editor within the Qt GUI for real-time inspection and editing of memory modules.
- **Token Usage & Rate-Limit Tracking**:
  - Live session and total token counts (`totalTokensUsed`) tracked per key.
  - Automatic HTTP response header parsing (`x-ratelimit-*`, `ratelimit-*`) for dynamic request limit calibration.
- **Queue & Load Balancing Strategies**:
  - **Sequential Priority**: Highest-priority keys evaluated first.
  - **Round Robin**: Distribute requests evenly across active keys.
  - **Least Loaded**: Direct traffic to keys with highest remaining capacity.
- **Duplicate Key Detection & Warning**: Identifies duplicate keys with amber warning banners and table row highlighting.
- **Minimalist Industrial IDE Theme**: Dark mode palette inspired by VS Code and Linear with strict zero-emoji visual guidelines.

---

## Installation

### One-Line Automated Installer

Run the following command in your terminal to automatically clone, build, and install the latest version under the binary name `argus-router`:

```bash
curl -sL https://raw.githubusercontent.com/dotdok132/argus-router/main/install.sh | bash
```

---

## Building from Source

### Prerequisites

- **CMake**: 3.16 or newer
- **C++ Compiler**: Modern C++20 compiler (GCC 12+, Clang 15+, or MSVC 2022)
- **Qt6 Framework**: `Qt6Core`, `Qt6Widgets`, `Qt6Network`

#### Installation of Dependencies (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libqt6network6
```

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/dotdok132/argus-router.git
cd argus-router

# Configure and compile
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Quick Start & Running

Launch the compiled executable:

```bash
./build/qt-token-router
```

The application opens the Qt6 GUI dashboard and starts the HTTP server listening on `http://127.0.0.1:8080`.

### Connecting Clients

Configure your environment variable or client application to point to the local proxy:

```bash
export OPENAI_API_BASE="http://127.0.0.1:8080/v1"
```

#### Example cURL Request

```bash
curl http://127.0.0.1:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "auto",
    "messages": [
      {"role": "user", "content": "Check memory files and summarize my coding guidelines."}
    ]
  }'
```

#### Integration with Larp CLI

```bash
larp why "What are my code requirements saved in memory?"
```

---

## Directory & Configuration Structure

- **Key Configuration**: `~/.config/ArgusAI/QtTokenRouter/keys.json`
- **Memory Library**: `.memory/`
  - `user_character.md`: User personality, preferences, and identity facts.
  - `code_requirements.md`: Project coding rules, architectural constraints, and stack guidelines.
  - `project_context.md`: Project summary, module documentation, and active task status.

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.
