# Development

```
npm install
npm test
```

`npm install` compiles the addon, including the vendored Lua and LuaFileSystem sources under
`vendor/`. `npm test` runs the suite with Node's built-in test runner.

To rebuild without reinstalling:

```
npx node-gyp rebuild
```

## Layout

| Path | Contents |
|---|---|
| `src/` | The Node-API binding. This is the code to change. |
| `vendor/lua/` | Lua 5.1.5, a verbatim upstream copy |
| `vendor/lfs/` | LuaFileSystem 1.8.0, a verbatim upstream copy |
| `test/` | Test suite and fixtures |
| `examples/` | Runnable examples, each with its own `index.js` |
| `binding.gyp` | Build definition for all platforms |

`vendor/` is deliberately unmodified. Keeping it a straight copy means upgrading Lua is a file copy
rather than a merge, so please do not patch it — if something in there needs fixing, fix it
upstream or work around it in `src/`.

## Continuous integration

Every pull request builds and tests on Linux, macOS and Windows against Node 20, 22 and 24, and
runs CodeQL over both the C++ and the JavaScript. All of those must pass before a merge.

CodeQL raises alerts inside `vendor/` because its path filters do not apply to compiled languages.
Findings there belong upstream; dismiss them rather than patching the vendored sources.

## Releasing

Commits follow [Conventional Commits](https://www.conventionalcommits.org/), and the **pull request
title** is what matters, since it becomes the squashed commit message. A title that does not parse
means a release that silently does not happen, so it is validated on every pull request.

release-please keeps an open `chore(master): release x.y.z` pull request that accumulates merged
changes, works out the next version and rewrites `CHANGELOG.md`. Merging that pull request tags the
commit and publishes a GitHub Release, which is what triggers the npm publish.

So merging the release pull request is the single deliberate act that ships a version — nothing
publishes on an ordinary merge to `master`.

Publishing uses [npm trusted publishing](https://docs.npmjs.com/trusted-publishers) over OIDC, so
there is no npm token stored in this repository, and published versions carry provenance linking
them to the commit and workflow run that built them.
