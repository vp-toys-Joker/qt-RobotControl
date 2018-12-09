# qt-RobotControl
qt project for arduino robot (https://github.com/vp-toys-Joker/arduino-robotbtcontrol)
Управление 4-х колесным arduino роботом по bluetooth каналу.
Статус: в разработке
Для управления роботом используются следующие клавиши клавиатуры:
1. "Up" - движение вперед
2. "Down" - движение назад
3. "Left" - движение разворот влево
4. "Right" - движение разворот вправо
5. "Home" или "Up+Left" - движение вперед и поворот влево
6. "PageUp" или "Up+Right" - движение вперед и поворот вправо
7. "End" или "Down+Left" - движение назад и поворот влево
8. "PageDn" или "Down+Right" - движение назад и поворот вправо
9. "+" - увеличение скорости движения (на данный момент не реализовано).
10. "-" - уменьшение скорости движения (на данный момент не реализовано).
Автоповтор игнорируется. Учитывается только первое нажатие и отпускание клавишей.
При отпускании одинарной или последней из двух одновременно нажатых управляющих клавишей посылается команда останов движения.

По BlueTooth каналу осуществляется односторонняя передача в напрвление от ПК к роботу следующих команд:
    'F' - движение вперед
    'L' - движение разворот влево
    'R' - движение разворот вправо
    'S' - останов движения
    'B' - движение назад
    'G' - движение вперед и влево
    'I' - движение вперед и вправо
    'H' - движение назад и влево
    'J' - движение назад и вправо
