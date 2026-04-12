#include "dlgpreferences.h"

#include "mainwindow.h"
#include "ui_dlgpreferences.h"
#include "ui_mainwindow.h"

extern MainWindow* mw_one;

dlgPreferences::dlgPreferences(QWidget* parent)
    : QDialog(parent), ui(new Ui::dlgPreferences) {
  ui->setupUi(this);
  ui->groupBox_Options->setVisible(false);
}

dlgPreferences::~dlgPreferences() { delete ui; }

void dlgPreferences::on_btnFont_clicked() { mw_one->set_Font(); }

void dlgPreferences::on_cboxWrapWord_stateChanged(int arg1) {
  Q_UNUSED(arg1);
  mw_one->set_wrap();
}

void dlgPreferences::on_rbtnUTF8_clicked() {
  mw_one->lblEncoding->setText("UTF-8");
}

void dlgPreferences::on_rbtnGBK_clicked() {
  mw_one->lblEncoding->setText("GBK");
}
