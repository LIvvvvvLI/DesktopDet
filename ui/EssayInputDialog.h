#ifndef EGGYPET_ESSAYINPUTDIALOG_H
#define EGGYPET_ESSAYINPUTDIALOG_H

#include <QDialog>
#include <QTextEdit>

class EssayInputDialog : public QDialog {
public:
    EssayInputDialog(const QString &path, QWidget *parent = nullptr);
    ~EssayInputDialog() { delete textEdit; };
protected:
    void showEvent(QShowEvent *event) override;

public:
    QTextEdit *textEdit = nullptr;
};

#endif //EGGYPET_ESSAYINPUTDIALOG_H
