# App-owned U-AVF Bridge preparation

## Proven this checkpoint

- Shared `BridgeEndpoint` owns bounded framing, duplicates a caller-approved
  descriptor for input/output, applies10s socket timeouts and reuses Core/Bridge
  JNI. No Activity, decoder or VM creation in this adapter.
- Actual descriptor-path local test on G925:49 commands,10 negative cases,
  16 GPU submissions,557056 words checked,0 SyncVal errors/warnings,
  clean EOF and zero Core resources after disconnect. Same-UID AF_UNIX,
  **not an AVF/Windows execution PASS**.
- Android descriptor adapter is packaged into an optional diagnostic U-AVF APK.
  Production U-AVF package/code/display is not rebuilt or installed by this step.
- Windows ARM64 freestanding client built,7168 bytes, only KERNEL32/WS2_32
  imports. Same-source x64 wire self-test executed on host. No ARM64 guest
  execution or guest GPU/ICD PASS claimed from cross-compilation.

## App-owned admission

Diagnostic source only, existing same-UID authenticated owner Binder:

- `mavk_bridge_enabled=true` explicitly opts into isolated installed-Windows mode.
- `mavk_validation=true` requests the packaged debug layer; default false.
- New authenticated transaction4 / provider `mavk_probe` starts one bounded test
  only after the owner has reported existing WinPE/viosock readiness.
- Port4057 is dedicated to the experiment. Existing generic delegated-port policy
  remains4055 display / optional4054 clipboard; it does not silently admit4057.
- Exact port/readiness/opt-in/64-bit bounds checked by84 Java assertions.
- Owner invokes its existing AVF `connectVsock(4057)` and passes the resulting FD
  to the common endpoint. No shell-owned socket, private system Binder replacement,
  new decoder, driver installation or VM config changes.
- Merely closing one duplicate FD cannot cancel all users: owner uses shutdown
  on destruction, plus per-socket timeouts. Native device-loss teardown still
  depends on driver `vkDeviceWaitIdle`; this is not a guaranteed GPU-hang watchdog.

The optional APK is **built, not installed**. No VM boot was performed here.

## Safe carrier plan (not applied)

Read-only cold inspection corrected an old assumption: the allocated cluster is
8192 bytes, but actual `/IDDWORK/idd-motion-source.exe` file length is7680 and
its current ESP source SHA is7da580…, matching the historical long-workload
delivery report. This is not the installed-root helper's old7f80… identity.
Do not confuse the two or weaken preimage guards.

Client was reduced to7168 bytes by removing redundant file-logging (stdout is
captured by its future launcher); no code/resource truncation. The prepared
carrier zero-pads to the existing7680-byte file and retains the original unused
cluster tail. One8KiB cluster, no FAT/directory/GPT/root/firmware write needed.
Full preimage and candidate stored on healthy E. **No bytes staged to VM disk.**

Authoritative plan: `E:/Mavk/artifacts/windows-bridge-carrier-preflight3-20261015/plan.json`.
Current observed slot: cluster3182 / absolute offset36028416. Revalidate cold
state, firmware/disk identity, current path/chain and whole-cluster preimage
immediately before any apply; never write based only on these stale numbers.

## Artifacts

- `E:/Mavk/artifacts/bridge-descriptor-build1-20261015/descriptor-run1.json`
  raw report SHA03635cb2975105f5a08764adb7f06104b59b7c7cadf793e0104102e547bc7374.
- Same directory Mavk-native-probe.apk SHA
  `1acb493345bcea5f150ce797859e96b40fa9a826806d3352b09a64675c6d24db`.
- `E:/U-AVF/artifacts/mavk-owner-build1-20261015/U-AVF-Windows-diagnostic.apk`
  SHA`61d7828b96212799cc824d5a06c43b80bb144e63f677211c7be30e93edc9cb6f`.
- `E:/Mavk/artifacts/windows-bridge-client-build3-20261015/mavk-bridge-aarch64.exe`
  SHA`99ef3264b1cc5d7917517a946fb0333ce1d084e4704a293eb4ff2b83f718ad9a`.
- Carrier preimage SHA3381bd9574ddecd6a6674d99b5b5be81e506393746c66460082e2bde35f4320d;
  prepared cluster SHA5b7ec1bb0ff6c6c4c6db8f017adf3d1bbe2f5481a8bd62aec7503cd6ce70f580.

## Exact next useful test

1. Preserve the currently installed isolated diagnostic APK before replacement;
   verify VM list remains empty. Do not install over production U-AVF.
2. Prepare a finite guest launcher for the new client, using the existing
   authorized diagnostic job/input mechanism; inspect its current admission and
   rollback rules first. No IddCx/certificate/CPU/codec work is needed for compute.
3. Only then apply the single carrier cluster with exact journal/readback, install
   the optional APK and boot the unchanged proven8CPU/native1200p profile once
   with the Mavk opt-in forwarded. No arbitrary old Pxx/Idd benchmark auto-launch.
4. Start guest client (listen4057), then request the authenticated owner test.
5. Guest expected:23 completed commands,131072 verified uint32,8 GPU jobs,
   `MAVK_WINDOWS_VSOCK_COMPUTE_PASS`. Owner expected: corresponding23 commands,
  8 submissions,525056 sent/1000 received bytes, clean EOF, zero resources and
   zero validation errors. Actual results, not transcript delivery acknowledgments.
6. Restore carrier exact original bytes/readback and leave baseline clean.

This is the first guest command/compute slice, not a Vulkan ICD or Windows graphics
driver. After actual AVF proof, extend toward real rendering/guest API rather than
repeating compute benchmarks. Ubuntu/P7/storage/H.264 unchanged; no commit/push.
