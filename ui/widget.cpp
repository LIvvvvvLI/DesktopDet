#include "widget.h"

#include <QApplication>
#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QTime>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QInputDialog>

#include <QtWin>

#include <windows.h>
#include <winuser.h>

#include "../utils/StringConstants.h"
#include "../event/DragFilter.h"
#include "../event/EventFilterForDeleteOrEditActionInMenu.h"
#include "../event/NativeEventFilter.h"
#include "EssayInputDialog.h"
#include "../event/EventFilterForDeleteActionInMenu.h"

// 函数头一定要完整
BOOL CALLBACK onWindowsEnum(HWND hwnd, LPARAM lparam) {
    auto *hwnd_vector = reinterpret_cast<QVector<int> *>(lparam);

    if (!IsWindowVisible(hwnd)) return true;
    if (!IsWindowEnabled(hwnd)) return true;

    HWND hParent = (HWND) GetWindowLong(hwnd, GWL_HWNDPARENT);
    if (IsWindowEnabled(hParent)) return true;
    if (IsWindowVisible(hParent)) return true;
    LONG gwl_style = GetWindowLong(hwnd, GWL_EXSTYLE);
    if ((gwl_style & WS_POPUP) && !(gwl_style & WS_CAPTION)) return true;

    char class_name[256];
    GetClassName(hwnd, class_name, sizeof(class_name));
//    // 可以考虑用title_name不为零再过滤一次
//    char title_name[256];
//    GetWindowText(hwnd, title_name, sizeof(title_name));

    // 任务栏本身可以用class_name == "Progman" or class_name == "Shell_TrayWnd"来过滤

    // 文件管理器的ClassName为CabinetWClass
    if (strcmp("CabinetWClass", class_name) == 0) {
        hwnd_vector->append(reinterpret_cast<const int>(hwnd));
    }

    // 文件管理器在这里会被筛掉
    if (GetClassLong(hwnd, GCL_HICON)) {
        // 排除掉Windows相关的窗口
        if (strstr(class_name, "Windows") == NULL) {
            HICON hIcon = (HICON) GetClassLong(hwnd, GCL_HICON);
            if (hIcon) {
                hwnd_vector->append(reinterpret_cast<const int>(hwnd));
            }
        }
    }
    return true;
}

Widget::Widget(const QString &settings_json_path, const QString &record_file_path, QWidget *parent) :
    QWidget(parent), settings_json_path(settings_json_path), record_file_path(record_file_path) {

    // 此处加载失败会直接isReady()返回false
    if (!loadQMapFromJsonFile(this->settings_json_path)) return;

    drag_filter = new DragFilter();
    this->bindMouseListener();  // 设置鼠标事件捕获以移动窗口
    deleteOrEdit_filter = new EventFilterForDeleteOrEditActionInMenu();

    setWindowFlag(Qt::Tool);  // 隐藏任务栏图标
    setWindowFlag(Qt::WindowStaysOnTopHint);  // 窗口始终置顶
    setWindowFlag(Qt::FramelessWindowHint);  // 去除边框
    setAttribute(Qt::WA_TranslucentBackground);  // 背景透明

    if (!(width & height)) {
        width = 124;
        height = 124;
        need_save = true;
    }
    resize(width, height);  // 固定比例, 为python程序里fixed的缩放(178, 178)


    label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);
    label->setScaledContents(true);
    // 解决只能放大窗口不能缩小的问题
    // 来自https://blog.csdn.net/weixin_42193704/article/details/121287558
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    movie = new QMovie(this);

    menu = new QMenu(this);
    folder_menu = new QMenu(QStringLiteral("目录"), menu);
    software_menu = new QMenu(QStringLiteral("软件"), menu);
    file_menu = new QMenu(QStringLiteral("文件"), menu);

    initMenu();

//    bindActionListener();

    // GIF文件的循环次数是由GIF文件本身决定的，通常为无限循环
    connect(movie, &QMovie::finished, this, &Widget::onMovieFinished);
    connect(movie, &QMovie::frameChanged, [this](int frameNumber){
        // GIF动画播放完最后一帧即视为Finished
        if (frameNumber == movie->frameCount() - 1) {
            onMovieFinished();
        }
    });

    cur_role_act = &Standby_Role_Act;
    showActAnimation(Standby_Role_Act);
}

