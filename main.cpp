#include <QApplication>
#include <QResource>
#include <QSystemTrayIcon>
#include <QMessageBox>

#include <windows.h>
#include <winuser.h>

#include "ui/widget.h"
#include "utils/StringConstants.h"
#include "event/NativeEventFilter.h"

// Clion中QT6的配置，外部工具与打包 https://zhuanlan.zhihu.com/p/519409942
int main(int argc, char *argv[]) {
    // QApplication::applicationDirPath()
    const QString &Resources_Path = QStringLiteral("./resource.rcc");
    const QString &Settings_Path =  QStringLiteral("./settings.json");
    const QString &RecordFile_Path =  QStringLiteral("./record.txt");
    const QString &Menu_QSS_Path =  QStringLiteral(":/qss/Resources/qss/menu.qss");

    const QString &Project_Name = QStringLiteral("EggyPet");
    const QString &System_Icon =  QStringLiteral(":/ico/Resources/ico/sys_tray_icon.png");

    QApplication a(argc, argv);

    // 使用rcc则可以直接在json中进行配置
    if (!QResource::registerResource(Resources_Path)) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("加载错误"), QStringLiteral("资源文件resource.rcc加载失败"));
        QApplication::quit();
        return 0;
    }

    // 加载menu_qss
    QFile qss(Menu_QSS_Path);
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(qss.readAll());
    }
    qss.close();

    Widget w(Settings_Path, RecordFile_Path);
    if (!w.isReady()) {
        QApplication::quit();
        return 0;
    }

    // 全局快捷键
    NativeEventFilter filter(MOD_ALT, 'Q', w.essay_action);
    // 先系统注册快捷键成功后再绑定事件过滤器
    if (RegisterHotKey(nullptr, filter.key ^ filter.mod, filter.mod, filter.key)) {
        a.installNativeEventFilter(&filter);
    }

    // 设置系统托盘
    QIcon icon(System_Icon);
    w.setWindowIcon(icon);
    QSystemTrayIcon systemTrayIcon(icon, &w);
    w.setSystemTrayIcon(&systemTrayIcon);
    systemTrayIcon.setToolTip(Project_Name);
    QMenu menu;
    auto showAct = new QAction(StringConstants::SHOW, &systemTrayIcon);
    auto exitAct = new QAction(StringConstants::EXIT, &systemTrayIcon);
    auto fixedAct = new QAction(StringConstants::FIXED, &systemTrayIcon);

    QObject::connect(showAct, &QAction::triggered, [&](){
        w.setVisible(true);
    });
    QObject::connect(exitAct, &QAction::triggered, [&](){
        w.exit();
        systemTrayIcon.hide();
        QApplication::quit();
    });
    QObject::connect(fixedAct, &QAction::triggered, [&](){
        if (StringConstants::FIXED == fixedAct->text()) {
            w.removeMouseListener();
            fixedAct->setText(StringConstants::UNFIXED);
        } else {
            w.bindMouseListener();
            fixedAct->setText(StringConstants::FIXED);
        }
    });

    menu.addAction(showAct);
    menu.addAction(fixedAct);
    menu.addAction(exitAct);
    systemTrayIcon.setContextMenu(&menu);

    systemTrayIcon.show();
    w.show();
    return QApplication::exec();
}
