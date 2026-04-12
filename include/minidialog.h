#ifndef MINIDIALOG_H
#define MINIDIALOG_H

#include <QDialog>
#include <QScrollBar>

#include <Qsci/qsciscintilla.h>

namespace Ui {
class miniDialog;
}

class miniDialog : public QDialog {
    Q_OBJECT

public:
    explicit miniDialog(QWidget* parent = nullptr);
    ~miniDialog();

private:
    Ui::miniDialog* ui;
};

class ZoomEditor : public QsciScintilla {
    Q_OBJECT

public:
    ZoomEditor(QWidget* parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
private slots:

private:
};

#endif // MINIDIALOG_H
