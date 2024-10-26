#include "functions.h"

// Функция для разделения строки на две части
void splitString(const QString &inputString, QString &lastName, QString &firstNameMiddleName) {
    QString input = inputString.trimmed();
    // Поиск пробела в строке
    int spaceIndex = input.indexOf(' ');

    // Если пробел найден
    if (spaceIndex != -1) {
        // Получение фамилии
        lastName = input.left(spaceIndex).trimmed();
        // Удаление фамилии из исходной строки
        input.remove(0, spaceIndex + 1);

        firstNameMiddleName = input.trimmed();

        // Если в строке есть ещё один пробел
        // if (input.contains(' ')) {
        //     // Разделение имени и отчества
        //     int nextSpaceIndex = input.indexOf(' ');
        //     firstNameMiddleName = input.mid(0, nextSpaceIndex);
        //     input.remove(0, nextSpaceIndex + 1); // Удаление имени из строки
        // } else {
        //     // Если в строке только имя после фамилии, то добавляем его к отчеству
        //     firstNameMiddleName = input;
        // }
    } else { // Если пробел не найден, то предполагается, что в строке только фамилия и имя
        lastName = input.trimmed();
        firstNameMiddleName = "";
    }
}
