# Authentication
Authentication decides who a player is when they join a server. All of it is off unless `emu.auth.enabled` is set. With it off, every joiner is the local `user.*` identity from the registry and the join ticket is the placeholder `"1"`.

The shared code is in `HttpServer/Emulator/AuthUtil.h` and `.cpp`: password hashing, cookie parsing, sessions, join tickets, Ed25519 signing and permission checks. New authentication code should extend it instead of repeating any of that in a handler.

# Accounts and sessions
Accounts are rows in the master database's `User` table. Passwords are hashed with Argon2id and a random 16 byte salt, and `VerifyPassword` compares in constant time. The Luau `hash` library uses the same settings, so a plugin can check the same hashes.

Logging in through `/v1/login` or `/emu/v1/login` creates a row in `LoginSession` and sets the `.LOGINSESSION` cookie to its token. `ResolveSessionUser()` turns a token back into a `SessionUser` (id, name, display name, rank, and whether the user is a guest or federated). It also updates the session's last-used time. A session unused for longer than `emu.auth.session_ttl_days` (30 by default) counts as expired, and `0` turns expiry off.

Sessions normally live in the master database. With `emu.auth.allow_accounts_from_all_mounted_databases` on, an account can come from any mounted database, and the session is created in the database that held it. Use the `EmuDbManager` overload of `ResolveSessionUser`, which finds the right database, anywhere that can happen. That setting lets anyone whose database you mount log in to your server, so it is off by default.

`emu.auth.password_based` and `emu.auth.allow_registration` control whether password logins and new sign-ups are accepted.

# Joining a game
Each join is checked twice: once by the emulator when the client asks to join, and again by the game server when the client connects to it.

## The join request
Modern clients ask `GameJoinHandler`, and 2021 clients ask `PlaceLauncherHandler` followed by `JoinScriptJsonHandler`. With authentication on, the handler finds the joiner with `ServerEmulator::ResolveJoiningUser()`, in this order:

1. The identity from a launch ticket the local client just redeemed. A client launched from the desktop app has no cookie, so `LaunchEngine` gives it a ticket on its command line (see [Engines](/internals/engines)).
2. A `.LOGINSESSION` cookie holding a federated voucher, which is verified with the master server.
3. A `.LOGINSESSION` cookie holding an ordinary session.

If nobody is found, the joiner becomes a guest when `emu.auth.allow_guests` is on. Otherwise the join is refused with status `22`, which the client shows as an authentication error with code 400. The client doesn't display the server's message.

A found identity is written into the join script's `UserId`, `UserName` and `DisplayName`, and a ticket goes into `ClientTicket` (or `authenticationTicket`).

## Tickets
What the ticket holds depends on who is joining:

| Joiner | Ticket |
| --- | --- |
| Local account | A random single-use ticket from `MintAuthTicket()`, stored in the master database's `AuthTicket` table. |
| Guest | The identity itself, encoded by `EncodeGuestTicket()`. Guests get a negative user id and have no database row. |
| Federated user | The identity itself, encoded by `EncodeFederatedTicket()`. |

Guests and federated users carry their identity inside the ticket because `AuthTicket.UserId` is a foreign key to `User`, and they have no row there.

## Redeeming
The game server confirms the player by posting the ticket to `/v1/authentication-ticket/redeem` (`AuthTicketRedeemHandler`). A stored ticket can be redeemed once, within `emu.auth.ticket_ttl` seconds of being minted. Guest and federated tickets are decoded instead of looked up.

# Federation
Servers can be run in two modes, set by `emu.auth.type`:

| Mode | Who checks identities |
| --- | --- |
| `master` | This server, using its own accounts. The default. |
| `slave` | The master server at `emu.auth.master`. |

A master server is the `master-server` plugin. Master servers can trust each other's users, which is called federation.

A player from another master joins with a voucher instead of a session. Their home master mints it at `/v1/join/mint-voucher` for the target server's master, and it travels in the `.LOGINSESSION` cookie in this form:

```
fedvoucher.<identity>.<actionId>.<body>.<signature>
```

The identity and body are URL-safe base64, and the signature is an Ed25519 signature in hex. A slave server can't check the signature itself, so `ResolveFederatedVoucher()` sends the pieces to its master's `/v1/join/verify-federated`, along with `emu.auth.federated_login`. That setting decides whether users of other masters may join, or only the master's own. The master answers with the user if it accepts the voucher.

A voucher with four parts is an old unsigned one. It is still forwarded so the master can reject it with a clear reason.

When the voucher rides a local launch instead of a cookie (joining a server on the same machine), `LaunchEngine` passes it as the launch ticket, and `AuthTicketRedeemHandler` verifies it the same way.

# Ranks and permissions
Each account has a rank from 0 to 255. Rank 0 is anyone without an account, guests included. `emu.roles` names the ranks a server uses, and `emu.ranks.default_rank` is the rank new accounts get.

A permission is a registry key under `emu.permissions` holding the lowest rank allowed to use it. Code asks for a permission by name, never a rank number, so renumbering ranks needs no code change. `HasPermission()` applies three rules:

- The user in `emu.ranks.owner_user_id` always counts as rank 255. This is the way back in after a bad rank edit.
- Rank 255 has every permission, even if the permissions table is broken.
- A permission with no registry key is denied, so a typo fails closed.

Luau pages use the same checks through [core.HasPermission](/api/luau/libraries/core/HasPermission).
