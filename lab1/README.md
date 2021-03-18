# Лабораторная работы #1

**Дисциплина:** "Системы ввода/вывода"

**Период:** весна 2021

**Название:** "Разработка драйверов символьных устройств"

**Цель работы:** получить знания и навыки разработки драйверов символьных устройств для операционной системы Linux.

## Описание функциональности драйвера

### Требования

Драйвер символьного устройства, должен удовлетворять требованиям:
1. Драйвер должен создавать символьное устройство /dev/var2
2. Драйвер должен создавать интерфейс для получения сведений о результатах операций над созданным в п.1 символьным устройством: файл /proc/var2

### Описания работы драйвера

При записи в файл символьного устройства текста типа “5+6” должен запоминаться результат операции, то есть 11 для данного примера. Должны поддерживаться операции сложения, вычитания, умножения и деления. Последовательность полученных результатов с момента загрузки модуля ядра должна выводиться при чтении созданного файла /proc/var2 в консоль пользователя.

При чтении из файла символьного устройства в кольцевой буфер ядра должен осуществляться вывод тех же данных, которые выводятся при чтении файла /proc/var2.


## Инструкция по сборке

! Используется Linux Kernel - 4.15.0

Сборка драйвера:

```bash
sudo make build
```

Собирается и создается файл драйвера `drv_module.ko`

Установка драйвера в систему Linux

```bash
make install
```

Удаление драйвера из систему Linux

```bash
make uninstall
```

Чистка директории от файлов сборки

```bash
make clean
```

Для упрощения работы были реализованны следующие методы:
```bash
make all
```
Который включает в себя методы `build` и `install`

```bash
make recompile
```
Который включает в себя методы `uninstall`, `clean`, `build` и `install` в соответствующем порядке. Данный метод удобен для повторной загрузке модуля в ядро системы.


## Инструкция пользователя
Модулю реализует подсчет введённых в символьное устройство алгебраических выражений следующего типа `r'\d+(\+|\-|\\|\*)\d+'`.

Чтобы подсчитать выражение, надо в `/dev/var2` записать это самое выражение. Например:
```bash
echo "5+6" > /dev/var2
```

Результат всех операций записан в `/proc/var2`. Прочитать результат: 
```bash
cat /proc/var2
```

Если при записи в /dev/var2 возникает ошибка __Permission denied__ выполнить:
```bash
chmod 777 /dev/var2
```

## Примеры использования

```bash
linux-pc@student lab1> echo "2 + 2" > /dev/var2
linux-pc@student lab1> echo "2 - 2" > /dev/var2
linux-pc@student lab1> echo "2 / 2" > /dev/var2
linux-pc@student lab1> echo "2 * 2" > /dev/var2
linux-pc@student lab1> echo "2 / 0" > /dev/var2
linux-pc@student lab1> cat /proc/var2
ERR: ZeroDivision
Result 4: 4
Result 3: 1
Result 2: 0
Result 1: 4
linux-pc@student lab1> dmesg | tail -n 19
[  197.272779] Loading module
[  197.272782] drv_module: Registered with major 242
[  197.272790] drv_module: Registered device class: drv_modules
[  197.272833] drv_module: Registered device: /dev/var2
[  197.272836] drv_module: /proc/var2 is created
[  229.865844] drv_module: Invoked proc_read
[  229.865846] drv_module: Result 5: ERR: ZeroDivision
[  229.865849] drv_module: Result 4: 4
[  229.865849] drv_module: Result 3: 1
[  229.865850] drv_module: Result 2: 0
[  229.865851] drv_module: Result 1: 4
[  229.865851] drv_module: Sum of all correct expressions: 9
[  229.865876] drv_module: Invoked proc_read
[  229.865877] drv_module: Result 5: ERR: ZeroDivision
[  229.865879] drv_module: Result 4: 4
[  229.865880] drv_module: Result 3: 1
[  229.865880] drv_module: Result 2: 0
[  229.865881] drv_module: Result 1: 4
[  229.865881] drv_module: Sum of all correct expressions: 9
```

...
