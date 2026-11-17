# T4k Arch CMake 4.5 validation status

The libvirt `arch` guest was started for the isolated check. Its DHCP address
was `192.168.122.100` with MAC `52:54:00:a7:c4:21`. A scoped TOFU SSH host key
was accepted into `/tmp/p4-arch-known_hosts` after confirming the libvirt
address and that there was no conflicting known-host entry. The ED25519
fingerprint was `SHA256:Fh4+8afdqdTPKDMiETKkBdE33MXaJ1POC7Pny5hW/b4`.

No SSH login key or account route was available: attempts with `arch` and
`ruizhi` returned `Permission denied (publickey,password)`. No guest build,
source copy, install or system change was performed. The prior T4h CMake 4.5
install-preparation run predates T4k and does not validate this change.

Status: pending genuine CMake 4.5 validation. Once guest access is supplied,
use fresh `/tmp` source/build/install paths, Unix Makefiles, clean
`install_prep/all`, and a staged install; verify all four warnings disappear
and each of the four typeinfo/plugin pairs exists under both installed QML
roots.
