# Mavk

Latest: [live Muck profiling, artifact rollback and limitations](docs/MUCK_GPU_PROFILE_AND_ARTIFACTS_2026-10-25.md).
Developer prototype: [AHB/input results and remaining gates](docs/DEMO_PREPARATION_2026-10-25.md).

Рабочее название независимого Vulkan runtime/backend для stock Android,
первый целевой GPU — Mali-G925 на Galaxy Tab S11. Название пока не проверено
на совпадения с другими проектами или товарными знаками.

English developer guide: [architecture, results, builds and limits](docs/DEVELOPER_REVIEW.md).

## Цель

Переиспользуемое userspace-ядро поверх штатного Android Vulkan-драйвера:
ресурсы, синхронизация, проверяемая совместимость и профилирование. Затем
адаптеры для Windows ARM64 в U-AVF, DroidDeck и Ubuntu.

Это **не новый аппаратный драйвер Mali** и пока не готовый универсальный
Vulkan ICD. Самостоятельный Android backend не равен Vulkan внутри VM и
сам по себе не ускорит DWM. Для этого нужны отдельные гостевые интеграции.

## Приоритет

1. Завершение DroidDeck-прототипа: обычный запуск игр и передача разработчикам. Длительная оптимизация FPS остановлена.
2. Общая совместимость по реальным требованиям приложений, без фиктивных features.
3. Windows: постоянный IddCx и продуктовая интеграция workspace в U-AVF.
4. Дальнейший Windows GPU/WDDM R&D и Ubuntu Vulkan, сохраняя VirGL/OpenGL.

Актуальные критерии и незавершённые задачи: [ACTIVE_PLAN](docs/ACTIVE_PLAN.md).
Аппаратные Core/Bridge и ограниченный Windows Vulkan ICD уже проверены;
это пока **не** означает готовность DXVK, игрового runtime или ускорение DWM.

Цель игровой совместимости на уровне альтернативных GPU-стеков — направление
разработки, **не обещание паритета со Snapdragon/Turnip**. Сравнение возможно
только на одинаковых играх, настройках и измерениях frame time.

## Проверенный игровой прототип

Steam → ARM64 Proton/Wine/FEX → экспериментальный DXVK-Sarek → Mavk
Compatibility → libhybris → штатный Vulkan Mali. Для вывода используется
необязательный XCB/Wayland AHB backend с acquire/release fences. RGBA передаётся
напрямую; BGRA требует одного GPU blit, без CPU readback. SHM остаётся fallback.
Общий Vulkan/Android код находится в `core/`, `compatibility/`, `adapters/android/`;
интеграция launcher — в `adapters/droiddeck/`, WSI — в `third_party/wsi-layer/`.
Исходный Android compositor и интерфейс выбора backend принадлежат отдельному
DroidDeck-форку; Mavk не заменяет Steam/Proton или интерфейс DroidDeck.

| Проверка на Tab S11 / Mali-G925 | Фактический результат |
|---|---|
| Native graphics/compute и ограниченный Windows ICD | PASS, см. ссылки ниже |
| AMD64 Wine D3D11, AHB, ResizeBuffers1920→1280→1920 | PASS, CoreZero/gaps0 |
| Native D3D11 через fallback SHM | Видимый поток,598 copied buffers/10с |
| Muck, движущееся меню | ~34–35 отображённых buffers/с, AHB/copy0 |
| Muck, одна тяжёлая игровая сцена | ~9.1 buffers/с при1600p и1200p |
| Half-Life2, Full HD | По наблюдению пользователя: обычно20–30FPS почти на максимальных настройках,30–50 на низких; редкие подвисания/повторы звука |
| DDNet | Текущий Linux/FEX startup blocker до Vulkan; не игровой PASS |

Buffers/с — измерение SurfaceFlinger и compositor, а не доказательство60
уникальных игровых кадров. Повторяющиеся чёрные диагональные артефакты остаются
открытой проблемой; экспериментальная CSE-оптимизация откачена. Измерения на
нагретом устройстве нельзя сравнивать с холодными без учёта thermal state.
Полная поддержка upstream DXVK и Vulkan CTS/conformance не заявляется.

План и критерии: [docs/ROADMAP.md](docs/ROADMAP.md).
Текущее состояние: [docs/STATE.md](docs/STATE.md).

## Границы

- Stock Android 16, без root, unlock, flash, ослабления SELinux/Code Integrity,
  private Binder и замены системного crosvm/vendor-драйвера.
- U-AVF Ubuntu/P7, Windows 8 CPU/storage/firmware и рабочий H.264 path сохраняются.
- Не объявлять unsupported Vulkan feature поддерживаемой ради запуска приложения.
- Локальные коммиты разрешены; push и публикация без разрешения запрещены.
- Лицензии: [LICENSE](LICENSE), [сторонние компоненты](THIRD_PARTY_NOTICES.md).
  Собственный код Apache2, кроме индивидуально отмеченных файлов; WSI MIT,
  производный Proton wrapper GPLv3. Сторонний код сохраняет свою лицензию.
  Полнота corresponding-source комплекта runtime пока не подтверждена.

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
