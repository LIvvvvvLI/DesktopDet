#ifndef EGGYPET_STRINGCONSTANTS_H
#define EGGYPET_STRINGCONSTANTS_H

#include <QString>

class StringConstants {
public:
    // 禁止用户创建 StringConstants 的实例
    StringConstants() = delete;

    // system icon
    static const QString &SHOW;
    static const QString &EXIT;
    static const QString &FIXED;
    static const QString &UNFIXED;

    // menu
    static const QString &ALWAYS_STANDBY;
    static const QString &UNALWAYS_STANDBY;
};

class TipString {
public:
    TipString() = delete;

    static const QString &FILE_LOAD_ERROR;
    static const QString &CHOOSE_A_EXE;
    static const QString &EXE_FILTER;
    static const QString &CHOOSE_A_FOLDER;
    static const QString &CHOOSE_A_FILE;
    static const QString &INPUT_NEWNAME;
    static const QString &EDIT_OR_DELETE;

    static const QString &CURRENT_PATH;
    static const QString &SUPPORT_CHINESE_IN_PATH;
};

#endif //EGGYPET_STRINGCONSTANTS_H
