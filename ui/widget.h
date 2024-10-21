#ifndef EGGYPET_WIDGET_H
#define EGGYPET_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMovie>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QFileInfo>
#include <QFileIconProvider>

class QPaintEvent;
class DragFilter;
class EventFilterForDeleteOrEditActionInMenu;

class Widget : public QWidget {
    Q_OBJECT
public:
    Widget(const QString &settings_json_path, const QString &record_file_path, QWidget *parent = nullptr);
    ~Widget();

public:
    void showActAnimation(const QString& k);

    void bindMouseListener();
    void removeMouseListener();
    void hideBorder();
    void showBorder();
    void exit();

    void setSystemTrayIcon(QSystemTrayIcon *systemTrayIcon);
    bool isStandby();
    bool isReady();
    void showRandomAction();
    QIcon getIconFromFile(const QFileInfo &fileInfo);

    bool saveQMapToJsonFile(const QString& filename);
    bool loadQMapFromJsonFile(const QString& filename);

    void runSoftware(const QString& k);
    void openFolder(const QString& k);
    void openFile(const QString& k);
    void deleteOrEditAction(QMenu *menu_have_action, QAction *action);

protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void initMenu();

    void addSoftware(const QFileInfo &fileInfo);
    void addSoftwareOnMenu(const QString &software_name, const QIcon &software_icon);
    void addFolder(const QString &folder_name, const QString &folder_path);
    void addFolderOnMenu(const QString &folder_name);
    void addFile(const QFileInfo &fileInfo);
    void addFileOnMenu(const QString &software_name, const QIcon &software_icon);

    static void makeMenuOnTop(QMenu *menu);
    static void applyStyle(QMenu *menu);
    static void configMenu(QMenu *menu);

private slots:
    void onMovieFinished();

    void onActionMenuTriggered(QAction *action);
    void onRunSoftwareMenuTriggered(QAction *action);
    void onOpenFolderMenuTriggered(QAction *action);
    void onOpenFileMenuTriggered(QAction *action);

    void onAddNewSoftware(bool checked);
    void onAddNewFolder(bool checked);
    void onAddNewFile(bool checked);

public:
    // 可全局响应的QAction
    QAction *essay_action = nullptr;

private:
    int width = 0;
    int height = 0;

    int last_windows = -1;

    const QString &settings_json_path;
    bool need_save = false;  // for settings.json
    const QString &record_file_path;

    QLabel *label;
    QMovie *movie;
    QMenu *menu;
    QMenu *software_menu;
    QMenu *folder_menu;
    QMenu *file_menu;

    DragFilter *drag_filter = nullptr;
    EventFilterForDeleteOrEditActionInMenu *deleteOrEdit_filter = nullptr;

    QSystemTrayIcon *systemTrayIcon = nullptr;

    QFileIconProvider iconProvider;

    const QString Standby_Role_Act = QStringLiteral("待机");
    const QString *cur_role_act = &Standby_Role_Act;

    bool always_standby = false;

    QMap<QString, QString> action_map;
    QMap<QString, QString> software_map;
    QMap<QString, QString> folder_map;
    QMap<QString, QString> file_map;

};

#endif //EGGYPET_WIDGET_H
