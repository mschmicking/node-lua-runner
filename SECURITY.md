# Security Policy

## Supported versions

| Version | Supported |
| ------- | --------- |
| 2.x     | Yes       |
| 1.x     | No        |

1.x does not build on current Node.js and carries several memory-safety defects that
2.0.0 fixed, including a fixed-size buffer that arbitrary-length Lua error messages
were formatted into. Please upgrade rather than asking for a backport.

## Reporting a vulnerability

Please report privately through GitHub's
[security advisory form](https://github.com/mschmicking/node-lua-runner/security/advisories/new)
rather than opening a public issue.

Include what you need to reproduce it: the Lua and JavaScript involved, your platform
and Node.js version, and what you observed.

If that form is not available to you, open an issue asking for a private channel —
without the details — rather than posting them publicly.

You should get an acknowledgement within a week or so. This is a spare-time project,
so please treat that as a good-faith aim and not a guarantee.

## Scope

This package embeds a Lua interpreter in your Node.js process, which shapes what
counts as a vulnerability here.

**In scope** — anything in `src/` that lets *ordinary* use go wrong: memory
corruption, use-after-free, or a process crash reachable from normal API calls or
from Lua code with no unusual privileges.

**Not in scope:**

- **Running untrusted Lua.** This library gives Lua scripts the full standard
  library, including `os.execute`, `io.open` and `require`, plus LuaFileSystem. A Lua
  script can therefore run commands and read and write files with the privileges of
  your Node.js process. That is what embedding Lua means; it is not a defect in this
  package. Do not feed it code you would not run yourself.
- **Misusing the low-level stack API.** This is a thin wrapper over the Lua C API and
  does not shield you from every misuse of it. Some operations on values of an
  unexpected type raise an *unprotected* Lua error, which aborts the process rather
  than throwing. `SetField` and `GetField` guard against this; other methods do not.
  A crash reached that way is documented behaviour, not a vulnerability.
- **Findings in `vendor/`.** Lua 5.1.5 and LuaFileSystem are vendored verbatim and are
  not patched here. Report those upstream. If something in them is genuinely
  exploitable through this package's API, do report it here as well.
