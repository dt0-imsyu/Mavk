# Mavk

**Экспериментальная Vulkan-прослойка поверх штатных GPU-драйверов Android.**
Первое проверенное устройство — Galaxy Tab S11 / Mali-G925, без root и прошивок.

[English](README.md) · [Форк DroidDeck](https://github.com/dt0-imsyu/DroidDeck) · [Архитектура и доказательства](docs/DEVELOPER_REVIEW.md) · [Сторонние лицензии](THIRD_PARTY_NOTICES.md)

## Что делает проект

Mavk объединяет переиспользуемое управление ресурсами и синхронизацией Vulkan,
совместимость и адаптеры вывода для DroidDeck и U-AVF. Работу на GPU выполняет
штатный драйвер Android. **Это не новый драйвер ядра Mali, не полноценный Vulkan
ICD, не WDDM-драйвер и не устанавливаемый вместо Turnip ZIP.**

Работающий игровой прототип:

`Steam → ARM64 Proton/Wine/FEX → экспериментальный DXVK-Sarek → Mavk → libhybris → Android Vulkan/Mali`

Необязательный XCB/Wayland AHardwareBuffer-вывод убирает чтение framebuffer через
CPU. RGBA использует прямой путь, BGRA требует преобразования на GPU. X11/SHM
сохранён как резервный вариант. Android-интерфейс и композитор принадлежат
отдельному форку DroidDeck.

## Попробовать — установить APK DroidDeck

Возьми [тестовый релиз форка](https://github.com/dt0-imsyu/DroidDeck/releases) и
[короткую инструкцию](https://github.com/dt0-imsyu/DroidDeck/blob/mavk-demo/docs/TRY_TEST_APK.md).
Mavk и проверенная интеграция уже внутри APK. Установи его, пройди обычную
загрузку runtime, включи Mavk/AHB в настройках launcher и используй Play в Steam.
Для этого пути не нужны сборка исходников или команды для каждой игры.

Сам Mavk — библиотека и слои для разработчиков: универсальной кнопки «установить
драйвер Android» нет. Не копируй библиотеки в системные/vendor-разделы Android.
Другому приложению нужен адаптер с правильными loader/ABI, владением ресурсами
и согласованным механизмом вывода.

## Реальные результаты

| Проверка на stock Tab S11 / Mali-G925 | Результат |
| --- | --- |
| Native Core compute/graphics и GPU readback | PASS; подробности в обзоре и истории |
| Ограниченные Windows Vulkan Loader/ICD и Bridge | Экспериментальный offscreen PASS; это не ускорение DWM |
| Wine D3D11, AHB и ResizeBuffers 1920→1280→1920 | PASS в отдельном потребителе |
| HL2, 1920×1080, низкие настройки, средние объекты/текстуры | По наблюдению владельца: от 30 FPS, местами стабильные 60 |
| HL2, почти максимальные настройки | Обычно 20–30 FPS по наблюдению владельца; редкие подвисания персонажей/звука |
| Muck, тяжёлый геймплей | Около 12–13 отображённых buffers/с в одной сцене, 9.1 в другой |
| Muck, движущееся меню | Около 34–35 отображённых buffers/с, AHB без CPU-копирования |
| Резервный SHM | Видимый движущийся D3D11-поток; повторно проверен кадр загрузки игры |

Наблюдения в игре, вызовы Present и счётчики compositor/SurfaceFlinger — разные
доказательства. HL2 не держит 60 FPS во всех сценах. Последний shader A/B Muck
дал 12.5 / 12.7 / 12.3, без существенного выигрыша; исследование FPS остановлено.
Полные Vulkan CTS, upstream DXVK, широкая игровая совместимость и проверка на
Adreno/других Mali не заявляются. Смена разрешения внутри HL2 может приводить
к вылету: выбирай режим до запуска сессии. Остаются периодические артефакты,
чёрные переходы первой главы и некоторые live-alias случаи D3D9-дескрипторов.

## Встроить Mavk Core в другое приложение

Нужны CMake 3.22+, компилятор C++17 и Vulkan headers/loader для целевой платформы.
Вызывающее приложение создаёт Vulkan device/queue и владеет ими; Core не создаёт
гостевой GPU или удалённый транспорт. Сборка и установка отдельной статической библиотеки:

```sh
cmake -S . -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core
cmake --install build/core --prefix build/install
```

Для Android при конфигурации укажи NDK toolchain и параметры цели:

```sh
cmake -S . -B build/android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DCMAKE_BUILD_TYPE=Release
cmake --build build/android
```

В проекте-потребителе направь `Mavk_DIR` на установленный каталог
`lib/cmake/Mavk`, затем подключи цель:

```cmake
find_package(Mavk CONFIG REQUIRED)
target_link_libraries(your_application PRIVATE Mavk::Core)
```

Подключай `<mavk/core.hpp>`. Сохраняй `VkDevice` и очередь живыми, синхронизируй
использование очереди снаружи и заверши GPU-работу до освобождения ресурсов.
Для поиска установленного пакета с Android cross-toolchain может потребоваться
`CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH`; сам Vulkan остаётся из целевого NDK.
Эта цель собирает **только Core**, а не слой совместимости, ICD или DroidDeck.

## Встроить адаптеры совместимости и вывода

| Каталог | Назначение |
| --- | --- |
| `core/` | Native Vulkan-ресурсы и отправка работы |
| `compatibility/` | Состояние/время жизни дескрипторов, BC, ограниченные shader guards |
| `adapters/vulkan-layer/` | Явный glibc Vulkan-слой и native dispatch |
| `adapters/android/`, `third_party/wsi-layer/` | Android/AHB-интерфейсы и MIT-форк WSI |
| `adapters/droiddeck/` | Упаковка runtime и производный GPLv3 Proton wrapper |
| `bridge/`, `icd/` | Отдельная экспериментальная интеграция U-AVF/Windows |
| `harness/`, `experiments/` | Аппаратные потребители и сохранённые инструменты разработки |

Для glibc-слоя нужны совместимые Vulkan/SPIRV-Tools build inputs и Android ABI bridge
в DroidDeck; это не Bionic drop-in. У SPIR-V-компилятора отдельный CMake-проект
в `compatibility/shaders`. Сборочные helpers и source pins описаны в
[обзоре](docs/DEVELOPER_REVIEW.md). Исторические исследовательские скрипты частично
используют локальные пути toolchain: это не переносимые установщики одной командой.
Для проверки игр используй готовую APK-интеграцию.

Переноси AHB allocation/import, acquire/release fences, fullscreen-геометрию и
поддержку compositor вместе. Одна WSI-библиотека не предоставляет DroidDeck-протокол
или его lifecycle. Сохраняй SHM и исходный путь Adreno. Не объявляй возможности,
которые выбранный драйвер/адаптер не реализует.

## Отладка и проверки

* В DroidDeck используй **Share logs** в Setup/шторке сессии. Укажи устройство/GPU,
  версию APK, игру, разрешение, настройки, выбор AHB/SHM и короткую движущуюся
  сцену. Перед отправкой убери из логов данные учётных записей.
* Для управляемого разработчиком процесса `MAVK_LAYER_PROFILE=1` включает
  ограниченные сводки CPU-времени, `MAVK_LAYER_TRACE=1` — подробные вызовы.
  По умолчанию оба выключены. `MAVK_GUARD_REPORTS=0` убирает диагностические
  записи/readback шейдеров, но не access guards. Для сравнения FPS выключай логи.
* Разбирай `MAVK_LAYER_GAP` на конкретной API-операции/ресурсе. Не устраняй его
  фиктивным feature bit или отключением защит для всех игр.
* Совместимость проверяй GPU-исполнением/readback, а Bridge/API-изменения — реальным
  потребителем. Native, проверенная эмуляция и неподдержанные пути должны различаться.
  Синтетический PASS не доказывает игровую совместимость.
* GPU-query/CSE-эксперименты, связанные с регрессиями, остаются выключенными.
  Число Present или частота экрана сами по себе не доказывают уникальные игровые FPS.

[Текущий handoff](docs/ACTIVE_PLAN.md) · [История доказательств](docs/STATE.md) ·
[Покрытие исходниками](docs/RUNTIME_SOURCE_COVERAGE_2026-10-26.md) · [План](docs/ROADMAP.md)

## Лицензия и авторство

Собственный код Mavk открыт под **Apache License 2.0**. При распространении сохраняй
copyright, лицензию и [NOTICE](NOTICE), отмечай свои изменения согласно лицензии.
Автор оригинальной работы Mavk — **dt0-imsyu**, также сохраняются права участников.

Сторонний код сохраняет свои условия: WSI — MIT; производный от DroidDeck Proton
wrapper — GPLv3; DXVK-Sarek, Mesa, libhybris, Openbox и компиляторные зависимости
не перелицензированы в Mavk. См. [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
Экспериментальный source/reviewer-комплект не заявляет полного покрытия исходниками
каждого внешнего бинарного компонента. Найденные пробелы записаны отдельно.
