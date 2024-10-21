#ifndef EGGYPET_DRAGFILTER_H
#define EGGYPET_DRAGFILTER_H

class DragFilter : public QObject {

    // 我们可以向一个对象上面安装多个事件处理器，只要调用多次installEventFilter()函数。
    // 如果一个对象存在多个事件过滤器，那么，最后一个安装的会第一个执行，也就是后进先执行的顺序。
    // 事件过滤器的强大之处在于，我们可以为整个应用程序添加一个事件过滤器。
    // 记得，installEventFilter()函数是QObject的函数，QApplication或者QCoreApplication对象都是QObject的子类。
    // 因此，我们可以向QApplication或者QCoreApplication添加事件过滤器。
    // 这种全局的事件过滤器将会在所有其它特性对象的事件过滤器之前调用。
    // 尽管很强大，但这种行为会严重降低整个应用程序的事件分发效率。因此，除非是不得不使用的情况，否则的话我们不应该这么做。

public:
    inline bool eventFilter(QObject *watched, QEvent *event) override {
        // 事件过滤器的调用时间是目标对象（也就是参数里面的watched对象）接收到事件对象之前。
        // 也就是说，如果你在事件过滤器中停止了某个事件，那么，watched对象以及以后所有的事件过滤器根本不会知道这么一个事件。

        auto *w = dynamic_cast<QWidget*>(watched);
        if (w == nullptr) {
            return false; // 继续转发事件, return true则处理事件，阻止默认处理
        } else {

            // 之前出现过:播放除待机外其他动作时拖动会崩溃
            // 此为临时解决方案
            //            if (!w->isStandby()) {
            //                return false;
            //            }
        }

        if (event->type() == QEvent::MouseButtonPress) {
            auto e = dynamic_cast<QMouseEvent*>(event);
            if (e) {
                pos = e->pos();
                if (!isMoving & e->buttons() & Qt::MouseButton::LeftButton) {
                    //                    w->showRandomAction();
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto e = dynamic_cast<QMouseEvent*>(event);
            if (e) {
                if (e->buttons() & Qt::MouseButton::LeftButton) {
                    isMoving = true;
                    w->move(e->globalPos() - pos);
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            isMoving = false;
        }

        // 调用父类的函数处理其他部件的过滤器
        return QObject::eventFilter(watched, event);
    }

private:
    QPoint pos;
    bool isMoving = false;
};

#endif //EGGYPET_DRAGFILTER_H