Widget::~Widget() {
    if (isReady()) {
        delete drag_filter;
        delete deleteOrEdit_filter;
        delete software_menu;
        delete folder_menu;
        delete menu;
        delete movie;
        delete label;
    }
}

void Widget::paintEvent(QPaintEvent *event) {
    auto size = this->size();
    label->resize(size);
    movie->setScaledSize(size);
}

void Widget::contextMenuEvent(QContextMenuEvent *event) {
    menu->popup(QCursor::pos());
    // 置顶窗口并获得焦点
    // 来自https://developer.aliyun.com/article/1468296
    menu->raise();
    menu->activateWindow();
    event->accept();  // 接受事件，防止默认行为
}

void Widget::onMovieFinished() {
    if (always_standby) {
        showActAnimation(Standby_Role_Act);
    } else {
        showRandomAction();
    }
}

void Widget::initMenu() {
    configMenu(menu);

    // 随笔
    essay_action = new QAction(QStringLiteral("随笔"), menu);
    menu->addAction(essay_action);
    connect(essay_action, &QAction::triggered, this, [this]() {
        auto *a = new EssayInputDialog(record_file_path, this);
        a->show();
        // TODO: 第一次打开窗口时无法成功获得焦点, 之后就可以成功获得焦点了
        a->raise();
        a->activateWindow();
        a->setFocus();
        a->textEdit->setFocus();
    });

    // 动作
    QMenu *action_menu = new QMenu(QStringLiteral("动作"), menu);
    menu->addMenu(action_menu);
    for (auto iter = action_map.begin(); iter != action_map.end(); ++iter) {
        action_menu->addAction(iter.key());
    }
    configMenu(action_menu);
    connect(action_menu, &QMenu::triggered, this, &Widget::onActionMenuTriggered);

    // 当前窗口
    QMenu *current_menu = new QMenu(QStringLiteral("切换"), menu);
    menu->addMenu(current_menu);
    configMenu(current_menu);
    char class_name[256];
    int size = sizeof(class_name);
    connect(current_menu, &QMenu::aboutToShow, this, [this, &class_name, size, current_menu]() {
//        ShellExecuteW(nullptr, );
        auto actions_in_current = current_menu->actions();
        for (QAction *a : actions_in_current) {
            current_menu->removeAction(a);
            delete a;
        }
        auto *hwnd_vector = new QVector<int>();
        EnumWindows(onWindowsEnum, reinterpret_cast<long>(hwnd_vector));
        for (int &i : *hwnd_vector) {
            GetWindowText(reinterpret_cast<HWND>(i), class_name, size);
            const QString &name = QString::fromLocal8Bit(class_name);
            HICON hIcon = (HICON) GetClassLong(reinterpret_cast<HWND>(i), GCL_HICON);
            QAction *run_software_action;
            if (hIcon) {
                run_software_action = new QAction(QtWin::fromHICON(hIcon), name, current_menu);
            } else {
                // 此时大概率是文件管理器
                run_software_action = new QAction(iconProvider.icon(QFileIconProvider::Folder), name, current_menu);
            }
//            QAction *run_software_action = new QAction(name, current_menu);
            connect(run_software_action, &QAction::triggered, this, [this, i] () {
                // https://learn.microsoft.com/zh-cn/windows/win32/api/winuser/nf-winuser-showwindow
                // SW_SHOW: 激活窗口并以当前大小和位置显示窗口。
                // SW_SHOWNA: 以当前的大小和位置来显示一个窗体，除非窗体是非激活状态的，否则本函数效果类似于SW_SHOW。
                // SW_RESTORE: 激活并显示一个指定的窗体，如果窗体处于最小化或最大化状态，系统会将其恢复到正常的大小和位置。
                // 当恢复一个最小化的窗口是，应用程序应该包含此标志。
                // SW_SHOWNOACTIVATE: 以最近的状态来显示一个窗体。除非窗台是非激活的，否则本函数的效果类似于SW_SHOWNORMAL。
                // SW_SHOWNORMAL: 激活并显示一个窗体，如果此窗体是处于最大化或最小化的，将恢复为默认的大小和位置。
                // 在程序第一次显示一个窗口时，应该设定这个标志。

                // 置顶方案一
                // BUG: 如果一直通过点击current_menu来切换，而没有操作置顶了的窗口的话，会失效
                // 可以考虑记录上一点击的窗口的句柄，在最后把它最小化
//                if (this->last_windows != -1) {
//                    ShowWindow(reinterpret_cast<HWND>(this->last_windows), SW_SHOWMINNOACTIVE);
//                }
//                if (IsIconic(reinterpret_cast<HWND>(i)) == 0) { // 判断是否最小化
//                    ShowWindow(reinterpret_cast<HWND>(i), SW_SHOWNA);
//                } else {
//                    ShowWindow(reinterpret_cast<HWND>(i), SW_RESTORE);
//                }
//                this->last_windows = i;

                // 置顶方案二, 无效, 内部好像也是通过SetWindowPos实现的
//                BringWindowToTop(reinterpret_cast<HWND>(i));

                // 置顶方案三, 参数有点复杂, 没仔细了解
//                SetWindowPos(reinterpret_cast<HWND>(i), HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);

                // 置顶方案四, 有时候虽然任务栏已经显示点亮切换到了, 但是并没有置顶显示
                // https://www.cnblogs.com/dengpeng1004/p/5049646.html
//                SetForegroundWindow(reinterpret_cast<HWND>(i));

                // 置顶方案五 https://www.cnblogs.com/ModBus/p/8709729.html
                // 目前没发现bug但可能被遗弃
                // 第二个参数需为true才能工作, 表示正在使用 Alt/Ctl+Tab 键序列将窗口切换到
                SwitchToThisWindow(reinterpret_cast<HWND>(i), true);
            });
            current_menu->addAction(run_software_action);
        }
        delete hwnd_vector;
    });
    current_menu->installEventFilter(new EventFilterForDeleteActionInMenu());

    // 运行软件
    menu->addMenu(software_menu);
    QAction *add_software_action = new QAction(QStringLiteral("新增软件"), software_menu);
    connect(add_software_action, &QAction::triggered, this, &Widget::onAddNewSoftware);
    software_menu->addAction(add_software_action);
    software_menu->addSeparator();
    for (auto iter = software_map.begin(); iter != software_map.end(); ++iter) {
        auto *run_software_action = new QAction(getIconFromFile(iter.value()), iter.key(), software_menu);
        connect(run_software_action, &QAction::triggered, this, [this, run_software_action] () {
            onRunSoftwareMenuTriggered(run_software_action);
        });
        software_menu->addAction(run_software_action);
//        run_software_action->installEventFilter(action_filter);
    }
    configMenu(software_menu);
    software_menu->installEventFilter(deleteOrEdit_filter);
    // 为了把打开软件的action和新增软件的action分开, 不再给run_menu注册事件
//    connect(software_menu, &QMenu::triggered, this, &Widget::onRunSoftwareMenuTriggered);

    // 打开目录
    menu->addMenu(folder_menu);
    QAction *add_folder_action = new QAction(QStringLiteral("新增目录"), folder_menu);
    connect(add_folder_action, &QAction::triggered, this, &Widget::onAddNewFolder);
    folder_menu->addAction(add_folder_action);
    folder_menu->addSeparator();
    for (auto iter = folder_map.begin(); iter != folder_map.end(); ++iter) {
        auto *open_folder_action = new QAction(iconProvider.icon(QFileIconProvider::Folder), iter.key(), folder_menu);
        connect(open_folder_action, &QAction::triggered, this, [this, open_folder_action] () {
            onOpenFolderMenuTriggered(open_folder_action);
        });
        folder_menu->addAction(open_folder_action);
    }
    configMenu(folder_menu);
    folder_menu->installEventFilter(deleteOrEdit_filter);
    //    connect(folder_menu, &QMenu::triggered, this, &Widget::onOpenFolderMenuTriggered);

    // 打开文件
    menu->addMenu(file_menu);
    QAction *add_file_action = new QAction(QStringLiteral("新增文件"), file_menu);
    connect(add_file_action, &QAction::triggered, this, &Widget::onAddNewFile);
    file_menu->addAction(add_file_action);
    file_menu->addSeparator();
    for (auto iter = file_map.begin(); iter != file_map.end(); ++iter) {
        auto *open_file_action = new QAction(getIconFromFile(iter.value()), iter.key(), file_menu);
        connect(open_file_action, &QAction::triggered, this, [this, open_file_action] () {
            onOpenFileMenuTriggered(open_file_action);
        });
        file_menu->addAction(open_file_action);
    }
    configMenu(file_menu);
    file_menu->installEventFilter(deleteOrEdit_filter);

    // 始终待机
    QAction *always_standby_action = new QAction(StringConstants::ALWAYS_STANDBY, menu);
    connect(always_standby_action, &QAction::triggered, [this, always_standby_action]() {
        if (StringConstants::ALWAYS_STANDBY == always_standby_action->text()) {
            always_standby = true;
            always_standby_action->setText(StringConstants::UNALWAYS_STANDBY);
        } else {
            always_standby = false;
            always_standby_action->setText(StringConstants::ALWAYS_STANDBY);
        }
    });
    menu->addAction(always_standby_action);


    // 隐藏
    QAction *hide = new QAction(QStringLiteral("隐藏"), menu);
    connect(hide, &QAction::triggered, [this]() {
        this->setVisible(false);
    });
    menu->addAction(hide);

    // 退出
    QAction *exit = new QAction(StringConstants::EXIT, menu);
    connect(exit, &QAction::triggered, [this]() {
        this->exit();
        systemTrayIcon->hide();
//        if (systemTrayIcon != nullptr) {
//            // TODO 优化
//            systemTrayIcon->hide();
//            delete systemTrayIcon;
//            systemTrayIcon = nullptr;
//        }
        QApplication::quit();
    });
    menu->addAction(exit);
}

