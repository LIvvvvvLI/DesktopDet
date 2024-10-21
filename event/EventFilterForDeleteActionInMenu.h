#ifndef EGGYPET_EVENTFILTERFORDELETEACTIONINMENU_H
#define EGGYPET_EVENTFILTERFORDELETEACTIONINMENU_H

#include <QMenu>
#include <QEvent>
#include <QMouseEvent>
#include <windows.h>

class EventFilterForDeleteActionInMenu : public QObject {

public:
    inline bool eventFilter(QObject *watched, QEvent *event) override {
        auto menu = dynamic_cast<QMenu*>(watched);
        auto e = dynamic_cast<QMouseEvent*>(event);
        if (menu && e) {
            if (e->button() == Qt::RightButton && e->type() == QEvent::MouseButtonRelease) {
                // 获取点击位置的 QAction
                QAction *clickedAction = menu->actionAt(e->pos());
                if (clickedAction) {
                    HWND hwnd = FindWindowW(nullptr, clickedAction->text().toStdWString().c_str());
                    if (hwnd) {
                        // 关闭窗口
                        HICON hIcon = (HICON) GetClassLong(reinterpret_cast<HWND>(hwnd), GCL_HICON);
                        if (!hIcon) {
                            // 这种情况大概率是文件夹, 所以直接关闭
                            SendMessage(hwnd, WM_CLOSE, 0, 0);
                            menu->removeAction(clickedAction);
                        } else {
                            // 其他窗口可能会弹出确认关闭窗口, 这就需要将窗口z序前置
                            // TODO: 某种意义上讲, SetForegroundWindow会导致关闭窗口的过程中Pet失去焦点会导致体验不佳
                            SetForegroundWindow(hwnd);
                            // BUG: 无法关闭Pet.exe所在目录的文件夹
                            SendMessage(hwnd, WM_CLOSE, 0, 0);
                            // 这个if其实不是很有意义.
                            // 因为对要关闭的窗口操作过的话会导致Pet失去焦点, 从而ContextMenu关闭. 下次打开时current_menu会重建
//                            if (!IsWindow(hwnd)) {
//                                menu->removeAction(clickedAction);
//                            }
                        }
                        return true;
                    }
                }
            } else {
                return false; // 处理其他鼠标事件
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

#endif //EGGYPET_EVENTFILTERFORDELETEACTIONINMENU_H
