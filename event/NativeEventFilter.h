// 用于在Windows上实现全局快捷键
#ifndef EGGYPET_NATIVEEVENTFILTER_H
#define EGGYPET_NATIVEEVENTFILTER_H

#include <QAbstractNativeEventFilter>

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    NativeEventFilter(const unsigned int &mod, const unsigned int &key, QAction *action) {
        this->mod = mod;
        this->key = key;
        this->action = action;
    };

    inline bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_HOTKEY) {
            const quint32 keycode = HIWORD(msg->lParam);
            const quint32 modifiers = LOWORD(msg->lParam);
            if (keycode == key && mod == modifiers) {
                action->trigger();
            }
        }
        return false;
    };

public:
    unsigned int mod;
    unsigned int key;

    QAction *action;
};

#endif //EGGYPET_NATIVEEVENTFILTER_H
