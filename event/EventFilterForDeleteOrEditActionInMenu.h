#ifndef EGGYPET_EVENTFILTERFORDELETEOREDITACTIONINMENU_H
#define EGGYPET_EVENTFILTERFORDELETEOREDITACTIONINMENU_H

#include "../ui/widget.h"
class Widget;

class EventFilterForDeleteOrEditActionInMenu : public QObject {

    // 事件过滤器的工作原理是基于事件接收者，而不是事件目标。
    // 因此，您应该为包含QAction的菜单或工具栏设置事件过滤器，而不是为QAction本身设置。
    // 经过测试为QAction本身设置事件过滤器无效

public:
    inline bool eventFilter(QObject *watched, QEvent *event) override {
        auto menu = dynamic_cast<QMenu*>(watched);
        auto e = dynamic_cast<QMouseEvent*>(event);
        if (menu && e) {
            if (e->button() == Qt::RightButton && e->type() == QEvent::MouseButtonRelease) {
                // 获取点击位置的 QAction
                QAction *clickedAction = menu->actionAt(e->pos());
                if (clickedAction) {
                    // software_menu(folder_menu)的parent是menu, menu的parent是widget
                    auto *w = dynamic_cast<Widget*>(menu->parent()->parent());
                    if (w) {  // Widget若转化失败, 会传向信号槽
                        w->deleteOrEditAction(menu, clickedAction);
                        return true;  // 返回ture阻止事件传向信号槽
                    }
                }
            } else {
                return false; // 处理其他鼠标事件
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

#endif //EGGYPET_EVENTFILTERFORDELETEOREDITACTIONINMENU_H
