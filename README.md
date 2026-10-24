# Mavk

Latest: [source-built DroidDeck APK and Muck pacing evidence](docs/SOURCE_APK_AND_MUCK_PACING_2026-10-25.md).
Developer prototype: [AHB/input results and remaining gates](docs/DEMO_PREPARATION_2026-10-25.md).

Рабочее название независимого Vulkan runtime/backend для stock Android,
первый целевой GPU — Mali-G925 на Galaxy Tab S11. Название пока не проверено
на совпадения с другими проектами или товарными знаками.

## Цель

Переиспользуемое userspace-ядро поверх штатного Android Vulkan-драйвера:
ресурсы, синхронизация, проверяемая совместимость и профилирование. Затем
адаптеры для Windows ARM64 в U-AVF, DroidDeck и Ubuntu.

Это **не новый аппаратный драйвер Mali** и пока не готовый универсальный
Vulkan ICD. Самостоятельный Android backend не равен Vulkan внутри VM и
сам по себе не ускорит DWM. Для этого нужны отдельные гостевые интеграции.

## Приоритет

1. Общая совместимость: BC1–BC7, корректные null-ресурсы и требования DXVK.
2. Настоящий D3D11-кадр через DXVK-Sarek в DroidDeck, затем реальные приложения.
3. Windows: постоянный IddCx и продуктовая интеграция workspace в U-AVF.
4. Дальнейший Windows GPU/WDDM R&D и Ubuntu Vulkan, сохраняя VirGL/OpenGL.

Актуальные критерии и незавершённые задачи: [ACTIVE_PLAN](docs/ACTIVE_PLAN.md).
Аппаратные Core/Bridge и ограниченный Windows Vulkan ICD уже проверены;
это пока **не** означает готовность DXVK, игрового runtime или ускорение DWM.

Цель игровой совместимости на уровне альтернативных GPU-стеков — направление
разработки, **не обещание паритета со Snapdragon/Turnip**. Сравнение возможно
только на одинаковых играх, настройках и измерениях frame time.

## Первый результат, который нужно получить

Небольшой отдельный Android test harness в app sandbox:

- инвентаризация реального Vulkan device/features/extensions;
- аппаратный graphics + compute workload, проверяемое изображение/результат;
- повторные allocate/submit/synchronize/free и lifecycle без утечек/ошибок;
- CPU submit time, GPU time где поддерживается, p50/p95/p99 frame time,
  память и накладные расходы относительно прямого вызова Android Vulkan.

Не начинать с полного Vulkan command-remoting или WDDM-драйвера. Сначала
доказать этот небольшой native milestone и составить capability matrix.

План и критерии: [docs/ROADMAP.md](docs/ROADMAP.md).
Текущее состояние: [docs/STATE.md](docs/STATE.md).

## Границы

- Stock Android 16, без root, unlock, flash, ослабления SELinux/Code Integrity,
  private Binder и замены системного crosvm/vendor-драйвера.
- U-AVF Ubuntu/P7, Windows 8 CPU/storage/firmware и рабочий H.264 path сохраняются.
- Не объявлять unsupported Vulkan feature поддерживаемой ради запуска приложения.
- Никаких commit/push/publication. Эта папка ещё не отдельный Git-репозиторий;
  публичный репозиторий пользователь создаст позже.
- Лицензия/зависимости/права проверяются до публикации. Новая лицензия здесь
  пока не назначается; сторонний код не становится собственным кодом Mavk.

На 2026-10-15 native compute и graphics работают на аппаратном G925.
Простой Surface workload подтверждён независимыми presentation timestamps
(59.92/с за фиксированные28с). Минимальный Core выделен отдельно; compute/image
A/B прошёл в3 процессах. Bounded graphics/lifecycle/synchronization-validation
gate пройден без ошибок; это не исчерпывающая проверка backend. Минимальный
compute Bridge проверен из Windows через app-owned AVF:8 аппаратных GPU jobs,
131072 значения проверены гостем,0 validation errors/warnings. Это ещё не ICD
или ускорение DWM. Graphics Bridge также PASS:3330 пикселей проверены Windows.
Ограниченный offscreen ICD прошёл runtime-тест в Windows ARM64: стандартный
Vulkan Loader → ICD → Mali,3330 пикселей проверены самим API-приложением,
EXIT0,zero validation errors/warnings. Это не WDDM/DWM, не игровой runtime и
не Vulkan conformance. См. [границы ICD](icd/README.md).
На 2026-10-16 неизменённый сторонний SaschaWillems renderheadless также
прошёл в Windows ARM64: стандартный Loader/ICD → Mali, изображение1024²,
3624 независимые проверки пикселей, EXIT0, ноль validation errors/warnings.
Пока это offscreen-рендер, не вывод Vulkan-окна, не игровой PASS и не DWM.
См. [checkpoint](docs/STATE.md).