void Widget::onActionMenuTriggered(QAction *action) {
    showActAnimation(action->text());
}

void Widget::onRunSoftwareMenuTriggered(QAction *action) {
    runSoftware(action->text());
}

void Widget::onOpenFolderMenuTriggered(QAction *action) {
    openFolder(action->text());
}

void Widget::onOpenFileMenuTriggered(QAction *action) {
    openFile(action->text());
}

void Widget::onAddNewSoftware(bool checked) {
    auto folder_path = QFileDialog::getOpenFileName(this, TipString::CHOOSE_A_EXE, TipString::CURRENT_PATH, TipString::EXE_FILTER);
    if (!folder_path.isEmpty()) {
        need_save = true;
        addSoftware(QFileInfo(folder_path));
    }
}

void Widget::onAddNewFolder(bool checked) {
    auto folder_path = QFileDialog::getExistingDirectory(this, TipString::CHOOSE_A_FOLDER, TipString::CURRENT_PATH);
    if (!folder_path.isEmpty()) {
        need_save = true;
        addFolder(QFileInfo(folder_path).fileName(), folder_path);
    }
}

void Widget::onAddNewFile(bool checked) {
    auto folder_path = QFileDialog::getOpenFileName(this, TipString::CHOOSE_A_FILE, TipString::CURRENT_PATH);
    if (!folder_path.isEmpty()) {
        need_save = true;
        addFile(QFileInfo(folder_path));
    }
}

