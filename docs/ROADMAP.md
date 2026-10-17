# Mavk — roadmap / acceptance gates

Current user-directed execution order: [ACTIVE_PLAN](ACTIVE_PLAN.md).
DroidDeck/DXVK first; then Windows IddCx and product workspace,then advanced
Windows GPU/WDDM. Private Mavk uploads authorized; U-AVF/public releases not.
Null-resource guards have a bounded native buffer proof,NOT full feature support;
see compatibility/NULL_RESOURCES.md and STATE.md before enabling any capability.

2026-10-17 Compatibility priority: full pinned Turnip750/Mali inventory and
DXVK admission audit complete (TURNIP_DXVK_CAPABILITIES_2026-10-17.md).
Common BC shadow/mip/dirty-copy storage +retained native partial upload+sRGB
bounded PASS6144pixels,SyncVal0/0. Correctness path is synchronous; no async
streaming/complete Vulkan image or DXVK gate claimed. See BC_TEXTURE_LIFECYCLE.md.
BC4/5 signed+unsigned and BC6H/7 bounded native sampling slices now PASS;
BC6H/7 independent Microsoft WARP oracle589824 components also PASS. Codecs
alone do not close Vulkan compressed-image API or DXVK gates. Next integrate
upload/mip/copy/view lifecycle+sRGB and explicit null-resource semantics.
Shared CPU BC1/2/3 decode +existing native Core upload/hardware sampling PASS:
16384 pixels,16 submits,SyncVal0/0. This is EMULATED decode only, not full BC
feature/ICD/DXVK support. Sarek b665309 reduces admission gaps to BC+nullDescriptor
but drops clip/cull semantics; don't mislabel as implementation. Next bounded
BC formats and null-resource correctness; preserve active Windows ICD checkpoint.

2026-10-17 development order update: native Feature Suite is the primary fast
gate (FEATURE_SUITE.md). Reuse existing Core workloads; do not boot Windows for
each Vulkan addition. Bridge/ICD changes and substantial shared updates still
require periodic Windows gates. Keep ICD active; WDDM separate from Core.

Решение пользователя от 2026-10-15. Приоритет меняется с дальнейшего выжимания
Windows capture FPS на аппаратное GPU-исполнение. Старые Windows цели не
завершены и не отменены; Mavk становится их первым GPU-зависимым этапом.

## Архитектурные границы

Общее ядро: Android Vulkan loader/vendor device → native backend → bounded
resource ownership, synchronization, capability/compatibility, diagnostics.

Потребители имеют отдельные адаптеры:

- DroidDeck: локальный ABI/runtime adapter (glibc/Bionic/Wine); без vsock.
- U-AVF: отдельный guest/host command bridge; Windows и Ubuntu ICD.
- Windows WDDM/D3D/DWM: отдельная исследовательская реализация, не побочный
  эффект наличия Vulkan ICD или виртуального IddCx-монитора.

Не закладывать DMA-BUF/shared memory/zero-copy как доказанную возможность AVF.
Не сериализовать весь Vulkan заранее. Нужный общий код выделять из рабочих
вертикальных срезов. Перенос API через vsock обязан учитывать memory semantics,
object lifetime, fences/semaphores, ordering, errors, teardown и backpressure.

## M0 — Native feasibility (первый обязательный этап)

- [x] Отдельный app-sandbox harness, без изменений production U-AVF APK/VM.
- [ ] Реальный device/vendor/driver/API, queue families, extensions, feature
      chains, memory types, limits; подтверждение отсутствия CPU renderer.
      App hardware/API/queues/memory/152 extensions verified. Fresh GPU-service
      full core1.0..1.3 features/limits/format inventory saved2026-10-17, matching
      app identity/extension set; remaining unqueried structs/usage combos UNKNOWN.
- [x] Graphics workload с движением в Surface + compute с проверкой результата.
      Direct graphics3330 pixels PASS; SurfaceFlinger59.9208 timestamps/s за28с.
- [ ] Выделение/освобождение buffers/images, upload/readback, queue submission,
      synchronization и recreate Surface. Не требовать unsupported extensions.
      Bounded Core gate PASS: app-local SyncVal + compute/image ABBA +2 active
      Surface losses/recreate,0 actual errors/warnings; broader stress remains.
- [ ] Сравнить direct Android Vulkan и тот же workload через минимальный Mavk
      слой: одинаковые device, workload, resolution, build и тепловые условия.
      Compute/image ABBA в3 процессах PASS; graphics Core/direct59.956/59.967
      timestamps/s при1920×1200;3330 pixels совпали с эталоном в каждом пути.
      Lifecycle/validation и повторные graphics measurements остаются gate.
- [ ] Сохранить сырые samples, p50/p95/p99 CPU/frame times, GPU timestamps
      только если поддерживаются; память, ошибки и длительность прогона.

PASS: аппаратный workload на G925 воспроизводится; корректность и управление
ресурсами проверены; overhead измерен. Наличие Vulkan device/треугольника не
означает игровую совместимость, полноценный backend или guest acceleration.

## M1 — Минимальное общее ядро

- [x] Явная таблица NATIVE / EMULATED / UNSUPPORTED реализованных срезов:
      Feature Suite on Android; verification status separate, missing/unsupported
      never advertised as PASS. Not a complete physical Vulkan capability list.
