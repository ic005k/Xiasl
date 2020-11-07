#ifndef DLGDECOMPILE_H
#define DLGDECOMPILE_H

#include <QDialog>
#include <QFileDialog>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

namespace Ui {
class dlgDecompile;
}

class dlgDecompile : public QDialog
{
    Q_OBJECT

public:
    explicit dlgDecompile(QWidget *parent = nullptr);
    ~dlgDecompile();

private slots:
    void on_btnAddDSDT_clicked();

    void on_btnAddSSDT_clicked();

    void on_btnRemoveSSDT_clicked();

    void on_btnDecompile_clicked();

    void on_btnClearList_clicked();

    void readDecompileResult(int exitCode);

public slots:


private:
    Ui::dlgDecompile *ui;

    QProcess *Decompile;
};

#endif // DLGDECOMPILE_H
