# Working on this repository

Guidance for AI agents and new contributors. Read this before changing anything.

## What this is

A Node.js native addon embedding Lua 5.1. The binding in `src/` is C++ against
[Node-API](https://nodejs.org/api/n-api.html); Lua 5.1.5 and LuaFileSystem are vendored under
`vendor/` and compiled in. There is no system Lua dependency and no prebuilt binary — every
install compiles from source.

## Build and test

```
npm install          # compiles the addon (runs node-gyp via the install script)
npm test             # node --test
npx node-gyp rebuild # rebuild without reinstalling
```

After changing anything in `src/`, rebuild before testing — `npm test` runs against the last
compiled `build/Release/nodelua.node` and will happily pass on stale output.

Also run the examples; they exercise paths the tests do not:

```
node examples/simple/index.js
node examples/lua_require/index.js
node examples/lua_lfs/index.js
```

## Layout

| Path | Notes |
|---|---|
| `src/` | The binding. This is what you change. |
| `vendor/lua/` | Lua 5.1.5, verbatim upstream — **do not modify** |
| `vendor/lfs/` | LuaFileSystem 1.8.0, verbatim upstream — **do not modify** |
| `test/` | Suite and fixtures |
| `docs/` | Reference documentation |
| `binding.gyp` | Build definition, all platforms |

`vendor/` is deliberately an unmodified copy so upgrading Lua stays a file copy rather than a
merge. If something in there needs fixing, fix it upstream or work around it in `src/`.

## Conventions in `src/`

- **Tabs for indentation**, matching the existing files.
- **Argument validation goes through `CheckArgs`**, not hand-written checks:
  ```cpp
  Napi::Value LuaState::SetField(const Napi::CallbackInfo& info) {
      Napi::Env env = info.Env();
      if(!CheckArgs(info, "SetField", {Arg::Number, Arg::String, Arg::Any}) || !EnsureOpen(env)){
          return env.Undefined();
      }
      ...
  ```
  It generates the error messages, so the wording stays consistent. The two exceptions are
  `Pop` and `SetTop`, whose argument is optional.
- **Errors from Lua go through `ThrowLuaError`**, which builds the message, **pops the error**
  and throws. Popping by hand at the call site is how the Lua stack silently grows.
- **Every method that touches `lua_` must call `EnsureOpen` first.** Without it, use after
  `Close()` is a use-after-free rather than an exception.
- **Resolve relative stack indices with `abs_index` before pushing anything.** Pushing shifts
  every negative index by one. This was a real bug: `SetField(-1, ...)` ended up assigning into
  the value it had just pushed.
- **Never let an unprotected Lua error reach the C API.** It aborts the whole process instead of
  throwing. Guard the type first, as `SetField` and `GetField` do with `EnsureIndexable`.
- **Do not throw C++ exceptions through Lua frames.** The addon builds with
  `NAPI_DISABLE_CPP_EXCEPTIONS` because Lua unwinds with `longjmp`, which would skip C++
  destructors. Use `ThrowAsJavaScriptException()` and return.

## Tests

`node:test` and `node:assert`, no test framework dependency — keep it that way.

Every bug fix gets a regression test that fails before the fix. The suite already pins the
historic ones (argument order, boolean conversion, load-without-executing); if you change
behaviour, update the corresponding test deliberately rather than making it pass.

Error message wording is asserted in tests. Changing it is a breaking change for anyone
matching on it.

## Branches and pull requests

- **Branch from the default branch.** Never base a branch on another open PR's branch — merges
  here are squash-only, so a squashed parent leaves duplicated commits and guaranteed conflicts.
- **PR titles must be Conventional Commits.** The title becomes the squash commit and the release
  version is derived from it, so a title that does not parse means a release that silently does
  not happen. Allowed scopes are listed in `.github/workflows/pr-title.yml`.
- `feat` → minor, `fix`/`perf`/`refactor` → patch, `feat!` or a `BREAKING CHANGE` footer → major.
- All checks must pass, and the branch must be up to date with the default branch. If GitHub says
  the branch is behind, that is not a conflict — use "Update branch".

## CI

Every PR builds and tests on Linux, macOS and Windows across three Node majors, and runs CodeQL
over the C++ and the JavaScript.

CodeQL raises alerts inside `vendor/` because its path filters do not apply to compiled
languages. Those belong upstream — dismiss them rather than patching the vendored sources.

When reading CodeQL results, check an analysis on the **default branch**. Pull-request analyses
only report alerts new relative to the base, so `results=0` on a PR does not mean the codebase is
clean.

## Releases

Do not bump the version or tag by hand. release-please maintains a release PR from the merged
commit history; merging it tags the commit, publishes a GitHub Release, and that event publishes
to npm over OIDC. See [docs/development.md](docs/development.md).

Anything that ships must be in the `files` allowlist in `package.json`. The release workflow
asserts the tarball contains all 29 vendored Lua sources — a tarball missing them is unbuildable
for everyone who installs it, and npm versions are immutable.

## Deliberately out of scope

Do not add these without discussion; each was considered and declined:

- **Prebuilt binaries.** Users need a C++ toolchain, as they always have.
- **Async execution.** The API is synchronous throughout.
- **Thread support.** One `LuaState` per thread is not supported, and Lua's `os.date` uses
  non-reentrant C library calls.
- **Patching `vendor/`.** See above.