- [ ] Ресурсы и синхронизация с определённым ownership и детерминированным teardown.
- [ ] Минимальные shaders/pipelines и тесты корректности; compatibility work
      только по конкретным reproducer, без ложного advertising features.
- [ ] Диагностика и repeatable performance harness. Никакой облачной отправки
      измерений или добавления телеметрии без отдельного решения.

## M2 — Windows: главный потребитель

Предварительный Bridge gate: bounded command/resource/compute протокол и
same-UID Android stream test перед AVF integration. Не называть local proof
готовым guest bridge или Vulkan ICD.

- [x] Минимальный compute guest/host bridge поверх app-owned AVF: Windows ARM64
      →8 hardware Mali jobs→131072 verified words,23 commands,zero validation
      errors/warnings,clean EOF/live zero. Это readback/copy path, не shared memory.
- [x] Graphics Bridge: Windows→Mali graphics pipeline→3330 guest-verified pixels,
      process EXIT0,zero validation errors/warnings,exact carrier rollback.
- [x] Вертикальный Vulkan ICD slice: guest app → команды → host Mali →
      проверяемый результат. Установить точный поддерживаемый поднабор API.
      ARM64 DLL/API consumer build PASS; HOST x64 loader manifest/entry PASS.
      Guest ARM64 standard loader/device/offscreen render PASS:3330 blue pixels,
      EXIT0,one hardware submission,zero validation errors/warnings,clean teardown.
      Limited non-conformant API-only consumer; не third-party game или WSI PASS.
- [ ] Guest/host costs: serialization, upload/readback, sync, GPU, presentation.
- [x] Реальный Vulkan workload в Windows, не software fallback/подменённый device name.
      Unmodified pinned SaschaWillems renderheadless1024²:standard loader/ICD,
      three indexed depth-tested triangles,EXIT0,3624 analytic pixel samples,
      real Mali submission,zero validation errors/warnings. Offscreen only;
      windowed presentation,games,performance/conformance remain unproven.
      Follow-up: own standard Win32 WSI consumer passed12 distinct moving
      images320×240 and was visible on tablet through existing H264. Guest CPU
      blit presentation only; third-party WSI,performance and DWM remain open.
- [x] Standard vertex uniform-buffer slice: set0/binding0,192B matrices,
      offset32 and late mapped writes; Windows verified3906 pixels on Mali,
      EXIT0,SyncVal0errors/0warnings. Not textures/general descriptor support.
- [x] Bounded standard compute ICD: actual app SPIR-V,one SSBO/1D dispatch,
      two Mali jobs,8192 verified words and descriptor guards,EXIT0,SyncVal0/0.
      This is not general compute or Vulkan conformance.
- [x] External window application: unchanged pinned upstream triangle,24 real
      Mali frames per run in IMMEDIATE/FIFO,guest FIFO EXIT0,SyncVal0/0.
      Correct RGB window observed on tablet through unchanged H264. CPU readback/
      blit WSI; NOT general WSI,visible-FPS,game or DWM acceleration proof.
- [x] Next application compatibility slice: sampled texture/image upload and
      sampler descriptors, driven by concrete unchanged upstream app requirements.
      Native Core4mip/nearest+linear/trilinear12288pixels PASS; local Bridge full
      1024²/11mip chain5592404B chunked upload retained for2 draws/2048pixels,
      3GPU/16rejections/SyncVal0/0 PASS. Standard Windows texture resource CPU
      resource/descriptors/standard consumer and unchanged external texture app
      now PASS:two24frame window runs,original10mip KTX,EXIT0,actual Mali25GPU
      per run,SyncVal0/0,correct image on Android. Not games/full texture API.
- [ ] Отдельный WDDM feasibility gate: поддерживаемая ARM64 driver model,
      signing/admission, memory/scheduling/presentation; безопасный откат.
- [ ] Только после этого аппаратные Direct3D/DWM и A/B системной композиции.

Если поддерживаемый stock-путь WDDM не найден, фиксировать конкретный blocker.
Нельзя заменять цель «ускоренный Windows desktop» лишь работающим guest ICD.
Не менять firmware/SMP/timer/проверенный Windows display ради этого proof.

## M3 — DroidDeck

- [ ] Проверить существующий код/runtime и ABI перед реализацией адаптера.
- [ ] Минимальный локальный Vulkan workload, затем требования конкретной DXVK версии.
- [ ] Реальная игра без root: FPS, p95/p99 frame time, корректность, CPU/GPU,
      память и thermal behavior. Не обещать все игры или паритет с Turnip.

Ветка может идти параллельно Windows после M0/M1, но это порядок разработки,
а не разрешение запускать дополнительных агентов или создавать внешние репозитории.

## M4 — Ubuntu

- [ ] Guest ICD через проверенный bridge, hardware device + Vulkan workload.
- [ ] Реальные приложения, не только vulkaninfo.
- [ ] Регрессия GNOME/VirGL/Mali, display, input, audio, lifecycle.

## Правила решений

Перед каждым этапом — проверяемая гипотеза и минимальный безопасный proof.
После доказательства — переиспользуемый код, не новая серия P-профилей.
Нативный успешный тест не доказывает совместимость guest memory или WDDM.
Неподдерживаемые features не «включать» флагами без реализации семантики.
Низкий FPS в Windows отделять от capture/transport FPS: быстрый вывод не
создаёт GPU-ускорения композиции или приложений.
