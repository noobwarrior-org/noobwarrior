# Permission
Things a plugin could be allowed to do. noobWarrior does not check these yet: the enum exists so that plugin code can refer to the names before permission checks are added.

| Name | Value | Description |
| --- | --- | --- |
| `AccessAllPluginDataUrl` | 0 | Read other plugins' data folders, not only its own. |
| `AccessAllPluginUrl` | 1 | Read other plugins' files, not only its own. |
| `AccessDbUrl` | 2 | Use `db://` URLs. |
| `AccessLocalFile` | 3 | Read files anywhere on the user's computer. Dangerous. |
| `NetServer` | 4 | Create HTTP servers. |
| `NetClient` | 5 | Make network requests. |
| `OsShell` | 6 | Run operating system shell commands. Dangerous. |
| `NoobShell` | 7 | Run noobWarrior's own shell commands. |
| `ScreenRecord` | 8 | Take screenshots of the desktop or a program. |
| `AudioRecord` | 9 | Record audio from the desktop or a program. |
