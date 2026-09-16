# edr\_test

Test task EDR agent



plan:

develop some sort of thread safe buffer +

create test/mock files and dirs +

create process analyzer +

create file analyzer +

create alerts classes +

create collector for alerts +

create some sort of backend listener to receive alerts by tcp protocol +


СБОРКА
- **CMake** ≥ 3.15 (проверено на 4.0.2)
- **Conan** 2.x (проверено на 2.16.1)
Тулл чейн , в данном случае: 
- **Windows** + **Visual Studio 2022** (Desktop C++ workload), MSVC v143

- В рабочей директории репозитория
# 1. Boost через Conan 
conan install . --build=missing

# 2. Конфигурация
cmake -S edr_reader -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake `
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
  Или ищем какой-то свой тулчейн, cmake preset

# 3. Сборка всех трёх исполняемых файлов
cmake --build build --config Release --target edr_agent edr_listener edr_unit_test

## Запуск

### `edr_agent.exe`

```
--help                  справка
--host, -h  <string>    хост, куда слать алерты (обязательный)
--port, -p  <string>    порт (обязательный)
--events, -e <string>   путь к файлу событий (default: process_events.txt)
--manifest, -m <string> путь к baseline-манифесту (default: manifest_test.json)
--dir, -d <string>      директория для контроля целостности (default: test_dir)
```

### `edr_listener.exe`

```
--help                  справка
--port, -p  <string>    порт для прослушивания (обязательный)
```

## Юнит-тесты

```powershell
.\build\Release\edr_unit_test.exe            # все тесты
.\build\Release\edr_unit_test.exe "[detector]"  # только правило детектирования
.\build\Release\edr_unit_test.exe "[buffer]"    # только очередь



Доработки
Подключение к реальным журналам
Шифрование трафика
Демонизация и далее по списку
Из важных:
Изменение логиики хеширования для больших файлов - у пикоша есть отдельная логика для этого.
Определениие количества ядер/реальных потоков и балансировка нагрузки.
Также в целом как выбор контейнеров, так и доработка логики/архитектуры
Для разных юз кейсов, ограничения по железу (памяти, скорости доступа к файлам), количество и размеры файлов,
скорость добавления событий и проч, требуют значительной доработки архитектуры для оптимизации скорости работы

По мелочи:
Доработка буффера, распаралеливание дополнительно алгоритмов - детектирования например
Профилирование методов, разноска логики в разные приложения/ сервисы
Или наоборот монолитизация, опятьже зависит от юз кейсов