#include "EssayInputDialog.h"
#include "../utils/StringConstants.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QTime>

EssayInputDialog::EssayInputDialog(const QString &path, QWidget *parent) : QDialog(parent) {
    textEdit = new QTextEdit(this);

    QPushButton *okButton = new QPushButton("确认", this);
    connect(okButton, &QPushButton::clicked, this, [this, path]{
        this->accept();
        const QString &str = this->textEdit->toPlainText();
        if (str.size() != 0) {
            QFile file(path);
            if (!file.open(QIODevice::Append | QIODevice::Text)) { // 以追加模式打开文件
                QMessageBox::warning(nullptr,
                                     TipString::FILE_LOAD_ERROR, path);
                this->accept();
                return;
            }
            QTextStream out(&file);
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            // 写入格式: [yyyy-MM-dd HH:mm] str(去掉首尾空格和换行)
            out << "[" << timestamp << "] " << str.trimmed() << "\n";

            file.close(); // 关闭文件
        }
    });

    // 大键盘上的回车是Return, 小键盘上的回车才是Enter
    okButton->setShortcut(QKeySequence("Ctrl+Return"));

    QPushButton *cancelButton = new QPushButton("取消", this);
    connect(cancelButton, &QPushButton::clicked, this, [this]{
        this->accept();
    });

    QVBoxLayout *v_layout = new QVBoxLayout();
    QHBoxLayout *h_layout = new QHBoxLayout();

    h_layout->addWidget(cancelButton);
    h_layout->addWidget(okButton);
    v_layout->addWidget(textEdit);
    v_layout->addLayout(h_layout);
    setLayout(v_layout);

    // 不显示窗口右上角问号
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::Window);
    // 通过new在堆上创建EssayInputDialog对象, 使用.show()后防止内存泄露
    setAttribute(Qt::WA_DeleteOnClose);

}

void EssayInputDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    // 将窗口中心move到鼠标位置, 由于在构造函数中获得的size()并不准确, 所以改在showEvent中处理
    auto size = this->size();
    auto pos = QCursor::pos();
    move(pos.x() - size.width() / 2, pos.y() - size.height() / 2);
}