void Widget::showActAnimation(const QString& k) {
//    qDebug(k.toStdString().c_str());
    movie->stop();
    cur_role_act = &k;
    movie->setFileName(action_map.value(k));

    if (movie->isValid()) {
        movie->start();
        label->setMovie(movie);
    } else {
        // TODO: 执行待机动作, 并防止死循环(可能存在bug)
        if (k != Standby_Role_Act) {
            showActAnimation(Standby_Role_Act);
        }
    }
}

void Widget::runSoftware(const QString& k) {
    auto software = software_map.value(k, nullptr);
    if (software != nullptr) {
        QProcess::startDetached(TipString::SUPPORT_CHINESE_IN_PATH + software + TipString::SUPPORT_CHINESE_IN_PATH);
    }
}

void Widget::openFolder(const QString& k) {
    auto folder = folder_map.value(k, nullptr);
    if (folder != nullptr) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    }
}

void Widget::openFile(const QString& k) {
    auto file = file_map.value(k, nullptr);
    if (file != nullptr) {
        // BUG: 偶现, 打开md文件时, 退出Qt程序, 打开的md文件也关闭了
        // 使用W即Unicode字符集即可解决中文路径问题
        ShellExecuteW(nullptr, nullptr,file.toStdWString().c_str(),
                      nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void Widget::bindMouseListener() {
    if (drag_filter != nullptr) {
        installEventFilter(drag_filter);
    }
}

void Widget::removeMouseListener() {
    if (drag_filter != nullptr) {
        removeEventFilter(drag_filter);
    }
}

void Widget::setSystemTrayIcon(QSystemTrayIcon *systemTrayIcon){
    this->systemTrayIcon = systemTrayIcon;
}

void Widget::makeMenuOnTop(QMenu *menu) {
    // 确保子菜单也始终置顶
    connect(menu, &QMenu::aboutToShow, [menu]() {
        menu->raise();
        menu->activateWindow();
    });
}

void Widget::applyStyle(QMenu *menu) {
    // 应用样式并设置透明
    menu->setProperty("class", QStringLiteral("blackMenu"));
    menu->setWindowFlag(Qt::FramelessWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
}

void Widget::configMenu(QMenu *menu) {
    makeMenuOnTop(menu);
    applyStyle(menu);
}

bool Widget::isStandby() {
    return *cur_role_act == Standby_Role_Act;
}

bool Widget::isReady() {
    return drag_filter != nullptr and deleteOrEdit_filter != nullptr;
}

QIcon Widget::getIconFromFile(const QFileInfo &fileInfo) {
    return iconProvider.icon(fileInfo);
}

void Widget::showRandomAction() {
    qsrand(QTime::currentTime().msec());
    int action_count = action_map.count();
    int random_act = qrand() % (action_count * 3);  // 待机:其他 = 2:1
    if (random_act < 2 * action_count) {
        showActAnimation(Standby_Role_Act);
    } else {
        random_act = random_act - 2 * action_count;
        auto iter = action_map.begin();
        for (; random_act != 0; ++iter, --random_act);
        showActAnimation(iter.key());
    }
}

void Widget::addSoftware(const QFileInfo &fileInfo) {
    software_map.insert(fileInfo.baseName(), fileInfo.filePath());
    addSoftwareOnMenu(fileInfo.baseName(), getIconFromFile(fileInfo));
}

void Widget::addSoftwareOnMenu(const QString &software_name, const QIcon &software_icon) {
    auto *run_software_action = new QAction(software_icon, software_name, software_menu);
    connect(run_software_action, &QAction::triggered, this, [this, run_software_action] () {
        onRunSoftwareMenuTriggered(run_software_action);
    });
    software_menu->addAction(run_software_action);
}

void Widget::addFolder(const QString &folder_name, const QString &folder_path) {
    folder_map.insert(folder_name, folder_path);
    addFolderOnMenu(folder_name);
}

void Widget::addFolderOnMenu(const QString &folder_name) {
    auto *open_folder_action = new QAction(folder_name, folder_menu);
    connect(open_folder_action, &QAction::triggered, this, [this, open_folder_action] () {
        onOpenFolderMenuTriggered(open_folder_action);
    });
    folder_menu->addAction(open_folder_action);
}

void Widget::addFile(const QFileInfo &fileInfo) {
    file_map.insert(fileInfo.fileName(), fileInfo.filePath());
    addFileOnMenu(fileInfo.fileName(), getIconFromFile(fileInfo));
}

void Widget::addFileOnMenu(const QString &file_name, const QIcon &file_icon) {
    auto *open_file_action = new QAction(file_icon, file_name, file_menu);
    connect(open_file_action, &QAction::triggered, this, [this, open_file_action] () {
        onOpenFileMenuTriggered(open_file_action);
    });
    file_menu->addAction(open_file_action);
}

bool Widget::saveQMapToJsonFile(const QString& filename) {
    QJsonObject json;
    QJsonObject json_action_map;
    for (auto it = action_map.begin(); it != action_map.end(); ++it) {
        json_action_map[it.key()] = QJsonValue(it.value());
    }

    QJsonObject json_software_map;
    for (auto it = software_map.begin(); it != software_map.end(); ++it) {
        json_software_map[it.key()] = QJsonValue(it.value());
    }

    QJsonObject json_folder_map;
    for (auto it = folder_map.begin(); it != folder_map.end(); ++it) {
        json_folder_map[it.key()] = QJsonValue(it.value());
    }

    QJsonObject json_file_map;
    for (auto it = file_map.begin(); it != file_map.end(); ++it) {
        json_file_map[it.key()] = QJsonValue(it.value());
    }

    json["width"] = width;
    json["height"] = height;

    json["action"] = json_action_map;
    json["software"] = json_software_map;
    json["folder"] = json_folder_map;
    json["file"] = json_file_map;

    QJsonDocument doc(json);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool Widget::loadQMapFromJsonFile(const QString& filename) {
    QFile file(filename);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(nullptr,
                              TipString::FILE_LOAD_ERROR, settings_json_path);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject json = doc.object();

    width = json.value(QLatin1String("width")).toInt();
    height = json.value(QLatin1String("height")).toInt();

    // 该语句报错, 所以没法直接写在for里, 考虑用size去迭代
    // qDebug("%s", json["action_map"].toObject().end().key().toStdString().c_str());

    auto action_json = json["action"].toObject();
    for (auto it = action_json.begin(); it != action_json.end(); ++it) {
        action_map.insert(it.key(), it.value().toString());
    }
    auto software_json = json["software"].toObject();
    for (auto it = software_json.begin(); it != software_json.end(); ++it) {
        software_map.insert(it.key(), it.value().toString());
    }
    auto folder_json = json["folder"].toObject();
    for (auto it = folder_json.begin(); it != folder_json.end(); ++it) {
        folder_map.insert(it.key(), it.value().toString());
    }
    auto file_json = json["file"].toObject();
    for (auto it = file_json.begin(); it != file_json.end(); ++it) {
        file_map.insert(it.key(), it.value().toString());
    }

    return true;
}

void Widget::exit() {
    software_menu->removeEventFilter(deleteOrEdit_filter);
    folder_menu->removeEventFilter(deleteOrEdit_filter);
    removeMouseListener();
    if (need_save) {
        saveQMapToJsonFile(settings_json_path);
    }
    close();
}

void Widget::showBorder() {
    setWindowFlag(Qt::Window);
}

void Widget::hideBorder() {
    setWindowFlag(Qt::FramelessWindowHint);
}

void Widget::deleteOrEditAction(QMenu *menu_have_action, QAction *action) {
    auto result = QMessageBox::question(this,
                                        QStringLiteral("关于: ") + action->text(),
                                        TipString::EDIT_OR_DELETE,
                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                        QMessageBox::Cancel);
    if (result == QMessageBox::No) {  // 删除
        if (menu_have_action == folder_menu) {
            folder_map.remove(action->text());
            need_save = true;
        } else if (menu_have_action == software_menu) {
            software_map.remove(action->text());
            need_save = true;
        } else if (menu_have_action == file_menu) {
            file_map.remove(action->text());
            need_save = true;
        } else {
            return;
        }
        menu_have_action->removeAction(action);
        delete action;
    } else if (result == QMessageBox::Yes){  // 修改
        bool ok;
        QString text = QInputDialog::getText(this,
                                             QStringLiteral("修改 ") + action->text(),
                                             TipString::INPUT_NEWNAME,
                                             QLineEdit::Normal, "", &ok);
        // 检查用户是否点击了 OK
        if (ok && !text.isEmpty()) {
            // 用户输入有效，显示输入的内容
            if (menu_have_action == folder_menu) {
                folder_map.insert(text, folder_map.value(action->text()));
                folder_map.remove(action->text());
                action->setText(text);
                menu->update();
                need_save = true;
            } else if (menu_have_action == software_menu) {
                software_map.insert(text, software_map.value(action->text()));
                software_map.remove(action->text());
                action->setText(text);
                menu->update();
                need_save = true;
            } else if (menu_have_action == file_menu) {
                file_map.insert(text, file_map.value(action->text()));
                file_map.remove(action->text());
                action->setText(text);
                menu->update();
                need_save = true;
            } else {
                return;
            }
        }
    } else {
        return;
    }
}
