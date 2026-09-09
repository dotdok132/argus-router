# Argus Token Router (Qt6 / C++)

A minimalist, high-performance C++ Qt6 GUI application and OpenAI-compatible local proxy server for AI API key pool routing, rate limit prevention, and dynamic model discovery.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Qt6](https://img.shields.io/badge/Qt-6.11-green.svg)
![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)

## Features

- **OpenAI-Compatible Local Proxy Server**: Embedded `QTcpServer` listening on `http://127.0.0.1:8080` handling `/v1/chat/completions` and `/v1/models`.
- **Key Pool Management**: Add, toggle, delete, and test keys for Google Gemini, Groq, OpenRouter, Anthropic, and custom OpenAI-compatible providers.
- **Duplicate Key Detection & Warning**: Real-time identification of duplicate API keys with live amber warning banners and table row highlighting.
- **Dynamic Gemini Discovery & Model Ranking**: Auto-queries Google's `ModelService.ListModels` API, filtering text models and ranking stable Flash/Pro versions dynamically (adapted from LarpHelper).
- **Universal Model Rewriting**: Automatically maps generic model names (`default`, `custom/default`, `auto`) to valid provider model IDs (`gemini-3.6-flash`, `meta-llama/llama-3.3-70b-instruct`, etc.).
- **Anti-Spam / Rate Limit Fuse**: 3-second debouncing safety guard per key to prevent API rate-limiting or IP bans during rapid manual pings.
- **Minimalist Industrial IDE Theme**: Dark mode design system inspired by VS Code and Linear.

## Building from Source

### Prerequisites
- CMake 3.16+
- C++20 compliant compiler (GCC 12+, Clang 15+, MSVC 2022)
- Qt 6.5+ (`Qt6Core`, `Qt6Widgets`, `Qt6Network`)

### Build Steps

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running

```bash
./qt-token-router
```

The application starts the local proxy server on `http://127.0.0.1:8080`.

## Connecting CLI Tools & Clients

Configure your CLI tools (such as `larp`, `cursor`, `cline`, or OpenAI Python SDK) to use the local proxy:

```bash
export OPENAI_API_BASE="http://127.0.0.1:8080/v1"
```

Example curl request:

```bash
curl http://127.0.0.1:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model": "default", "messages": [{"role": "user", "content": "Hello!"}]}'
```

## License

MIT License.
