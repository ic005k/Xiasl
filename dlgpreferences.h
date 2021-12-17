#ifndef DLGPREFERENCES_H
#define DLGPREFERENCES_H

#include <QDialog>

namespace Ui {
class dlgPreferences;
}

class dlgPreferences : public QDialog {
  Q_OBJECT

 public:
  explicit dlgPreferences(QWidget *parent = nullptr);
  ~dlgPreferences();
  Ui::dlgPreferences *ui;

 private slots:
  void on_btnFont_clicked();

  void on_cboxWrapWord_stateChanged(int arg1);

  void on_rbtnUTF8_clicked();

  void on_rbtnGBK_clicked();

 private:
};

#endif  // DLGPREFERENCES_H
