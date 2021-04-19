#include "dlgdecompile.h"
#include "mainwindow.h"
#include "ui_dlgdecompile.h"

extern MainWindow* mw_one;

dlgDecompile::dlgDecompile(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::dlgDecompile)
{
    ui->setupUi(this);

    ui->tabWidget->tabBar()->setCurrentIndex(0);
}

dlgDecompile::~dlgDecompile()
{
    delete ui;
}

void dlgDecompile::on_btnAddDSDT_clicked()
{
    QString file;
    file = QFileDialog::getOpenFileName(
        this, tr("Select files to open"),
        "", "aml list (*.aml);;dat list(*.dat);;All files(*.*)");

    if (file != "")
        ui->editDSDT->setText(file);
}

void dlgDecompile::on_btnAddSSDT_clicked()
{
    QStringList list;
    list = QFileDialog::getOpenFileNames(
        this, tr("Select files to open"),
        "", "aml list (*.aml);;dat list(*.dat);;All files(*.*)");

    ui->listSSDT->addItems(list);
}

void dlgDecompile::on_btnRemoveSSDT_clicked()
{

    QListWidgetItem* sel = ui->listSSDT->currentItem();

    int r = ui->listSSDT->row(sel);

    QListWidgetItem* item = ui->listSSDT->takeItem(r);

    ui->listSSDT->removeItemWidget(item);

    delete item;
}

void dlgDecompile::on_btnDecompile_clicked()
{
    QFileInfo appInfo(qApp->applicationDirPath());
    Decompile = new QProcess;

    QStringList list;
    for (int i = 0; i < ui->listSSDT->count(); i++) {
        ui->listSSDT->setCurrentRow(i);

        list.append(ui->listSSDT->currentItem()->text());
    }

#ifdef Q_OS_WIN32

    Decompile->start(appInfo.filePath() + "/iasl.exe", QStringList() << "-e" << list << "-d" << ui->editDSDT->text());
#endif

#ifdef Q_OS_LINUX

    Decompile->start(appInfo.filePath() + "/iasl", QStringList() << "-e" << list << "-d" << ui->editDSDT->text());

#endif

#ifdef Q_OS_MAC

    Decompile->start(appInfo.filePath() + "/iasl", QStringList() << "-e" << list << "-d" << ui->editDSDT->text());
#endif

    connect(Decompile, SIGNAL(finished(int)), this, SLOT(readDecompileResult(int)));

#ifdef Q_OS_WIN32

    Decompile->execute(appInfo.filePath() + "/iasl.exe", QStringList() << "-e" << list << "-d" << ui->editDSDT->text());
#endif

#ifdef Q_OS_LINUX

    Decompile->execute(appInfo.filePath() + "/iasl", QStringList() << "-e" << list << "-d" << ui->editDSDT->text());

#endif

#ifdef Q_OS_MAC

    Decompile->execute(appInfo.filePath() + "/iasl", QStringList() << "-e" << list << "-d" << ui->editDSDT->text());
#endif

    QFileInfo fi(ui->editDSDT->text());
    QString file = fi.path() + "/" + fi.baseName() + ".dsl";
    QFileInfo out(file);
    if (out.exists())
        mw_one->loadFile(file, -1, -1);
}

void dlgDecompile::on_btnClearList_clicked()
{
    ui->listSSDT->clear();
}

void dlgDecompile::readDecompileResult(int exitCode)
{

    QString result, result1;
    result = QString::fromUtf8(Decompile->readAllStandardOutput());
    result1 = QString::fromUtf8(Decompile->readAllStandardError());

    ui->textLog->clear();
    ui->textLog->append(result);
    ui->textLog->append(result1);

    if (exitCode == 0) {
        //成功

    } else {
    }
}
